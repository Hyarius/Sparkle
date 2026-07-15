#include "battle/placement/placement_resolver.hpp"

#include <algorithm>
#include <cstdlib>
#include <set>
#include <variant>

namespace pg
{
	namespace
	{
		[[nodiscard]] BattleConstructionError placementError(const std::string &p_encounterId, std::string p_detail)
		{
			BattleConstructionError error;
			error.code = BattleConstructionErrorCode::EnemyPlacementInvalid;
			error.encounterDefinitionId = p_encounterId;
			error.side = BattleSide::Enemy;
			error.diagnosticDetail = std::move(p_detail);
			return error;
		}

		[[nodiscard]] EnemyPlacementPlan resolveFixed(
			const BoardData &p_board,
			const FixedOpponentPlacementPolicy &p_policy,
			std::size_t p_enemyCount,
			const std::string &p_encounterId)
		{
			if (p_policy.cells.size() < p_enemyCount)
			{
				return EnemyPlacementPlan{
					{}, placementError(p_encounterId, "the fixed placement list is shorter than the enemy roster")};
			}

			const std::span<const BoardCell> zone = p_board.deployment().cells(BattleSide::Enemy);
			std::vector<BoardCell> planned;
			std::set<BoardCell, BoardCellLess> used;
			for (std::size_t index = 0; index < p_enemyCount; ++index)
			{
				const std::array<int, 2> authored = p_policy.cells[index];
				std::optional<BoardCell> resolved;
				for (const BoardCell &candidate : zone)
				{
					if (candidate.x != authored[0] || candidate.z != authored[1])
					{
						continue;
					}
					// First by increasing y then BoardCellLess: the smallest cell of the column.
					if (!resolved.has_value() || BoardCellLess{}(candidate, *resolved))
					{
						resolved = candidate;
					}
				}
				if (!resolved.has_value())
				{
					return EnemyPlacementPlan{
						{},
						placementError(
							p_encounterId,
							"fixed cell [" + std::to_string(authored[0]) + ", " + std::to_string(authored[1]) +
								"] is not a standable enemy-zone column")};
				}
				if (!used.insert(*resolved).second)
				{
					return EnemyPlacementPlan{
						{}, placementError(p_encounterId, "fixed placement resolved two enemies onto one cell")};
				}
				planned.push_back(*resolved);
			}
			return EnemyPlacementPlan{std::move(planned), std::nullopt};
		}

		[[nodiscard]] EnemyPlacementPlan resolveByLine(
			const BoardData &p_board,
			const ByLineOpponentPlacementPolicy &p_policy,
			std::size_t p_enemyCount,
			const std::string &p_encounterId)
		{
			const DeploymentLayout &layout = p_board.deployment();
			const int stripDepth = layout.stripDepth;
			if (p_policy.rowsFromEnemyEdge < 0 || p_policy.rowsFromEnemyEdge >= stripDepth)
			{
				return EnemyPlacementPlan{
					{}, placementError(p_encounterId, "rowsFromEnemyEdge is outside the actual deployment depth")};
			}

			const std::span<const BoardCell> zone = layout.cells(BattleSide::Enemy);
			std::vector<BoardCell> planned;
			for (int row = p_policy.rowsFromEnemyEdge; row < stripDepth && planned.size() < p_enemyCount; ++row)
			{
				std::vector<BoardCell> inRow;
				for (const BoardCell &cell : zone)
				{
					if (layout.rowIndexFromOuterEdge(BattleSide::Enemy, cell) == row)
					{
						inRow.push_back(cell);
					}
				}
				if (inRow.empty())
				{
					continue;
				}

				int minimumPerpendicular = layout.perpendicularCoordinate(inRow.front());
				int maximumPerpendicular = minimumPerpendicular;
				for (const BoardCell &cell : inRow)
				{
					const int perpendicular = layout.perpendicularCoordinate(cell);
					minimumPerpendicular = std::min(minimumPerpendicular, perpendicular);
					maximumPerpendicular = std::max(maximumPerpendicular, perpendicular);
				}
				const int midpointDouble = minimumPerpendicular + maximumPerpendicular;

				std::sort(inRow.begin(), inRow.end(), [&](const BoardCell &p_a, const BoardCell &p_b) {
					const int perpendicularA = layout.perpendicularCoordinate(p_a);
					const int perpendicularB = layout.perpendicularCoordinate(p_b);
					switch (p_policy.order)
					{
					case OpponentLineOrder::LeftToRight:
						if (perpendicularA != perpendicularB)
						{
							return perpendicularA < perpendicularB;
						}
						break;
					case OpponentLineOrder::RightToLeft:
						if (perpendicularA != perpendicularB)
						{
							return perpendicularA > perpendicularB;
						}
						break;
					case OpponentLineOrder::CenterOut:
					{
						const int distanceA = std::abs(2 * perpendicularA - midpointDouble);
						const int distanceB = std::abs(2 * perpendicularB - midpointDouble);
						if (distanceA != distanceB)
						{
							return distanceA < distanceB;
						}
						if (perpendicularA != perpendicularB)
						{
							return perpendicularA < perpendicularB;
						}
						break;
					}
					}
					// Stacked supports in one column tie-break by increasing y then BoardCellLess.
					return BoardCellLess{}(p_a, p_b);
				});

				for (const BoardCell &cell : inRow)
				{
					if (planned.size() >= p_enemyCount)
					{
						break;
					}
					planned.push_back(cell);
				}
			}

			if (planned.size() < p_enemyCount)
			{
				return EnemyPlacementPlan{
					{}, placementError(p_encounterId, "the enemy deployment strip cannot seat the whole roster by line")};
			}
			return EnemyPlacementPlan{std::move(planned), std::nullopt};
		}

		[[nodiscard]] EnemyPlacementPlan resolveSeededRandom(
			const BoardData &p_board,
			std::size_t p_enemyCount,
			BattleRng &p_rng,
			const std::string &p_encounterId)
		{
			// Canonical, RNG-free ordering first, out of the frozen graph, never out of unordered
			// occupancy output. Every enemy-zone cell is free at construction, but the occupancy check
			// keeps this correct if a later step reuses it mid-battle.
			std::vector<BoardCell> pool;
			for (const BoardCell &cell : p_board.deployment().cells(BattleSide::Enemy))
			{
				if (!p_board.occupancy().unitAt(cell).has_value())
				{
					pool.push_back(cell);
				}
			}
			std::sort(pool.begin(), pool.end(), BoardCellLess{});

			// Capacity is proven before a single draw, so a failure consumes zero RNG.
			if (pool.size() < p_enemyCount)
			{
				return EnemyPlacementPlan{
					{}, placementError(p_encounterId, "not enough free enemy-zone cells for a seeded-random roster")};
			}

			// Partial Fisher-Yates: exactly one uniformBelow per roster entry, so seed + board + team
			// pin both the placements and the draw count.
			for (std::size_t index = 0; index < p_enemyCount; ++index)
			{
				const std::uint64_t remaining = static_cast<std::uint64_t>(pool.size() - index);
				const std::size_t pick = index + static_cast<std::size_t>(p_rng.uniformBelow(remaining));
				std::swap(pool[index], pool[pick]);
			}
			pool.resize(p_enemyCount);
			return EnemyPlacementPlan{std::move(pool), std::nullopt};
		}
	}

	bool isLegalDeploymentCell(const BoardData &p_board, BattleSide p_side, const BoardCell &p_cell) noexcept
	{
		return p_board.deployment().contains(p_side, p_cell) && p_board.isStandable(p_cell);
	}

	EnemyPlacementPlan planEnemyPlacements(
		const BoardData &p_board,
		const OpponentPlacementPolicy &p_policy,
		std::size_t p_enemyCount,
		BattleRng &p_rng,
		const std::string &p_encounterDefinitionId)
	{
		return std::visit(
			[&](const auto &p_specific) -> EnemyPlacementPlan {
				using Policy = std::decay_t<decltype(p_specific)>;
				if constexpr (std::is_same_v<Policy, FixedOpponentPlacementPolicy>)
				{
					return resolveFixed(p_board, p_specific, p_enemyCount, p_encounterDefinitionId);
				}
				else if constexpr (std::is_same_v<Policy, ByLineOpponentPlacementPolicy>)
				{
					return resolveByLine(p_board, p_specific, p_enemyCount, p_encounterDefinitionId);
				}
				else
				{
					return resolveSeededRandom(p_board, p_enemyCount, p_rng, p_encounterDefinitionId);
				}
			},
			p_policy);
	}
}
