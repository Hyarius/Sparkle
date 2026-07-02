#pragma once

#include "voxel/voxel_enums.hpp"

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>
#include <vector>

namespace pg
{
	struct VoxelData
	{
		VoxelTraversal traversal = VoxelTraversal::Solid;
		std::vector<std::string> tags;

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
