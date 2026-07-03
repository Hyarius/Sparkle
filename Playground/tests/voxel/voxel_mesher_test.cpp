#include "voxel/voxel_mesher.hpp"
#include "world/showcase.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <vector>

namespace
{
	[[nodiscard]] pg::VoxelRegistry loadRegistry()
	{
		pg::VoxelRegistry registry;
		registry.load(std::filesystem::path(PG_RESOURCE_DIR) / "data" / "voxels");
		return registry;
	}

	[[nodiscard]] pg::VoxelCell cell(
		const pg::VoxelRegistry &p_registry,
		const char *p_id,
		pg::VoxelOrientation p_orientation = pg::VoxelOrientation::PositiveZ,
		pg::VoxelFlip p_flip = pg::VoxelFlip::PositiveY)
	{
		return {p_registry.numericId(p_id), p_orientation, p_flip};
	}

	[[nodiscard]] spk::Vector3Int direction(pg::VoxelOrientation p_orientation)
	{
		switch (p_orientation)
		{
		case pg::VoxelOrientation::PositiveX:
			return {1, 0, 0};
		case pg::VoxelOrientation::PositiveZ:
			return {0, 0, 1};
		case pg::VoxelOrientation::NegativeX:
			return {-1, 0, 0};
		case pg::VoxelOrientation::NegativeZ:
			return {0, 0, -1};
		}
		return {};
	}
}

TEST(VoxelMesher, LoneCubeContributesSixFaces)
{
	const pg::VoxelRegistry registry = loadRegistry();
	pg::VoxelGrid grid({1, 1, 1}, cell(registry, "stone-block"));
	const spk::TextureMesh3D mesh = pg::VoxelMesher::buildRenderMesh(grid, registry);

	EXPECT_EQ(mesh.nbShape(), 6);
	EXPECT_EQ(mesh.indexes().size(), 36);
}

TEST(VoxelMesher, AdjacentCubesCullTheirSharedFaces)
{
	const pg::VoxelRegistry registry = loadRegistry();
	pg::VoxelGrid grid({2, 1, 1}, cell(registry, "stone-block"));
	const spk::TextureMesh3D mesh = pg::VoxelMesher::buildRenderMesh(grid, registry);

	EXPECT_EQ(mesh.nbShape(), 10);
}

TEST(VoxelMesher, FullyBuriedCubeContributesNoGeometry)
{
	const pg::VoxelRegistry registry = loadRegistry();
	pg::VoxelGrid grid({3, 3, 3}, cell(registry, "stone-block"));
	grid.cell(1, 1, 1) = cell(registry, "grass-block");
	const spk::TextureMesh3D mesh = pg::VoxelMesher::buildRenderMesh(grid, registry);

	EXPECT_EQ(mesh.nbShape(), 54);
	// The buried grass cube uses columns 0-2; every exposed stone face uses column 3.
	EXPECT_TRUE(std::ranges::all_of(mesh.vertices(), [](const spk::TextureVertex3D &p_vertex) {
		return p_vertex.uv.x >= 0.375f;
	}));
}

TEST(VoxelMesher, SlopeBackOccludesButTriangleSideDoesNot)
{
	const pg::VoxelRegistry registry = loadRegistry();

	pg::VoxelGrid behind({1, 1, 2});
	behind.cell(0, 0, 0) = cell(registry, "slope-grass");
	behind.cell(0, 0, 1) = cell(registry, "stone-block");
	EXPECT_EQ(pg::VoxelMesher::buildRenderMesh(behind, registry).nbShape(), 9);

	pg::VoxelGrid beside({2, 1, 1});
	beside.cell(0, 0, 0) = cell(registry, "slope-grass");
	beside.cell(1, 0, 0) = cell(registry, "stone-block");
	EXPECT_EQ(pg::VoxelMesher::buildRenderMesh(beside, registry).nbShape(), 10);
}

TEST(VoxelMesher, CrossPlaneNeverOccludesOrGetsOccluded)
{
	const pg::VoxelRegistry registry = loadRegistry();
	pg::VoxelGrid grid({2, 1, 1});
	grid.cell(0, 0, 0) = cell(registry, "bush");
	grid.cell(1, 0, 0) = cell(registry, "stone-block");
	const spk::TextureMesh3D mesh = pg::VoxelMesher::buildRenderMesh(grid, registry);

	EXPECT_EQ(mesh.nbShape(), 10);
}

TEST(VoxelMesher, MapsWorldPlanesToLocalForEveryOrientationAndFlip)
{
	using O = pg::VoxelOrientation;
	using P = pg::VoxelAxisPlane;
	using F = pg::VoxelFlip;

	EXPECT_EQ(pg::VoxelMesher::mapWorldPlaneToLocal(P::PositiveZ, O::PositiveZ, F::PositiveY), P::PositiveZ);
	EXPECT_EQ(pg::VoxelMesher::mapWorldPlaneToLocal(P::PositiveX, O::PositiveX, F::PositiveY), P::PositiveZ);
	EXPECT_EQ(pg::VoxelMesher::mapWorldPlaneToLocal(P::NegativeZ, O::NegativeZ, F::PositiveY), P::PositiveZ);
	EXPECT_EQ(pg::VoxelMesher::mapWorldPlaneToLocal(P::NegativeX, O::NegativeX, F::PositiveY), P::PositiveZ);
	EXPECT_EQ(pg::VoxelMesher::mapWorldPlaneToLocal(P::PositiveY, O::PositiveZ, F::NegativeY), P::NegativeY);
	EXPECT_EQ(pg::VoxelMesher::mapWorldPlaneToLocal(P::NegativeY, O::PositiveZ, F::NegativeY), P::PositiveY);
}

TEST(VoxelMesher, RotatedSlopeOccludesTheCorrectCubeFace)
{
	const pg::VoxelRegistry registry = loadRegistry();
	const std::array orientations = {
		pg::VoxelOrientation::PositiveX,
		pg::VoxelOrientation::PositiveZ,
		pg::VoxelOrientation::NegativeX,
		pg::VoxelOrientation::NegativeZ};

	for (pg::VoxelOrientation orientation : orientations)
	{
		pg::VoxelGrid grid({3, 1, 3});
		const spk::Vector3Int slopePosition{1, 0, 1};
		grid.cell(slopePosition) = cell(registry, "slope-grass", orientation);
		grid.cell(slopePosition + direction(orientation)) = cell(registry, "stone-block");
		EXPECT_EQ(pg::VoxelMesher::buildRenderMesh(grid, registry).nbShape(), 9);
	}
}

TEST(VoxelMesher, MaskMeshDrapesCubeAndSlopeSurfacesIntoRequestedAtlasCell)
{
	const pg::VoxelRegistry registry = loadRegistry();
	pg::VoxelGrid grid({2, 1, 1});
	grid.cell(0, 0, 0) = cell(registry, "stone-block");
	grid.cell(1, 0, 0) = cell(registry, "slope-grass");
	const pg::VoxelGridCellLookup lookup(grid, registry);
	const std::array positions = {spk::Vector3Int{0, 0, 0}, spk::Vector3Int{1, 0, 0}};
	const spk::TextureMesh3D mesh = pg::VoxelMesher::buildMaskMesh(
		positions,
		[](const pg::VoxelCell &) {
			return pg::AtlasCell{2, 1};
		},
		lookup);

	ASSERT_EQ(mesh.nbShape(), 2);
	EXPECT_TRUE(std::ranges::all_of(mesh.vertices(), [](const spk::TextureVertex3D &p_vertex) {
		return p_vertex.uv.x >= 0.5f && p_vertex.uv.x <= 0.75f &&
			   p_vertex.uv.y >= 0.25f && p_vertex.uv.y <= 0.5f;
	}));

	const auto vertices = mesh.vertices();
	EXPECT_TRUE(std::ranges::any_of(vertices, [](const spk::TextureVertex3D &p_vertex) {
		return p_vertex.position.x < 1.0f && p_vertex.position.y == 1.0f;
	}));
	EXPECT_TRUE(std::ranges::any_of(vertices, [](const spk::TextureVertex3D &p_vertex) {
		return p_vertex.position.x >= 1.0f && p_vertex.position.y == 0.0f;
	}));
	EXPECT_TRUE(std::ranges::any_of(vertices, [](const spk::TextureVertex3D &p_vertex) {
		return p_vertex.position.x >= 1.0f && p_vertex.position.y == 1.0f;
	}));
}

TEST(VoxelMesher, RepeatedMaskCellGetsDeterministicLayerOffset)
{
	const pg::VoxelRegistry registry = loadRegistry();
	pg::VoxelGrid grid({1, 1, 1}, cell(registry, "stone-block"));
	const pg::VoxelGridCellLookup lookup(grid, registry);
	const std::array positions = {spk::Vector3Int{0, 0, 0}, spk::Vector3Int{0, 0, 0}};
	const spk::TextureMesh3D mesh = pg::VoxelMesher::buildMaskMesh(
		positions,
		[](const pg::VoxelCell &) {
			return pg::AtlasCell{0, 0};
		},
		lookup);

	ASSERT_EQ(mesh.nbShape(), 2);
	EXPECT_TRUE(std::ranges::any_of(mesh.vertices(), [](const spk::TextureVertex3D &p_vertex) {
		return std::abs(p_vertex.position.y - 1.001f) < 0.0001f;
	}));
}

TEST(VoxelMesher, ShowcaseMeshTotalsRemainStable)
{
	const pg::VoxelRegistry registry = loadRegistry();
	const pg::VoxelGrid grid = pg::buildShowcaseGrid(registry);
	const spk::TextureMesh3D mesh = pg::VoxelMesher::buildRenderMesh(grid, registry);

	EXPECT_EQ(mesh.nbShape(), 513);
	EXPECT_EQ(mesh.indexes().size(), 3102);
}
