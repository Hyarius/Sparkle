#include <gtest/gtest.h>

#include "core/paths.hpp"
#include "voxel/shape_catalog.hpp"
#include "voxel/voxel_family_definition.hpp"
#include "voxel/voxel_registry.hpp"
#include "world/voxel_world.hpp"

#include "structures/voxel/spk_voxel_ray_cast.hpp"

#include <filesystem>
#include <string_view>

TEST(VoxelRayCastPlaygroundPolicy, IgnoresPassableVoxelsAndSelectsSolidVoxels)
{
	pg::ShapeCatalog shapes;
	spk::loadJsonDirectory(
		shapes,
		pg::resourceRoot() / "data" / "shapes",
		[](std::string_view p_id, pg::JsonReader &p_reader) {
			pg::ShapeDefinition definition = pg::parseShapeDefinition(p_reader);
			definition.id = p_id;
			return definition;
		});
	pg::VoxelRegistry registry;
	pg::VoxelFamilyCatalog families;
	spk::loadJsonDirectory(
		families,
		pg::resourceRoot() / "data" / "voxel_families",
		[&shapes](std::string_view p_id, pg::JsonReader &p_reader) {
			pg::VoxelFamilyDefinition definition = pg::parseVoxelFamilyDefinition(p_reader, shapes);
			definition.id = p_id;
			return definition;
		});
	registry.load(shapes, families, pg::resourceRoot() / "data" / "voxels");
	pg::VoxelWorld world(registry, [](spk::VoxelChunk &) {
	});
	world.loadChunk({0, 0, 0});
	ASSERT_TRUE(world.setCell({1, 0, 0}, {.id = registry.numericId("bush")}));
	ASSERT_TRUE(world.setCell({2, 0, 0}, {.id = registry.numericId("stone-block")}));

	const auto hit = spk::VoxelRayCast::cast(
		world.map(),
		{{0.5f, 0.5f, 0.5f}, {1, 0, 0}},
		4.0f,
		[&world](const spk::Vector3Int &, const spk::VoxelCell &p_cell) {
			const pg::VoxelDefinition *definition = world.tryDefinition(p_cell);
			return definition != nullptr && definition->data.traversal == pg::VoxelTraversal::Solid;
		});
	ASSERT_TRUE(hit.has_value());
	EXPECT_EQ(hit->cell, spk::Vector3Int(2, 0, 0));
}
