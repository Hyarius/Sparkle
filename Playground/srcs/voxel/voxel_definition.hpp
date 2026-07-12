#pragma once

#include "core/json.hpp"
#include "voxel/shape_catalog.hpp"
#include "voxel/voxel_data.hpp"
#include "voxel/voxel_traversal_data.hpp"

#include "structures/voxel/spk_voxel_fluid.hpp"
#include "structures/voxel/spk_voxel_ids.hpp"
#include "structures/voxel/spk_voxel_shape.hpp"

#include <memory>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

namespace pg
{
	// One state of a semantic voxel type: only what can legitimately differ between states
	// lives here - the dense runtime handle the world stores, the walk heights and an
	// optional authored name. Gameplay identity (traversal, tags) stays on the type.
	struct VoxelStateDefinition
	{
		spk::VoxelStateId id{};
		spk::VoxelRuntimeId runtimeId{};

		std::string name;

		CardinalHeightCollection heights;
	};

	// Gameplay-side description of one semantic voxel type. The render shapes stay owned
	// by the spk::VoxelRegistry; this carries the data the navigation graph and gameplay
	// queries need. States are contiguous from 0 and state 0 is the default state.
	struct VoxelDefinition
	{
		std::string id;
		spk::VoxelTypeId typeId{};

		VoxelData data;

		std::vector<VoxelStateDefinition> states;

		[[nodiscard]] const VoxelStateDefinition *tryState(spk::VoxelStateId p_state) const noexcept
		{
			return p_state.index() < states.size() ? &states[p_state.index()] : nullptr;
		}

		[[nodiscard]] const VoxelStateDefinition &state(spk::VoxelStateId p_state) const
		{
			const VoxelStateDefinition *result = tryState(p_state);
			if (result == nullptr)
			{
				throw std::out_of_range(
					"voxel '" + id + "' has no state " + std::to_string(p_state.value));
			}
			return *result;
		}

		[[nodiscard]] const VoxelStateDefinition &defaultState() const
		{
			return state(spk::VoxelStateId{});
		}
	};

	// One parsed authored state: its explicit id, optional name, the render shape
	// instantiated from the shared shape definition plus this state's textures, and the
	// walk heights copied from that shared shape definition.
	struct ParsedVoxelState
	{
		spk::VoxelStateId id{};
		std::string name;
		std::unique_ptr<spk::VoxelShape> shape;
		CardinalHeightCollection heights;
	};

	// An ordinary voxel's rendering: its authored states, instantiated from the shared
	// shape definition.
	struct ParsedRegularVoxel
	{
		std::vector<ParsedVoxelState> states;
	};

	// A fluid voxel's rendering: nothing is authored beyond the description handed to
	// spk::VoxelRegistry::registerFluid, which generates the source and flow-level states.
	struct ParsedFluidVoxel
	{
		spk::VoxelFluidDescription description;
	};

	// One parsed JSON voxel: the shared gameplay data plus its rendering, which is exactly
	// one of ordinary authored states or a generated fluid (never both). VoxelRegistry
	// registers either as one spk voxel type and keeps the definitions on its side.
	//
	// Version 2 files author a single top-level "textures" block and become one state
	// (id 0, name "default"); version 3 files author an explicit "states" array, or a
	// "fluid" block with type-level "textures" (top/bottom/side).
	struct ParsedVoxel
	{
		std::string id;
		VoxelData data;

		std::variant<ParsedRegularVoxel, ParsedFluidVoxel> rendering;
	};

	[[nodiscard]] ParsedVoxel parseVoxelDefinition(JsonReader &p_reader, const ShapeCatalog &p_shapes);
}
