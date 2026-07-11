#pragma once

#include "core/json.hpp"
#include "voxel/fluid.hpp"
#include "voxel/voxel_data.hpp"
#include "voxel/voxel_traversal_data.hpp"

#include "structures/voxel/spk_voxel_shape.hpp"

#include <memory>
#include <optional>
#include <string>

namespace pg
{

	// Gameplay-side description of a voxel type, keyed by the same numeric id the spk render
	// registry uses. It no longer owns render geometry (that lives in the spk::VoxelShape held
	// by the spk::VoxelRegistry) - only the data the navigation graph needs.
	struct VoxelDefinition
	{
		std::string id;
		VoxelData data;
		CardinalHeightCollection heights;
	};

	// One parsed JSON voxel: the spk render shape plus the gameplay definition. VoxelRegistry
	// splits it into the spk::VoxelRegistry (shape) and its own catalog (data + heights), and,
	// when the shape is a fluid, generates its fill-stage slabs from the fluid metadata below.
	struct ParsedVoxel
	{
		std::string id;
		VoxelData data;
		CardinalHeightCollection heights;
		std::unique_ptr<spk::VoxelShape> shape;
		std::optional<FluidData> fluid;
	};

	[[nodiscard]] ParsedVoxel parseVoxelDefinition(JsonReader &p_reader);
}
