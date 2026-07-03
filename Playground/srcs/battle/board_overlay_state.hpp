#pragma once

#include "structures/math/spk_vector3.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <utility>
#include <vector>

namespace pg
{
	// The battle overlay is decoupled: phases/input write board-local cells into these per-category
	// sets and bump a change counter; the view (BoardOverlayView + BoardOverlayLogic) rebuilds its
	// mask meshes only when the counter moved (D31, rendering-cameras.md §3). Plain data, headless
	// and unit-testable — the render layer never reaches into battle logic.
	enum class OverlayCategory : std::size_t
	{
		Deployment,	  // deployment strips (Placement phase)
		MoveRange,	  // reachable cells with current MP
		Path,		  // hovered movement path
		AbilityRange, // ability range, line of sight clear
		AreaOfEffect, // AoE footprint under the hovered anchor
		LosBlocked,	  // in range but blocked by terrain (shown instead of the range mask)
		Hovered,	  // the cell under the cursor
		Invalid,	  // an illegal target the cursor is over
		Count
	};

	inline constexpr std::size_t OverlayCategoryCount = static_cast<std::size_t>(OverlayCategory::Count);

	// The game-rules.json `overlayMasks` key each category resolves its atlas cell from.
	[[nodiscard]] inline std::string_view overlayMaskKey(OverlayCategory p_category) noexcept
	{
		switch (p_category)
		{
		case OverlayCategory::Deployment: return "deployment";
		case OverlayCategory::MoveRange: return "movement";
		case OverlayCategory::Path: return "path";
		case OverlayCategory::AbilityRange: return "abilityRange";
		case OverlayCategory::AreaOfEffect: return "areaOfEffect";
		case OverlayCategory::LosBlocked: return "losBlocked";
		case OverlayCategory::Hovered: return "hovered";
		case OverlayCategory::Invalid: return "invalid";
		case OverlayCategory::Count: break;
		}
		return "";
	}

	struct BoardOverlayState
	{
		std::array<std::vector<spk::Vector3Int>, OverlayCategoryCount> layers;
		// Starts at 1 so a freshly-bound view (counter 0) always rebuilds once.
		std::uint64_t changeCounter = 1;

		[[nodiscard]] std::vector<spk::Vector3Int> &cells(OverlayCategory p_category) noexcept
		{
			return layers[static_cast<std::size_t>(p_category)];
		}
		[[nodiscard]] const std::vector<spk::Vector3Int> &cells(OverlayCategory p_category) const noexcept
		{
			return layers[static_cast<std::size_t>(p_category)];
		}

		// Replace one category's cells and bump the counter (one edit == one rebuild).
		void set(OverlayCategory p_category, std::vector<spk::Vector3Int> p_cells)
		{
			cells(p_category) = std::move(p_cells);
			++changeCounter;
		}
		void clear(OverlayCategory p_category)
		{
			if (!cells(p_category).empty())
			{
				cells(p_category).clear();
				++changeCounter;
			}
		}
		void clearAll()
		{
			for (std::vector<spk::Vector3Int> &layer : layers)
			{
				layer.clear();
			}
			++changeCounter;
		}
		// Batch several edits, then bump once — avoids N rebuilds when input refreshes a preview.
		void touch() noexcept
		{
			++changeCounter;
		}
	};
}
