#pragma once

#include "core/json.hpp"
#include "voxel/fluid.hpp"
#include "voxel/shape_catalog.hpp"
#include "voxel/voxel_data.hpp"
#include "voxel/voxel_traversal_data.hpp"

#include "structures/voxel/spk_voxel_ids.hpp"
#include "structures/voxel/spk_voxel_shape.hpp"

#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
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

	// One parsed JSON voxel: the shared gameplay data plus one render shape per authored
	// state. VoxelRegistry registers all state shapes as one spk voxel type and keeps the
	// definitions on its side.
	//
	// Version 2 files author a single top-level "textures" block and become one state
	// (id 0, name "default"); version 3 files author an explicit "states" array.
	struct ParsedVoxel
	{
		std::string id;
		VoxelData data;
		std::vector<ParsedVoxelState> states;

		// Keep the existing parsed fluid data until the dedicated Sparkle fluid migration
		// is applied (fluids are authored as version 2 single-state voxels meanwhile).
		std::optional<FluidData> fluid;
	};

	[[nodiscard]] ParsedVoxel parseVoxelDefinition(JsonReader &p_reader, const ShapeCatalog &p_shapes);
}
