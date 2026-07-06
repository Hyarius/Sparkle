#include <gtest/gtest.h>

#include <memory>

#include "structures/voxel/spk_cube_voxel_shape.hpp"
#include "structures/voxel/spk_voxel_mesher.hpp"

namespace
{
	// Half-height block: the bottom is a full boundary quad (it occludes neighbors), the
	// sides are half quads laying on the cell boundary (they can be occluded but never
	// occlude) and the top sits at mid-height (never involved in occlusion).
	class SlabShape : public spk::VoxelShape
	{
	public:
		SlabShape() :
			spk::VoxelShape({{"side", {0, 0}}, {"top", {0, 0}}, {"bottom", {0, 0}}})
		{
		}

	protected:
		void _constructRenderFaces() override
		{
			auto &faces = mutableRenderFaces();
			faces.outer(spk::VoxelAxisPlane::NegativeY) = createFace(createRectangle(
				"bottom", {0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1}));
			faces.outer(spk::VoxelAxisPlane::PositiveY) = createFace(createRectangle(
				"top", {0, 0.5f, 1}, {1, 0.5f, 1}, {1, 0.5f, 0}, {0, 0.5f, 0}));
			faces.outer(spk::VoxelAxisPlane::PositiveX) = createFace(createRectangle(
				"side", {1, 0, 1}, {1, 0, 0}, {1, 0.5f, 0}, {1, 0.5f, 1}));
			faces.outer(spk::VoxelAxisPlane::NegativeX) = createFace(createRectangle(
				"side", {0, 0, 0}, {0, 0, 1}, {0, 0.5f, 1}, {0, 0.5f, 0}));
			faces.outer(spk::VoxelAxisPlane::PositiveZ) = createFace(createRectangle(
				"side", {0, 0, 1}, {1, 0, 1}, {1, 0.5f, 1}, {0, 0.5f, 1}));
			faces.outer(spk::VoxelAxisPlane::NegativeZ) = createFace(createRectangle(
				"side", {1, 0, 0}, {0, 0, 0}, {0, 0.5f, 0}, {1, 0.5f, 0}));
		}
	};

	// Vegetation-like shape made of inner faces only: it is always drawn while visible
	// and never takes part in the occlusion culling.
	class CrossShape : public spk::VoxelShape
	{
	public:
		CrossShape() :
			spk::VoxelShape({{"quad", {0, 0}}})
		{
		}

	protected:
		void _constructRenderFaces() override
		{
			auto &faces = mutableRenderFaces();
			faces.innerFaces.push_back(createFace(createRectangle(
				"quad", {0, 0, 0}, {1, 0, 1}, {1, 1, 1}, {0, 1, 0})));
			faces.innerFaces.push_back(createFace(createRectangle(
				"quad", {0, 0, 1}, {1, 0, 0}, {1, 1, 0}, {0, 1, 1})));
		}
	};

	struct TestRegistry
	{
		spk::VoxelRegistry registry;
		std::int32_t cube = 0;
		std::int32_t slab = 0;
		std::int32_t cross = 0;

		TestRegistry()
		{
			cube = registry.registerShape(std::make_unique<spk::CubeVoxelShape>(spk::AtlasCell{0, 0}));
			slab = registry.registerShape(std::make_unique<SlabShape>());
			cross = registry.registerShape(std::make_unique<CrossShape>());
		}
	};

	// Reports an infinite field of cubes below y == 0, everything else empty.
	class SolidFloorLookup final : public spk::IVoxelCellLookup
	{
	private:
		spk::VoxelCell _cube;

	public:
		explicit SolidFloorLookup(std::int32_t p_cubeId) :
			_cube{p_cubeId}
		{
		}

		[[nodiscard]] const spk::VoxelCell *tryCell(const spk::Vector3Int &p_worldCell) const override
		{
			return p_worldCell.y < 0 ? &_cube : nullptr;
		}
	};
}

TEST(VoxelMesher, LoneCubeContributesSixFaces)
{
	const TestRegistry test;
	const spk::VoxelGrid grid({1, 1, 1}, {test.cube});
	const spk::TextureMesh3D mesh = spk::VoxelMesher::buildRenderMesh(grid, test.registry);

	EXPECT_EQ(mesh.nbShape(), 6u);
	EXPECT_EQ(mesh.indexes().size(), 36u);
}

TEST(VoxelMesher, AdjacentCubesCullTheirSharedFaces)
{
	const TestRegistry test;
	const spk::VoxelGrid grid({2, 1, 1}, {test.cube});
	const spk::TextureMesh3D mesh = spk::VoxelMesher::buildRenderMesh(grid, test.registry);

	EXPECT_EQ(mesh.nbShape(), 10u);
}

TEST(VoxelMesher, FullyBuriedCubeContributesNoGeometry)
{
	const TestRegistry test;
	const spk::VoxelGrid filled({3, 3, 3}, {test.cube});
	EXPECT_EQ(spk::VoxelMesher::buildRenderMesh(filled, test.registry).nbShape(), 54u);

	// Emptying the center exposes the six faces of the cavity instead.
	spk::VoxelGrid hollow({3, 3, 3}, {test.cube});
	hollow.cell(1, 1, 1) = {};
	EXPECT_EQ(spk::VoxelMesher::buildRenderMesh(hollow, test.registry).nbShape(), 60u);
}

TEST(VoxelMesher, PartialOuterFacesAreCulledButNeverCull)
{
	const TestRegistry test;
	spk::VoxelGrid grid({2, 1, 1});
	grid.cell(0, 0, 0) = {test.slab};
	grid.cell(1, 0, 0) = {test.cube};
	const spk::TextureMesh3D mesh = spk::VoxelMesher::buildRenderMesh(grid, test.registry);

	// The slab keeps bottom, -X, +Z, -Z and its mid-height top (its +X half quad is
	// occluded by the cube); the cube keeps all six faces because the slab's half quad
	// does not cover its -X face.
	EXPECT_EQ(mesh.nbShape(), 11u);
}

TEST(VoxelMesher, InnerFacesIgnoreOcclusionEntirely)
{
	const TestRegistry test;
	spk::VoxelGrid lone({1, 1, 1}, {test.cross});
	EXPECT_EQ(spk::VoxelMesher::buildRenderMesh(lone, test.registry).nbShape(), 2u);

	spk::VoxelGrid grid({2, 1, 1});
	grid.cell(0, 0, 0) = {test.cross};
	grid.cell(1, 0, 0) = {test.cube};
	// Both cross quads stay, and the cube keeps all six faces.
	EXPECT_EQ(spk::VoxelMesher::buildRenderMesh(grid, test.registry).nbShape(), 8u);
}

TEST(VoxelMesher, WorldLookupCullsFacesAcrossTheGridBoundary)
{
	const TestRegistry test;
	const SolidFloorLookup lookup(test.cube);
	const spk::VoxelGrid grid({1, 1, 1}, {test.cube});

	// The chunk sits directly on the solid floor: its bottom face is culled.
	const spk::TextureMesh3D onFloor = spk::VoxelMesher::buildRenderMesh(grid, test.registry, lookup, {0, 0, 0});
	EXPECT_EQ(onFloor.nbShape(), 5u);

	// One cell higher, nothing touches the floor anymore.
	const spk::TextureMesh3D above = spk::VoxelMesher::buildRenderMesh(grid, test.registry, lookup, {0, 1, 0});
	EXPECT_EQ(above.nbShape(), 6u);
}

TEST(VoxelMesher, MapsWorldPlanesToLocalForEveryOrientationAndFlip)
{
	using O = spk::VoxelOrientation;
	using P = spk::VoxelAxisPlane;
	using F = spk::VoxelFlip;

	EXPECT_EQ(spk::VoxelMesher::mapWorldPlaneToLocal(P::PositiveZ, O::PositiveZ, F::PositiveY), P::PositiveZ);
	EXPECT_EQ(spk::VoxelMesher::mapWorldPlaneToLocal(P::PositiveX, O::PositiveX, F::PositiveY), P::PositiveZ);
	EXPECT_EQ(spk::VoxelMesher::mapWorldPlaneToLocal(P::NegativeZ, O::NegativeZ, F::PositiveY), P::PositiveZ);
	EXPECT_EQ(spk::VoxelMesher::mapWorldPlaneToLocal(P::NegativeX, O::NegativeX, F::PositiveY), P::PositiveZ);
	EXPECT_EQ(spk::VoxelMesher::mapWorldPlaneToLocal(P::PositiveY, O::PositiveZ, F::NegativeY), P::NegativeY);
	EXPECT_EQ(spk::VoxelMesher::mapWorldPlaneToLocal(P::NegativeY, O::PositiveZ, F::NegativeY), P::PositiveY);
}

TEST(VoxelMesher, OrientedCellsRemainWatertight)
{
	const TestRegistry test;
	for (const spk::VoxelOrientation orientation : {
			 spk::VoxelOrientation::PositiveZ,
			 spk::VoxelOrientation::PositiveX,
			 spk::VoxelOrientation::NegativeZ,
			 spk::VoxelOrientation::NegativeX})
	{
		spk::VoxelGrid grid({2, 1, 1});
		grid.cell(0, 0, 0) = {test.cube, orientation};
		grid.cell(1, 0, 0) = {test.cube};
		EXPECT_EQ(spk::VoxelMesher::buildRenderMesh(grid, test.registry).nbShape(), 10u)
			<< "orientation " << static_cast<int>(orientation);
	}
}
