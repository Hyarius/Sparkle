#pragma once

#include "structures/voxel/spk_voxel_shape.hpp" // spk::AtlasCell

#include <cstddef>
#include <cstdint>
#include <vector>

namespace pg
{
	// Emitted by a voxel whose shape "type" is "fluid". A fluid is a model in its own right (a
	// stair or a slope can never be a fluid), so this data rides on the shape, not on a tag.
	//
	// The stage count equals maxSpread - the material's "viscosity": a fluid that can only reach
	// N cells from its source has N fill stages, from a nearly-full cube (stage N, next to the
	// source) down to a thin film (stage 1, at the flow's edge). A lower maxSpread => thicker,
	// less mobile fluid.
	struct FluidData
	{
		int maxSpread = 8;         // stage count == how many cells the fluid flows from a source
		bool falls = true;         // whether the fluid forms falling columns over empty space
		spk::AtlasCell texture{};  // atlas cell reused for every generated fill-stage slab
	};

	// A fluid resolved to numeric ids: the authored source block plus the fill-stage slabs the
	// registry generates from its FluidData. Filled in during VoxelRegistry::load.
	struct FluidFamily
	{
		std::int32_t sourceId = -1;         // the authored "fluid" voxel (a full, persistent source)
		int maxSpread = 8;
		bool falls = true;
		std::vector<std::int32_t> stageIds; // stageIds[k - 1] renders fill stage k (1..maxSpread)

		[[nodiscard]] std::int32_t stageId(int p_stage) const
		{
			return stageIds[static_cast<std::size_t>(p_stage - 1)];
		}
	};

	// Where a given cell id sits inside its fluid family, for O(1) lookups during simulation.
	struct FluidRef
	{
		std::size_t family = 0; // index into VoxelRegistry::fluids()
		int stage = 0;          // fill stage of this id; a source reports stage == maxSpread
		bool source = false;    // true for the authored source block (persistent, infinite)
	};
}
