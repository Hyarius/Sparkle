#include <gtest/gtest.h>

#include "voxel/voxel_registry.hpp"
#include "world/voxel_world.hpp"
#include "world/world_raycaster.hpp"

#include <array>
#include <cmath>
#include <filesystem>
#include <limits>
#include <memory>
#include <string>

namespace
{
	class RaycastWorld
	{
	public:
		pg::VoxelRegistry registry;
		pg::VoxelWorld world;

		RaycastWorld() :
			world(registry, [](spk::VoxelChunk &) {})
		{
			registry.load(std::filesystem::path(PG_RESOURCE_DIR) / "data" / "voxels");
			world.loadChunk({0, 0, 0});
		}

		void place(
			const std::string &p_id,
			const spk::Vector3Int &p_position,
			spk::VoxelOrientation p_orientation = spk::VoxelOrientation::PositiveZ,
			spk::VoxelFlip p_flip = spk::VoxelFlip::PositiveY)
		{
			ASSERT_TRUE(world.setCell(
				p_position,
				spk::VoxelCell{registry.numericId(p_id), p_orientation, p_flip}));
		}
	};
}

TEST(WorldRaycaster, SlabUsesItsVisibleHalfAndReportsTheSurface)
{
	RaycastWorld test;
	test.place("slab-stone", {1, 0, 0});

	EXPECT_FALSE(pg::WorldRaycaster::raycast(test.world, pg::WorldRay{{0.25f, 0.75f, 0.5f}, {1, 0, 0}}, 3.0f));

	const auto side = pg::WorldRaycaster::raycast(
		test.world,
		pg::WorldRay{{0.25f, 0.25f, 0.5f}, {1, 0, 0}},
		3.0f);
	ASSERT_TRUE(side.has_value());
	EXPECT_EQ(side->cell, spk::Vector3Int(1, 0, 0));
	EXPECT_NEAR(side->distance, 0.75f, 0.0001f);
	EXPECT_EQ(side->enterFace, pg::VoxelAxisPlane::NegativeX);
	EXPECT_EQ(side->normal, spk::Vector3(-1, 0, 0));

	const auto top = pg::WorldRaycaster::raycast(
		test.world,
		pg::WorldRay{{1.5f, 2.0f, 0.5f}, {0, -1, 0}},
		3.0f);
	ASSERT_TRUE(top.has_value());
	EXPECT_NEAR(top->distance, 1.5f, 0.0001f);
	EXPECT_EQ(top->enterFace, pg::VoxelAxisPlane::PositiveY);
}

TEST(WorldRaycaster, SlopeOrientationAndVerticalFlipTransformPickingGeometry)
{
	struct Case
	{
		spk::VoxelOrientation orientation;
		spk::Vector2 horizontal;
	};
	const std::array cases = {
		Case{spk::VoxelOrientation::PositiveZ, {0.5f, 0.25f}},
		Case{spk::VoxelOrientation::PositiveX, {0.25f, 0.5f}},
		Case{spk::VoxelOrientation::NegativeZ, {0.5f, 0.75f}},
		Case{spk::VoxelOrientation::NegativeX, {0.75f, 0.5f}}};

	for (const Case &entry : cases)
	{
		for (const spk::VoxelFlip flip : {spk::VoxelFlip::PositiveY, spk::VoxelFlip::NegativeY})
		{
			RaycastWorld test;
			test.place("slope-grass", {1, 0, 1}, entry.orientation, flip);
			const bool inverted = flip == spk::VoxelFlip::NegativeY;
			const pg::WorldRay ray{
				{1.0f + entry.horizontal.x, inverted ? -1.0f : 2.0f, 1.0f + entry.horizontal.y},
				{0, inverted ? 1.0f : -1.0f, 0}};
			const auto hit = pg::WorldRaycaster::raycast(test.world, ray, 4.0f);
			ASSERT_TRUE(hit.has_value());
			EXPECT_EQ(hit->cell, spk::Vector3Int(1, 0, 1));
			EXPECT_EQ(hit->enterFace, pg::VoxelAxisPlane::Count);
			EXPECT_NEAR(hit->distance, 1.75f, 0.0002f);
			EXPECT_NEAR(std::abs(hit->normal.y), std::sqrt(0.5f), 0.0002f);
			EXPECT_EQ(hit->normal.y < 0, inverted);
		}
	}
}

TEST(WorldRaycaster, StairAndCuboidVoidsDoNotBehaveLikeCubes)
{
	RaycastWorld test;
	test.place("stair-stone", {1, 0, 0});
	test.place("oak-trunk", {3, 0, 0});

	EXPECT_FALSE(pg::WorldRaycaster::raycast(test.world, pg::WorldRay{{0.0f, 0.75f, 0.25f}, {1, 0, 0}}, 2.9f));

	const auto lowerTread = pg::WorldRaycaster::raycast(
		test.world,
		pg::WorldRay{{1.5f, 2.0f, 0.25f}, {0, -1, 0}},
		3.0f);
	ASSERT_TRUE(lowerTread.has_value());
	EXPECT_NEAR(lowerTread->distance, 1.5f, 0.0001f);

	EXPECT_FALSE(pg::WorldRaycaster::raycast(test.world, pg::WorldRay{{2.0f, 0.5f, 0.1f}, {1, 0, 0}}, 3.0f));
	const auto trunk = pg::WorldRaycaster::raycast(
		test.world,
		pg::WorldRay{{2.0f, 0.5f, 0.5f}, {1, 0, 0}},
		3.0f);
	ASSERT_TRUE(trunk.has_value());
	EXPECT_EQ(trunk->cell, spk::Vector3Int(3, 0, 0));
	EXPECT_NEAR(trunk->distance, 1.31f, 0.0002f);
}

TEST(WorldRaycaster, StartsInsideAtTheNearestExitAndAcceptsShapeBoundaries)
{
	RaycastWorld test;
	test.place("slab-stone", {1, 0, 0});

	const auto inside = pg::WorldRaycaster::raycast(
		test.world,
		pg::WorldRay{{1.5f, 0.25f, 0.5f}, {1, 0, 0}},
		2.0f);
	ASSERT_TRUE(inside.has_value());
	EXPECT_NEAR(inside->distance, 0.5f, 0.0001f);
	EXPECT_EQ(inside->normal, spk::Vector3(1, 0, 0));

	const auto boundary = pg::WorldRaycaster::raycast(
		test.world,
		pg::WorldRay{{1.5f, 0.5f, 0.5f}, {0, -1, 0}},
		2.0f);
	ASSERT_TRUE(boundary.has_value());
	EXPECT_FLOAT_EQ(boundary->distance, 0.0f);
	EXPECT_EQ(boundary->normal, spk::Vector3(0, 1, 0));

	EXPECT_FALSE(pg::WorldRaycaster::raycast(test.world, pg::WorldRay{{1.5f, 0.75f, 0.5f}, {1, 0, 0}}, 2.0f));
}

TEST(WorldRaycaster, GridTiesAdvanceAllAxesWithoutTestingZeroVolumeCells)
{
	RaycastWorld test;
	test.place("stone-block", {1, 0, 0});
	test.place("stone-block", {0, 1, 0});
	test.place("stone-block", {1, 1, 0});

	const auto edge = pg::WorldRaycaster::raycast(
		test.world,
		pg::WorldRay{{0.5f, 0.5f, 0.5f}, {1, 1, 0}},
		4.0f);
	ASSERT_TRUE(edge.has_value());
	EXPECT_EQ(edge->cell, spk::Vector3Int(1, 1, 0));
	EXPECT_NEAR(edge->distance, std::sqrt(0.5f), 0.0002f);

	test.place("stone-block", {1, 1, 1});
	const auto corner = pg::WorldRaycaster::raycast(
		test.world,
		pg::WorldRay{{0.5f, 0.5f, 0.5f}, {1, 1, 1}},
		4.0f);
	ASSERT_TRUE(corner.has_value());
	EXPECT_EQ(corner->cell, spk::Vector3Int(1, 1, 1));
	EXPECT_NEAR(corner->distance, std::sqrt(0.75f), 0.0002f);
}

TEST(WorldRaycaster, ValidatesInputsAndHonorsMaximumDistance)
{
	RaycastWorld test;
	test.place("stone-block", {2, 0, 0});

	EXPECT_FALSE(pg::WorldRaycaster::raycast(test.world, {{0, 0.5f, 0.5f}, {1, 0, 0}}, 1.99f));
	EXPECT_TRUE(pg::WorldRaycaster::raycast(test.world, {{0, 0.5f, 0.5f}, {1, 0, 0}}, 2.0f));
	EXPECT_FALSE(pg::WorldRaycaster::raycast(test.world, {{0, 0, 0}, {1, 0, 0}}, -1.0f));
	EXPECT_THROW(pg::WorldRaycaster::raycast(test.world, {{0, 0, 0}, {0, 0, 0}}, 1.0f), std::invalid_argument);
	EXPECT_THROW(pg::WorldRaycaster::raycast(test.world, {{std::numeric_limits<float>::quiet_NaN(), 0, 0}, {1, 0, 0}}, 1.0f), std::invalid_argument);
	EXPECT_THROW(pg::WorldRaycaster::raycast(test.world, {{0, 0, 0}, {std::numeric_limits<float>::infinity(), 0, 0}}, 1.0f), std::invalid_argument);
	EXPECT_THROW(pg::WorldRaycaster::raycast(test.world, {{0, 0, 0}, {1, 0, 0}}, std::numeric_limits<float>::infinity()), std::invalid_argument);
	EXPECT_THROW(pg::WorldRaycaster::raycast(test.world, {{std::numeric_limits<float>::max(), 0, 0}, {1, 0, 0}}, 1.0f), std::out_of_range);
}
