#include "board/board_builder.hpp"
#include "support/board_fixture.hpp"
#include "world/map_definition.hpp"
#include "world/voxel_world.hpp"

#include <gtest/gtest.h>

#include <memory>

namespace
{
	[[nodiscard]] pg::VoxelCell ground()
	{
		return {pg::test::BoardFixture::registry().numericId("stone-block")};
	}

	[[nodiscard]] std::unique_ptr<pg::VoxelWorld> worldFrom(pg::VoxelGrid p_grid)
	{
		pg::MapDefinition map{.grid = std::move(p_grid)};
		auto world = std::make_unique<pg::VoxelWorld>(pg::test::BoardFixture::registry());
		world->loadFromMap(map);
		return world;
	}

	[[nodiscard]] pg::VoxelGrid flatGrid(int p_size = 16)
	{
		pg::VoxelGrid grid({p_size, 4, p_size});
		for (int z = 0; z < p_size; ++z)
		{
			for (int x = 0; x < p_size; ++x)
			{
				grid.cell(x, 0, z) = ground();
			}
		}
		return grid;
	}
}

TEST(BoardBuilder, DerivesZeroCopyWorldRegionWithCenteredAndClampedAnchor)
{
	auto world = worldFrom(flatGrid());
	const std::size_t revision = world->revision();
	const pg::BoardData centered = pg::BoardBuilder::fromWorld(*world, {8, 0, 8}, {11, 11});

	EXPECT_EQ(centered.worldAnchor(), spk::Vector3Int(3, 0, 3));
	EXPECT_EQ(centered.terrain().size(), spk::Vector3Int(11, 16, 11));
	EXPECT_EQ(centered.navigation().graph().size(), 121);
	EXPECT_EQ(world->revision(), revision);
	EXPECT_EQ(
		centered.terrain().cells().cell({0, 0, 0}),
		world->cell(centered.worldAnchor()));
	EXPECT_TRUE(world->setCell(centered.worldAnchor(), {}));
	EXPECT_TRUE(centered.terrain().cells().cell({0, 0, 0}).isEmpty());

	const pg::BoardData edge = pg::BoardBuilder::fromWorld(*world, {0, 0, 0}, {11, 11});
	EXPECT_EQ(edge.worldAnchor(), spk::Vector3Int(0, 0, 0));
}

TEST(BoardBuilder, ComputesBorderRingAndOpposingDeploymentStrips)
{
	auto world = worldFrom(flatGrid());
	const pg::BoardData board = pg::BoardBuilder::fromWorld(
		*world, {8, 0, 8}, {11, 11}, 2, 6, pg::VoxelOrientation::PositiveZ);

	EXPECT_EQ(board.borderCells().size(), 40);
	EXPECT_EQ(board.deploymentZones().player.size(), 22);
	EXPECT_EQ(board.deploymentZones().enemy.size(), 22);
	EXPECT_TRUE(board.isBorderCell({0, 0, 0}));
	EXPECT_FALSE(board.isBorderCell({5, 0, 5}));
	for (const spk::Vector3Int &cell : board.deploymentZones().player)
	{
		EXPECT_GE(cell.z, 9);
	}
	for (const spk::Vector3Int &cell : board.deploymentZones().enemy)
	{
		EXPECT_LT(cell.z, 2);
	}
}

TEST(BoardBuilder, NudgesRectangleToFindValidDeploymentTerrain)
{
	pg::VoxelGrid grid({16, 4, 16});
	for (int z = 0; z < 16; ++z)
	{
		for (int x = 7; x < 16; ++x)
		{
			grid.cell(x, 0, z) = ground();
		}
	}
	auto world = worldFrom(std::move(grid));

	const pg::BoardData board = pg::BoardBuilder::fromWorld(
		*world, {8, 0, 8}, {7, 7}, 1, 5, pg::VoxelOrientation::PositiveX);
	EXPECT_EQ(board.worldAnchor().x, 7);
	EXPECT_EQ(board.terrain().size().x, 7);
}

TEST(BoardBuilder, ShrinksOnPathologicalTerrain)
{
	pg::VoxelGrid grid({16, 4, 16});
	for (int z = 6; z <= 10; ++z)
	{
		for (int x = 6; x <= 10; ++x)
		{
			grid.cell(x, 0, z) = ground();
		}
	}
	auto world = worldFrom(std::move(grid));

	const pg::BoardData board = pg::BoardBuilder::fromWorld(
		*world, {8, 0, 8}, {11, 11}, 2, 6, pg::VoxelOrientation::PositiveZ);
	EXPECT_EQ(board.terrain().size().x, 5);
	EXPECT_EQ(board.terrain().size().z, 5);
	EXPECT_EQ(board.worldAnchor(), spk::Vector3Int(6, 0, 6));
}
