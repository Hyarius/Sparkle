#include "battle/query/battle_query_service.hpp"

#include "abilities/ability_definition.hpp"
#include "battle/battle_context.hpp"
#include "board/board_line_of_sight.hpp"
#include "core/registries.hpp"

#include <algorithm>
#include <cstdlib>
#include <limits>

namespace pg
{
	namespace
	{
		bool permits(const TargetFilter &f, const BattleUnit &source, const BattleUnit *occupant)
		{
			if (occupant == nullptr) return f.allowEmptyCell;
			if (occupant->id() == source.id()) return f.allowSelf;
			return occupant->side() == source.side() ? f.allowAllies : f.allowEnemies;
		}
		int absDiff(int a, int b) { const long long d = static_cast<long long>(a) - b; return static_cast<int>(d < 0 ? -d : d); }
		bool inRange(const RangeDefinition &range, const BattleUnit &source, BoardCell from, BoardCell to)
		{
			const int dx = absDiff(to.x, from.x), dz = absDiff(to.z, from.z);
			const long long max = static_cast<long long>(range.maximum) + source.range();
			if (max > std::numeric_limits<int>::max()) return false;
			const int limit = static_cast<int>(max);
			switch (range.shape) {
			case RangeShape::Self: return dx == 0 && dz == 0;
			case RangeShape::Diamond: { const int d = dx + dz; return d >= range.minimum && d <= limit; }
			case RangeShape::OrthogonalLine: { const int d = std::max(dx,dz); return (dx == 0 || dz == 0) && d >= range.minimum && d <= limit; }
			case RangeShape::DiagonalLine: { const int d = std::max(dx,dz); return dx == dz && d >= range.minimum && d <= limit; }
			}
			return false;
		}
	}

	std::expected<MovePlan, CommandRejection> BattleQueryService::planMove(BattleUnitId id, BoardCell destination) const
	{
		const BattleUnit *unit = _context.tryUnit(id);
		if (!unit) return std::unexpected(CommandRejection::UnknownUnit);
		if (!unit->placed()) return std::unexpected(CommandRejection::UnitNotPlaced);
		if (!unit->isActiveCombatant()) return std::unexpected(CommandRejection::NotActiveUnit);
		const auto origin = _context.board().occupancy().cellOf(id);
		if (!origin) return std::unexpected(CommandRejection::UnitNotPlaced);
		if (*origin == destination) return std::unexpected(CommandRejection::NoStateChange);
		if (!_context.board().isStandable(destination)) return std::unexpected(CommandRejection::UnknownBoardCell);
		if (_context.board().occupancy().unitAt(destination).has_value()) return std::unexpected(CommandRejection::DestinationBlocked);
		const auto path = findWeightedPath(_context.board().navigation(), *origin, destination, unit->movementPoints(), boardMovementQuery(_context.board(), id));
		if (!path) return std::unexpected(CommandRejection::DestinationUnreachable);
		MovePlan result{.unit=id,.origin=*origin,.destination=destination,.totalMovementPointCost=static_cast<std::uint32_t>(path->totalCost)};
		for (std::size_t i=1;i<path->cells.size();++i) result.steps.push_back({path->cells[i-1],path->cells[i],static_cast<std::uint32_t>(_context.board().movementCost(path->cells[i]))});
		return result;
	}

	std::expected<CastPlan, CommandRejection> BattleQueryService::planCast(BattleUnitId id, std::string_view abilityId, BoardCell anchor) const
	{
		const BattleUnit *source = _context.tryUnit(id);
		if (!source) return std::unexpected(CommandRejection::UnknownUnit);
		if (!source->placed()) return std::unexpected(CommandRejection::UnitNotPlaced);
		const AbilityDefinition *ability = _context.registries().abilities().tryGet(std::string(abilityId));
		if (!ability) return std::unexpected(CommandRejection::UnknownAbility);
		if (std::ranges::find(source->abilityIds(), ability->id) == source->abilityIds().end()) return std::unexpected(CommandRejection::AbilityNotOwned);
		if (source->actionPoints() < ability->cost.actionPoints) return std::unexpected(CommandRejection::InsufficientActionPoints);
		if (source->movementPoints() < ability->cost.movementPoints) return std::unexpected(CommandRejection::InsufficientMovementPoints);
		const BoardCell from = *_context.board().occupancy().cellOf(id);
		if (!_context.board().isStandable(anchor)) return std::unexpected(CommandRejection::UnknownBoardCell);
		if (!inRange(ability->range, *source, from, anchor)) return std::unexpected(CommandRejection::AnchorOutOfRange);
		if (ability->range.requiresLineOfSight && !BoardLineOfSight::trace(_context.board(), from, anchor).clear) return std::unexpected(CommandRejection::AnchorLineOfSightBlocked);
		const auto anchorId = _context.board().occupancy().unitAt(anchor);
		const BattleUnit *anchorUnit = anchorId ? &_context.unit(*anchorId) : nullptr;
		if (!permits(ability->anchorFilter, *source, anchorUnit)) return std::unexpected(CommandRejection::AnchorFilterRejected);
		CastPlan result{.source=id,.abilityId=ability->id,.sourceCellAtCapture=from,.anchor=anchor,.primaryUnit=anchorId};
		for (const auto &node : _context.board().navigation().allNodes()) {
			const BoardCell cell=node.position; const int dx=absDiff(cell.x,anchor.x), dz=absDiff(cell.z,anchor.z);
			bool include=false; switch(ability->area.shape) { case AreaShape::Single: include=dx==0&&dz==0; break; case AreaShape::Diamond: include=dx+dz<=ability->area.radius; break; case AreaShape::Square: include=std::max(dx,dz)<=ability->area.radius; break; case AreaShape::Cross: include=(dx==0||dz==0)&&std::max(dx,dz)<=ability->area.radius; break; case AreaShape::Line: { const int sdx=anchor.x-from.x, sdz=anchor.z-from.z; if(sdx==0&&sdz==0) include=dx==0&&dz==0; else if(std::abs(sdx)>=std::abs(sdz)) include=cell.z==anchor.z && (sdx>0 ? cell.x>=anchor.x : cell.x<=anchor.x) && dx<=ability->area.radius; else include=cell.x==anchor.x && (sdz>0 ? cell.z>=anchor.z : cell.z<=anchor.z) && dz<=ability->area.radius; break; }}
			if(include) result.areaCells.push_back(cell);
		}
		std::ranges::sort(result.areaCells, BoardCellLess{}); result.areaCells.erase(std::unique(result.areaCells.begin(),result.areaCells.end()),result.areaCells.end());
		for(const BoardCell &cell: result.areaCells) {
			const auto occupant=_context.board().occupancy().unitAt(cell); const BattleUnit *target=occupant?&_context.unit(*occupant):nullptr;
			if(!permits(ability->affectedFilter,*source,target)) continue;
			result.affectedCells.push_back(cell);
			// areaCells are already in board order, which is the primary captured-unit ordering.
			if(occupant && std::ranges::find(result.affectedUnits,*occupant)==result.affectedUnits.end()) result.affectedUnits.push_back(*occupant);
		}
		return result;
	}

	std::vector<MovePlan> BattleQueryService::legalMoves(BattleUnitId id) const { std::vector<MovePlan> r; for(const auto &n:_context.board().navigation().allNodes()) if(auto p=planMove(id,n.position);p) r.push_back(*p); return r; }
	std::vector<AbilityAnchorPreview> BattleQueryService::abilityAnchors(BattleUnitId id,std::string_view ability) const { std::vector<AbilityAnchorPreview> r; for(const auto &n:_context.board().navigation().allNodes()){ AbilityAnchorPreview p{.anchor=n.position}; auto plan=planCast(id,ability,n.position); if(plan){p.legal=true;p.plan=std::move(*plan);} else p.reasons.push_back(plan.error()); r.push_back(std::move(p)); } return r; }
	bool BattleQueryService::hasAnyLegalNonEndCommand(BattleUnitId id) const { if(!legalMoves(id).empty()) return true; const auto *unit=_context.tryUnit(id); if(!unit) return false; for(const auto &a:unit->abilityIds()) for(const auto &p:abilityAnchors(id,a)) if(p.legal) return false; return false; }
}
