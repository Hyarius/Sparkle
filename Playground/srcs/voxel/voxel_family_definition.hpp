#pragma once

#include "core/json.hpp"
#include "core/registry.hpp"
#include "voxel/shape_catalog.hpp"

#include <map>
#include <string>
#include <vector>

namespace pg
{
	// Playground content recipe that expands one authored material into several
	// Sparkle voxel states. Texture mappings are target-slot -> base-material-slot.
	struct VoxelFamilyVariant
	{
		std::string name;
		std::string shapeId;
		std::map<std::string, std::string> textureMapping;
	};

	struct VoxelFamilyDefinition
	{
		std::string id;
		std::string baseShapeId;
		std::vector<VoxelFamilyVariant> variants;
	};

	using VoxelFamilyCatalog = Registry<VoxelFamilyDefinition>;

	[[nodiscard]] VoxelFamilyDefinition parseVoxelFamilyDefinition(
		JsonReader &p_reader,
		const ShapeCatalog &p_shapes);
}
