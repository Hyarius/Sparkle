#pragma once

#include "voxel/voxel_data.hpp"
#include "voxel/voxel_shape.hpp"

#include <memory>
#include <string>

namespace pg
{
	class JsonReader;

	struct VoxelDefinition
	{
		std::string id;
		VoxelData data;
		std::unique_ptr<VoxelShape> shape;
	};

	[[nodiscard]] VoxelDefinition parseVoxelDefinition(JsonReader &p_reader);
}
