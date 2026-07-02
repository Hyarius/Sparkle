#include "board/cell_source.hpp"
#include "board/pathfinder.hpp"
#include "board/traversal_graph_builder.hpp"

#include <gtest/gtest.h>

#include <filesystem>

namespace
{
	[[nodiscard]] const pg::VoxelRegistry &registry()
	{
		static const pg::VoxelRegistry result = [] {
			pg::VoxelRegistry loaded;
			loaded.load(std::filesystem::path(PG_RESOURCE_DIR) / "data" / "voxels");
			return loaded;
		}();
		return result;
	}

	[[nodiscard]] pg::VoxelCell voxel(
		const char *p_id,
		pg::VoxelOrientation p_orientation = pg::VoxelOrientation::PositiveZ)
	{
		return {registry().numericId(p_id), p_orientation};
	}

	[[nodiscard]] pg::TraversalGraph graphFor(const pg::VoxelGrid &p_grid)
	{
		pg::GridCellSource source(p_grid, registry());
		return pg::TraversalGraphBuilder::build(source, {{0, 0, 0}, p_grid.size()});
	}
}

TEST(TraversalGraph, ConnectsFlatStandableCells)
{
	pg::VoxelGrid grid({5, 3, 1});
	for (int x = 0; x < 5; ++x) grid.cell(x, 0, 0) = voxel("stone-block");
	const pg::TraversalGraph graph = graphFor(grid);

	EXPECT_EQ(graph.size(), 5);
	ASSERT_NE(graph.tryGetNode({2, 0, 0}), nullptr);
	EXPECT_TRUE(graph.tryGetNode({2, 0, 0})->neighbors[0].has_value());
	EXPECT_TRUE(graph.tryGetNode({2, 0, 0})->neighbors[1].has_value());
}

TEST(TraversalGraph, ConnectsSlopeChainAcrossVerticalLayers)
{
	pg::VoxelGrid grid({1, 7, 4});
	grid.cell(0, 0, 0) = voxel("slope-grass");
	grid.cell(0, 1, 1) = voxel("slope-grass");
	grid.cell(0, 2, 2) = voxel("slope-grass");
	grid.cell(0, 2, 3) = voxel("grass-block");
	const pg::TraversalGraph graph = graphFor(grid);
	const auto path = pg::Pathfinder::findPath(graph, {0, 0, 0}, {0, 2, 3});

	ASSERT_TRUE(path.has_value());
	EXPECT_EQ(path->size(), 4);
}

TEST(TraversalGraph, ConnectsStairsAndRejectsHeadBlockedCells)
{
	pg::VoxelGrid stairs({1, 6, 3});
	stairs.cell(0, 0, 0) = voxel("stair-stone");
	stairs.cell(0, 1, 1) = voxel("stair-stone");
	stairs.cell(0, 1, 2) = voxel("stone-block");
	EXPECT_TRUE(pg::Pathfinder::findPath(graphFor(stairs), {0, 0, 0}, {0, 1, 2}).has_value());

	pg::VoxelGrid blocked({2, 4, 1});
	blocked.cell(0, 0, 0) = voxel("stone-block");
	blocked.cell(1, 0, 0) = voxel("stone-block");
	blocked.cell(1, 1, 0) = voxel("stone-block");
	const pg::TraversalGraph blockedGraph = graphFor(blocked);
	EXPECT_NE(blockedGraph.tryGetNode({0, 0, 0}), nullptr);
	EXPECT_EQ(blockedGraph.tryGetNode({1, 0, 0}), nullptr);
}

TEST(Pathfinder, FindsShortestDetourAndReportsUnreachable)
{
	pg::VoxelGrid grid({5, 4, 3});
	for (int z = 0; z < 3; ++z)
		for (int x = 0; x < 5; ++x)
			grid.cell(x, 0, z) = voxel("stone-block");
	grid.cell(2, 1, 1) = voxel("wall-stone");
	grid.cell(2, 2, 1) = voxel("wall-stone");
	const pg::TraversalGraph graph = graphFor(grid);
	const auto path = pg::Pathfinder::findPath(graph, {0, 0, 1}, {4, 0, 1});

	ASSERT_TRUE(path.has_value());
	EXPECT_EQ(path->size(), 7);
	EXPECT_FALSE(pg::Pathfinder::findPath(graph, {0, 0, 1}, {2, 0, 1}).has_value());
}

TEST(Pathfinder, FloodReachableHonorsMaximumCost)
{
	pg::VoxelGrid grid({7, 3, 1});
	for (int x = 0; x < 7; ++x) grid.cell(x, 0, 0) = voxel("stone-block");
	const auto reachable = pg::Pathfinder::floodReachable(graphFor(grid), {3, 0, 0}, 2.0f);

	EXPECT_EQ(reachable.size(), 5);
	EXPECT_TRUE(reachable.contains({1, 0, 0}));
	EXPECT_TRUE(reachable.contains({5, 0, 0}));
	EXPECT_FALSE(reachable.contains({0, 0, 0}));
}
