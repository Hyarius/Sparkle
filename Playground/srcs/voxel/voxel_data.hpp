#pragma once

#include "voxel/voxel_enums.hpp"

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>
#include <vector>

namespace pg
{
	// The authored bounds of a voxel's terrain movement cost. Optional, defaulting to 1, so every
	// voxel file written before battle movement existed stays valid and stays free to walk on.
	inline constexpr int MinimumVoxelMovementCost = 1;
	inline constexpr int MaximumVoxelMovementCost = 1000000;

	struct VoxelData
	{
		VoxelTraversal traversal = VoxelTraversal::Solid;
		std::vector<std::string> tags;

		// What entering the standable cell supported by this voxel costs a battle unit: mud is
		// explicitly 2, a slope still costs whatever it was authored to cost. It is type-level, so
		// every state and orientation of the voxel shares it, and it is never derived from a tag, a
		// transparency, a slope height or a render shape.
		//
		// Only the solid support voxel is charged. A passable voxel (the air, the bush, the water a
		// unit stands in) never contributes: the cost of a cell is the cost of the ground under it.
		int movementCost = MinimumVoxelMovementCost;

		[[nodiscard]] bool hasTag(std::string_view p_tag) const
		{
			const auto normalize = [](std::string_view p_value) {
				const std::size_t first = p_value.find_first_not_of(" \t\r\n");
				if (first == std::string_view::npos)
				{
					return std::string{};
				}
				const std::size_t last = p_value.find_last_not_of(" \t\r\n");
				std::string result(p_value.substr(first, last - first + 1));
				std::transform(result.begin(), result.end(), result.begin(), [](unsigned char p_character) {
					return static_cast<char>(std::tolower(p_character));
				});
				return result;
			};

			const std::string expected = normalize(p_tag);
			if (expected.empty())
			{
				return false;
			}
			return std::ranges::any_of(tags, [&normalize, &expected](const std::string &p_candidate) {
				return normalize(p_candidate) == expected;
			});
		}
	};
}
