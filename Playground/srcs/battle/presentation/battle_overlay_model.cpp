#include "battle/presentation/battle_overlay_model.hpp"

#include <algorithm>
#include <map>

namespace pg
{
	std::string_view overlayMaskKey(BattleMaskKind p_kind) noexcept
	{
		switch (p_kind)
		{
		case BattleMaskKind::Deployment:
			return "deployment";
		case BattleMaskKind::Reachable:
			return "movement";
		case BattleMaskKind::Path:
			return "path";
		case BattleMaskKind::AbilityRange:
			return "abilityRange";
		case BattleMaskKind::AreaPreview:
			return "areaOfEffect";
		case BattleMaskKind::LineOfSightBlocked:
			return "losBlocked";
		case BattleMaskKind::Hovered:
			return "hovered";
		case BattleMaskKind::Invalid:
			return "invalid";
		}
		return "invalid";
	}

	int overlayMaskPriority(BattleMaskKind p_kind) noexcept
	{
		switch (p_kind)
		{
		case BattleMaskKind::Invalid:
			return 8;
		case BattleMaskKind::Hovered:
			return 7;
		case BattleMaskKind::LineOfSightBlocked:
			return 6;
		case BattleMaskKind::AreaPreview:
			return 5;
		case BattleMaskKind::Path:
			return 4;
		case BattleMaskKind::AbilityRange:
			return 3;
		case BattleMaskKind::Deployment:
			return 2;
		case BattleMaskKind::Reachable:
			return 1;
		}
		return 0;
	}

	const BattleOverlayModel &BattleOverlayModelBuilder::model() const noexcept
	{
		return _model;
	}

	bool BattleOverlayModelBuilder::update(std::vector<BattleMaskCell> p_candidates)
	{
		std::map<BoardCell, BattleMaskKind, CellPositionLess> resolved;
		for (const BattleMaskCell &candidate : p_candidates)
		{
			auto [iterator, inserted] = resolved.emplace(candidate.cell, candidate.kind);
			if (!inserted && overlayMaskPriority(candidate.kind) > overlayMaskPriority(iterator->second))
			{
				iterator->second = candidate.kind;
			}
		}
		std::vector<BattleMaskCell> next;
		next.reserve(resolved.size());
		for (const auto &[cell, kind] : resolved)
		{
			next.push_back({.cell = cell, .kind = kind});
		}
		if (next == _model.cells)
		{
			return false;
		}
		_model.cells = std::move(next);
		++_model.revision;
		return true;
	}

	void BattleOverlayModelBuilder::clear()
	{
		(void)update({});
	}
}
