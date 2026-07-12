#pragma once

#include "core/json.hpp"

#include "structures/voxel/spk_voxel_ids.hpp"

#include <optional>
#include <string>
#include <vector>

namespace pg
{
	struct WeightedId
	{
		std::optional<std::string> id;
		// Voxel entries may select a non-default state of their type ({"voxel": "bush",
		// "state": 1}); a bare id string is shorthand for state 0. Domains whose ids are
		// not voxels must reject entries that specify a state.
		std::optional<spk::VoxelStateId> state;
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
