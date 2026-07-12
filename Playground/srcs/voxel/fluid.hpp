#pragma once

#include "structures/voxel/spk_voxel_ids.hpp"
#include "structures/voxel/spk_voxel_shape.hpp" // spk::AtlasCell

#include <cstddef>
#include <vector>

namespace pg
{
	// Emitted by a voxel that declares a "fluid" block. A fluid is a model in its own right
	// (a stair or a slope can never be a fluid), so this data rides on the voxel definition.
	//
	// The stage count equals maxSpread - the material's "viscosity": a fluid that can only
	// reach N cells from its source has N fill stages, from a nearly-full cube (stage N,
	// next to the source) down to a thin film (stage 1, at the flow's edge). A lower
	// maxSpread => thicker, less mobile fluid.
	struct FluidData
	{
		int maxSpread = 8;        // stage count == how many cells the fluid flows from a source
		bool falls = true;        // whether the fluid forms falling columns over empty space
		spk::AtlasCell texture{}; // atlas cell reused for every generated fill-stage slab
	};

	// One state of a fluid resolved to its full identity: the shared semantic type, the
	// state inside it and the dense runtime handle the world stores.
	struct FluidStateRef
	{
		spk::VoxelTypeId type{};
		spk::VoxelStateId state{};
		spk::VoxelRuntimeId runtime{};

		std::size_t level = 0; // fill level; the source reports maxSpread (full strength)
		bool source = false;
	};

	// A fluid resolved to ONE semantic voxel type: state 0 is the authored source, states
	// 1..maxSpread the generated flow levels (level k renders at height k / maxSpread).
	// The source and the strongest flow level stay distinct states even though both use
	// full-height geometry. Filled in during VoxelRegistry::load.
	struct FluidFamily
	{
		spk::VoxelTypeId type{};

		FluidStateRef source;
		std::vector<FluidStateRef> levels; // levels[k - 1] = flow level k (1..maxSpread)

		int maxSpread = 8;
		bool falls = true;

		[[nodiscard]] spk::VoxelRuntimeId stageId(int p_stage) const
		{
			return levels[static_cast<std::size_t>(p_stage - 1)].runtime;
		}
	};

	// Where a given runtime id sits inside its fluid family, for O(1) lookups during
	// simulation.
	struct FluidRef
	{
		std::size_t family = 0; // index into VoxelRegistry::fluids()
		int stage = 0;          // fill level of this state; a source reports stage == maxSpread
		bool source = false;    // true for the authored source block (persistent, infinite)
	};
}
