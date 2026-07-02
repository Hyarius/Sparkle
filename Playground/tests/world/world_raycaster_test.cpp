#include "components/camera3d.hpp"
#include "rendering/mouse_picker.hpp"
#include "world/chunk_provider.hpp"
#include "world/voxel_world.hpp"
#include "world/world_navigation.hpp"
#include "world/world_raycaster.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <memory>

namespace
{
	class EmptyProvider final : public pg::IChunkProvider
	{
	public:
		void fill(pg::Chunk &p_chunk) const override { p_chunk.requestSynchronization(); }
	};

	[[nodiscard]] const pg::VoxelRegistry &registry()
	{
		static const pg::VoxelRegistry result = [] {
			pg::VoxelRegistry loaded;
			loaded.load(std::filesystem::path(PG_RESOURCE_DIR) / "data" / "voxels");
			return loaded;
		}();
		return result;
	}

	[[nodiscard]] std::unique_ptr<pg::VoxelWorld> worldWithBlock(const spk::Vector3Int &p_position)
	{
		auto world = std::make_unique<pg::VoxelWorld>(registry());
		static EmptyProvider provider;
		(void)world->loadChunk({{0, 0, 0}}, provider);
		(void)world->setCell(p_position, {registry().numericId("stone-block")});
		return world;
	}
}

TEST(WorldRaycaster, HitsAxisAlignedVoxelWithEntryFace)
{
	auto world = worldWithBlock({2, 2, 2});
	const auto hit = pg::WorldRaycaster::raycast(*world, {{0.5f, 2.5f, 2.5f}, {1, 0, 0}}, 10);
	ASSERT_TRUE(hit.has_value());
	EXPECT_EQ(hit->cell, spk::Vector3Int(2, 2, 2));
	EXPECT_EQ(hit->enterFace, pg::VoxelAxisPlane::NegativeX);
	EXPECT_NEAR(hit->distance, 1.5f, 0.0001f);
}

TEST(WorldRaycaster, HitsDiagonalAndDetectsOriginInsideSolid)
{
	auto world = worldWithBlock({2, 2, 2});
	const auto diagonal = pg::WorldRaycaster::raycast(*world, {{0.5f, 2.5f, 0.5f}, {1, 0, 1}}, 10);
	ASSERT_TRUE(diagonal.has_value());
	EXPECT_EQ(diagonal->cell, spk::Vector3Int(2, 2, 2));

	const auto inside = pg::WorldRaycaster::raycast(*world, {{2.5f, 2.5f, 2.5f}, {0, 1, 0}}, 10);
	ASSERT_TRUE(inside.has_value());
	EXPECT_EQ(inside->distance, 0);
	EXPECT_EQ(inside->enterFace, pg::VoxelAxisPlane::Count);
}

TEST(WorldRaycaster, ReturnsEmptyForMiss)
{
	auto world = worldWithBlock({2, 2, 2});
	EXPECT_FALSE(pg::WorldRaycaster::raycast(*world, {{0.5f, 8.5f, 0.5f}, {1, 0, 0}}, 10).has_value());
}

TEST(MousePicker, CenterPixelRayPassesThroughCameraTarget)
{
	pg::Camera3D camera;
	camera.setPerspective(60, 0.1f, 100);
	camera.setViewportSize(800, 600);
	camera.setPosition({0, 0, -5});
	camera.setTarget({0, 0, 0});
	const pg::WorldRay ray = pg::MousePicker::screenToRay(camera, {800, 600}, {400, 300});

	EXPECT_NEAR(ray.direction.x, 0, 0.0001f);
	EXPECT_NEAR(ray.direction.y, 0, 0.0001f);
	EXPECT_NEAR(ray.direction.z, 1, 0.0001f);
	EXPECT_LT((ray.origin + ray.direction * 5.0f).distance({0, 0, 0}), 0.001f);
}

TEST(MousePicker, DescendsHitColumnToStandableSurface)
{
	pg::VoxelWorld world(registry());
	EmptyProvider provider;
	(void)world.loadChunk({{0, 0, 0}}, provider);
	(void)world.setCell({2, 0, 2}, {registry().numericId("stone-block")});
	pg::WorldNavigation navigation(world, {{0, 0, 0}, {16, 16, 16}});
	const auto picked = pg::MousePicker::pickStandable(world, navigation, {{2.5f, 8, 2.5f}, {0, -1, 0}});

	ASSERT_TRUE(picked.has_value());
	EXPECT_EQ(*picked, spk::Vector3Int(2, 0, 2));
}
