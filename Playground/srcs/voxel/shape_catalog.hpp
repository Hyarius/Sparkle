#pragma once

#include "core/json.hpp"
#include "core/registry.hpp"
#include "voxel/voxel_traversal_data.hpp"

#include "structures/voxel/spk_data_voxel_shape.hpp"

#include <memory>
#include <set>
#include <string>

namespace pg
{
	// One data-driven voxel shape, authored in resources/data/shapes/*.json: the polygon
	// geometry consumed by spk::DataVoxelShape plus the walk heights the navigation graph
	// needs. Heights are authored explicitly next to the geometry (never derived from it),
	// and outer/inner face classification is computed by spk::DataVoxelShape, so a shape
	// file is nothing but polygons and gameplay numbers.
	//
	// Voxel definitions reference a shape by its file stem and supply one atlas cell per
	// texture slot the polygons use; `slots` records that set for exact validation.
	// A ShapeDefinition is a factory: instantiate() builds the final render shape by
	// binding the shared geometry to one voxel's texture cells.
	struct ShapeDefinition
	{
		std::string id;
		spk::VoxelShapeDescription description;
		CardinalHeightCollection heights;
		std::set<std::string> slots;

		[[nodiscard]] std::unique_ptr<spk::VoxelShape> instantiate(spk::VoxelShape::TextureSlots p_textures) const;
	};

	[[nodiscard]] ShapeDefinition parseShapeDefinition(JsonReader &p_reader);

	// The shape half of the voxel catalog, loaded from resources/data/shapes and passed to
	// VoxelRegistry::load so every voxel can instantiate its referenced geometry.
	using ShapeCatalog = Registry<ShapeDefinition>;
}
