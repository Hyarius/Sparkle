#pragma once

#include "board/board_cell.hpp"
#include "creatures/creature_species_definition.hpp"
#include "structures/graphics/geometry/spk_color.hpp"
#include "structures/math/spk_vector3.hpp"

#include <string_view>

namespace pg
{
	class BoardData;

	inline constexpr float BattleUnitBaseEdge = 0.65f;
	inline constexpr float BattleUnitFootClearance = 0.01f;

	// Presentation conversion lives at the boundary.  PlaceholderVisual intentionally remains
	// engine-free in the immutable content layer.
	[[nodiscard]] spk::Color toSpkColor(const PlaceholderVisual &p_visual) noexcept;
	[[nodiscard]] float toWorldScale(const PlaceholderVisual &p_visual) noexcept;
	[[nodiscard]] PlaceholderVisual fallbackPlaceholderVisual(std::string_view p_semanticId) noexcept;

	// p_renderedHeight is the already-scaled mesh height.  localSupport is always board-local;
	// BoardData applies its single source-specific translation exactly once.
	[[nodiscard]] spk::Vector3 battleUnitCenterPosition(
		const BoardData &p_board,
		const BoardCell &p_localSupport,
		float p_renderedHeight);

	// Like battleUnitCenterPosition, but samples a shaped movement segment in local board space.
	[[nodiscard]] spk::Vector3 battleUnitSegmentCenterPosition(
		const BoardData &p_board,
		const BoardCell &p_from,
		const BoardCell &p_to,
		float p_progress,
		float p_renderedHeight);
}
