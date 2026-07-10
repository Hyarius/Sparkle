#include <gtest/gtest.h>

#include <array>
#include <memory>
#include <stdexcept>

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
			faces.outer(spk::VoxelAxisPlane::NegativeY).emplace(createRectangle("bottom", {0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1}));
			faces.outer(spk::VoxelAxisPlane::PositiveY).emplace(createRectangle("top", {0, 0.5f, 1}, {1, 0.5f, 1}, {1, 0.5f, 0}, {0, 0.5f, 0}));
			faces.outer(spk::VoxelAxisPlane::PositiveX).emplace(createRectangle("side", {1, 0, 1}, {1, 0, 0}, {1, 0.5f, 0}, {1, 0.5f, 1}));
			faces.outer(spk::VoxelAxisPlane::NegativeX).emplace(createRectangle("side", {0, 0, 0}, {0, 0, 1}, {0, 0.5f, 1}, {0, 0.5f, 0}));
			faces.outer(spk::VoxelAxisPlane::PositiveZ).emplace(createRectangle("side", {0, 0, 1}, {1, 0, 1}, {1, 0.5f, 1}, {0, 0.5f, 1}));
			faces.outer(spk::VoxelAxisPlane::NegativeZ).emplace(createRectangle("side", {1, 0, 0}, {0, 0, 0}, {0, 0.5f, 0}, {1, 0.5f, 0}));
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
			faces.innerFaces.push_back(createRectangle(
				"quad", {0, 0, 0}, {1, 0, 1}, {1, 1, 1}, {0, 1, 0}));
			faces.innerFaces.push_back(createRectangle(
				"quad", {0, 0, 1}, {1, 0, 0}, {1, 1, 0}, {0, 1, 1}));
		}
	};

	[[nodiscard]] std::unique_ptr<spk::VoxelShape> makeTransparentCube(float p_transparency)
	{
		auto shape = std::make_unique<spk::CubeVoxelShape>(spk::AtlasCell{0, 0});
		shape->setTransparency(p_transparency);
		return shape;
	}

	struct TestRegistry
	{
		spk::VoxelRegistry registry;
		std::int32_t cube = 0;
		std::int32_t slab = 0;
		std::int32_t cross = 0;
		std::int32_t water = 0;	   // half-transparent cube
		std::int32_t deepWater = 0; // same transparency level as water
		std::int32_t glass = 0;	   // another transparency level
		std::int32_t ghost = 0;	   // fully transparent, never rendered

		TestRegistry()
		{
			cube = registry.registerShape(std::make_unique<spk::CubeVoxelShape>(spk::AtlasCell{0, 0}));
			slab = registry.registerShape(std::make_unique<SlabShape>());
			cross = registry.registerShape(std::make_unique<CrossShape>());
			water = registry.registerShape(makeTransparentCube(0.5f));
			deepWater = registry.registerShape(makeTransparentCube(0.5f));
			glass = registry.registerShape(makeTransparentCube(0.25f));
			ghost = registry.registerShape(makeTransparentCube(1.0f));
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
	const spk::VoxelRenderMeshes meshes = spk::VoxelMesher::buildRenderMesh(grid, test.registry);

	EXPECT_EQ(meshes.opaque.nbShape(), 6u);
	EXPECT_EQ(meshes.opaque.indexes().size(), 36u);
	EXPECT_EQ(meshes.transparent.nbShape(), 0u);
}

TEST(VoxelMesher, AdjacentCubesCullTheirSharedFaces)
{
	const TestRegistry test;
	const spk::VoxelGrid grid({2, 1, 1}, {test.cube});
	const spk::VoxelRenderMeshes meshes = spk::VoxelMesher::buildRenderMesh(grid, test.registry);

	EXPECT_EQ(meshes.opaque.nbShape(), 10u);
}

TEST(VoxelMesher, FullyBuriedCubeContributesNoGeometry)
{
	const TestRegistry test;
	const spk::VoxelGrid filled({3, 3, 3}, {test.cube});
	EXPECT_EQ(spk::VoxelMesher::buildRenderMesh(filled, test.registry).opaque.nbShape(), 54u);

	// Emptying the center exposes the six faces of the cavity instead.
	spk::VoxelGrid hollow({3, 3, 3}, {test.cube});
	hollow.cell(1, 1, 1) = {};
	EXPECT_EQ(spk::VoxelMesher::buildRenderMesh(hollow, test.registry).opaque.nbShape(), 60u);
}

TEST(VoxelMesher, PartialOuterFacesAreCulledButNeverCull)
{
	const TestRegistry test;
	spk::VoxelGrid grid({2, 1, 1});
	grid.cell(0, 0, 0) = {test.slab};
	grid.cell(1, 0, 0) = {test.cube};
	const spk::VoxelRenderMeshes meshes = spk::VoxelMesher::buildRenderMesh(grid, test.registry);

	// The slab keeps bottom, -X, +Z, -Z and its mid-height top (its +X half quad is
	// occluded by the cube); the cube keeps all six faces because the slab's half quad
	// does not cover its -X face.
	EXPECT_EQ(meshes.opaque.nbShape(), 11u);
}

TEST(VoxelMesher, InnerFacesIgnoreOcclusionEntirely)
{
	const TestRegistry test;
	spk::VoxelGrid lone({1, 1, 1}, {test.cross});
	EXPECT_EQ(spk::VoxelMesher::buildRenderMesh(lone, test.registry).opaque.nbShape(), 2u);

	spk::VoxelGrid grid({2, 1, 1});
	grid.cell(0, 0, 0) = {test.cross};
	grid.cell(1, 0, 0) = {test.cube};
	// Both cross quads stay, and the cube keeps all six faces.
	EXPECT_EQ(spk::VoxelMesher::buildRenderMesh(grid, test.registry).opaque.nbShape(), 8u);
}

TEST(VoxelMesher, WorldLookupCullsFacesAcrossTheGridBoundary)
{
	const TestRegistry test;
	const SolidFloorLookup lookup(test.cube);
	const spk::VoxelGrid grid({1, 1, 1}, {test.cube});

	// The chunk sits directly on the solid floor: its bottom face is culled.
	const spk::VoxelRenderMeshes onFloor = spk::VoxelMesher::buildRenderMesh(grid, test.registry, lookup, {0, 0, 0});
	EXPECT_EQ(onFloor.opaque.nbShape(), 5u);

	// One cell higher, nothing touches the floor anymore.
	const spk::VoxelRenderMeshes above = spk::VoxelMesher::buildRenderMesh(grid, test.registry, lookup, {0, 1, 0});
	EXPECT_EQ(above.opaque.nbShape(), 6u);
}

TEST(VoxelMesher, TransparentVoxelsLandInTheTransparentMesh)
{
	const TestRegistry test;
	const spk::VoxelGrid grid({1, 1, 1}, {test.water});
	const spk::VoxelRenderMeshes meshes = spk::VoxelMesher::buildRenderMesh(grid, test.registry);

	EXPECT_EQ(meshes.opaque.nbShape(), 0u);
	EXPECT_EQ(meshes.transparent.nbShape(), 6u);
	// Each vertex carries the voxel's opacity (1 - transparency) for the blending pass.
	for (const spk::VoxelVertex3D &vertex : meshes.transparent.vertices())
	{
		EXPECT_FLOAT_EQ(vertex.alpha, 0.5f);
	}
}

TEST(VoxelMesher, TransparentNeighborsNeverHideOpaqueFaces)
{
	const TestRegistry test;
	spk::VoxelGrid grid({2, 1, 1});
	grid.cell(0, 0, 0) = {test.cube};
	grid.cell(1, 0, 0) = {test.water};
	const spk::VoxelRenderMeshes meshes = spk::VoxelMesher::buildRenderMesh(grid, test.registry);

	// The cube keeps all six faces (the water behind it must not carve a hole), while
	// the water's face against the cube is hidden by the opaque neighbor.
	EXPECT_EQ(meshes.opaque.nbShape(), 6u);
	EXPECT_EQ(meshes.transparent.nbShape(), 5u);
}

TEST(VoxelMesher, SameTransparencyLevelsCullTheirSharedFaces)
{
	const TestRegistry test;
	spk::VoxelGrid grid({2, 1, 1});
	grid.cell(0, 0, 0) = {test.water};
	grid.cell(1, 0, 0) = {test.deepWater};
	// Two ids of the same transparency level behave like one continuous body: the
	// internal faces are culled just like between two opaque cubes.
	EXPECT_EQ(spk::VoxelMesher::buildRenderMesh(grid, test.registry).transparent.nbShape(), 10u);
}

TEST(VoxelMesher, DifferentTransparencyLevelsKeepTheirSharedFaces)
{
	const TestRegistry test;
	spk::VoxelGrid grid({2, 1, 1});
	grid.cell(0, 0, 0) = {test.water};
	grid.cell(1, 0, 0) = {test.glass};
	// Both interface faces stay visible: looking through one level must still show the
	// other one behind it.
	EXPECT_EQ(spk::VoxelMesher::buildRenderMesh(grid, test.registry).transparent.nbShape(), 12u);
}

TEST(VoxelMesher, FullyTransparentVoxelsEmitNothingAndHideNothing)
{
	const TestRegistry test;
	spk::VoxelGrid lone({1, 1, 1}, {test.ghost});
	const spk::VoxelRenderMeshes loneMeshes = spk::VoxelMesher::buildRenderMesh(lone, test.registry);
	EXPECT_EQ(loneMeshes.opaque.nbShape(), 0u);
	EXPECT_EQ(loneMeshes.transparent.nbShape(), 0u);

	spk::VoxelGrid grid({2, 1, 1});
	grid.cell(0, 0, 0) = {test.ghost};
	grid.cell(1, 0, 0) = {test.cube};
	// The invisible voxel neither renders nor occludes: the cube keeps all six faces.
	EXPECT_EQ(spk::VoxelMesher::buildRenderMesh(grid, test.registry).opaque.nbShape(), 6u);
}

TEST(VoxelMesher, MapsWorldPlanesToLocalForEveryOrientationAndFlip)
{
	using O = spk::VoxelOrientation;
	using P = spk::VoxelAxisPlane;
	using F = spk::VoxelFlip;

	constexpr std::array worldPlanes = {P::PositiveX, P::NegativeX, P::PositiveY, P::NegativeY, P::PositiveZ, P::NegativeZ};
	constexpr std::array orientations = {O::PositiveZ, O::PositiveX, O::NegativeZ, O::NegativeX};
	constexpr std::array positiveYMappings = {
		std::array{P::PositiveX, P::NegativeX, P::PositiveY, P::NegativeY, P::PositiveZ, P::NegativeZ},
		std::array{P::PositiveZ, P::NegativeZ, P::PositiveY, P::NegativeY, P::NegativeX, P::PositiveX},
		std::array{P::NegativeX, P::PositiveX, P::PositiveY, P::NegativeY, P::NegativeZ, P::PositiveZ},
		std::array{P::NegativeZ, P::PositiveZ, P::PositiveY, P::NegativeY, P::PositiveX, P::NegativeX}};

	for (std::size_t orientationIndex = 0; orientationIndex < orientations.size(); ++orientationIndex)
	{
		for (std::size_t planeIndex = 0; planeIndex < worldPlanes.size(); ++planeIndex)
		{
			EXPECT_EQ(
				spk::VoxelMesher::mapWorldPlaneToLocal(worldPlanes[planeIndex], orientations[orientationIndex], F::PositiveY),
				positiveYMappings[orientationIndex][planeIndex]);

			P flippedMapping = positiveYMappings[orientationIndex][planeIndex];
			if (flippedMapping == P::PositiveY)
			{
				flippedMapping = P::NegativeY;
			}
			else if (flippedMapping == P::NegativeY)
			{
				flippedMapping = P::PositiveY;
			}
			EXPECT_EQ(
				spk::VoxelMesher::mapWorldPlaneToLocal(worldPlanes[planeIndex], orientations[orientationIndex], F::NegativeY),
				flippedMapping);
		}
	}

	EXPECT_THROW((void)spk::VoxelMesher::mapWorldPlaneToLocal(P::Count, O::PositiveZ, F::PositiveY), std::invalid_argument);
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
		EXPECT_EQ(spk::VoxelMesher::buildRenderMesh(grid, test.registry).opaque.nbShape(), 10u)
			<< "orientation " << static_cast<int>(orientation);
	}
}
