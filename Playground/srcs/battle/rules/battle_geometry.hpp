#pragma once

#include "structures/math/spk_vector2.hpp"

#include <vector>

namespace pg
{
	// Range/AoE geometry shared by BattleActionValidator (legal targeting) and the battle-input
	// overlay previews. Everything is computed on the board's x/z plane (elevation ignored, per
	// battle.md §targeting); the caller resolves the resulting (x,z) offsets to actual board
	// cells. Truth-table tested in tests/battle/battle_geometry_test.cpp.
	enum class RangeShape
	{
		Circle,	  // Manhattan ring min <= d <= max
		Line,	  // the 4 straight axis rays
		Diagonal, // the 4 diagonal rays
		Self	  // the caster cell only
	};

	enum class AreaShape
	{
		Square,	 // Chebyshev disk (max(|x|,|z|) <= value)
		Cross,	 // Manhattan on the two axes only
		Circle,	 // Manhattan disk
		Line	 // value cells stepping away along the caster->anchor direction
	};

	namespace BattleGeometry
	{
		// p_offset = target - caster on (x, z). Returns whether that offset is inside the range
		// shape's band. min/max are measured in the shape's own step metric.
		[[nodiscard]] bool rangeContains(
			RangeShape p_shape,
			const spk::Vector2Int &p_offset,
			int p_minimumRange,
			int p_maximumRange) noexcept;

		// Offsets (relative to the anchor, on x/z) covered by the AoE. Always includes {0,0};
		// value 0 collapses every shape to the single anchor cell (data-model §5 convention).
		// p_casterToAnchor = anchor - caster; only the Line shape reads it (for its direction).
		[[nodiscard]] std::vector<spk::Vector2Int> areaOffsets(
			AreaShape p_shape,
			int p_value,
			const spk::Vector2Int &p_casterToAnchor);
	}
}
