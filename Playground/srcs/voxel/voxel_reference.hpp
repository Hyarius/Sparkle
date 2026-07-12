#pragma once

#include "structures/voxel/spk_voxel_ids.hpp"

#include <string>

namespace pg
{
	// An authored reference to one voxel state: the semantic string id plus the state to
	// use. JSON accepts the bare string form ("bush") as shorthand for the default state 0;
	// systems that need another state write {"voxel": "bush", "state": 1} in weighted
	// pools (see weighted_id_parser). Runtime ids are resolved after registries load -
	// authored data never stores them.
	struct VoxelReference
	{
		std::string id;
		spk::VoxelStateId state{};

		[[nodiscard]] bool operator==(const VoxelReference &) const = default;
	};
}
