#include <gtest/gtest.h>

#include "voxel/shape_catalog.hpp"
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
		std::filesystem::path(PG_RESOURCE_DIR) / "data" / "shapes",
		[](std::string_view p_id, pg::JsonReader &p_reader) {
			pg::ShapeDefinition definition = pg::parseShapeDefinition(p_reader);
			definition.id = p_id;
			return definition;
		});
	pg::VoxelRegistry registry;
	registry.load(shapes, std::filesystem::path(PG_RESOURCE_DIR) / "data" / "voxels");
	pg::VoxelWorld world(registry, [](spk::VoxelChunk &) {});
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
