#pragma once

#include "core/json.hpp"

#include <optional>
#include <string>
#include <vector>

namespace pg
{
	struct WeightedId
	{
		std::optional<std::string> id;
		double weight = 1.0;
		std::string path;
	};

	// Parses the weighted ID schema shared by biome palettes and voxel-content
	// palettes. Resolution is deliberately separate so the same IDs can become
	// biome strings, numeric VoxelCells, or another domain value.
	[[nodiscard]] std::vector<WeightedId> parseWeightedIds(
		const spk::JSON::Value &p_value,
		const JsonReader &p_reader,
		const std::string &p_path);
}
