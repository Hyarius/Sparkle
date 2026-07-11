#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "structures/voxel/spk_cube_voxel_shape.hpp"
#include "structures/voxel/spk_cuboid_voxel_shape.hpp"
#include "structures/voxel/spk_slab_voxel_shape.hpp"
#include "structures/voxel/spk_slope_voxel_shape.hpp"
#include "structures/voxel/spk_stair_voxel_shape.hpp"
#include "structures/voxel/spk_voxel_mesher.hpp"
#include "structures/voxel/spk_voxel_orientation.hpp"

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

	// Declared as the -X outer face, but inset from the cell boundary. It must not hide
	// geometry in the neighboring cell because the two surfaces never touch.
	class InsetTransparentSideShape : public spk::VoxelShape
	{
	public:
		InsetTransparentSideShape() :
			spk::VoxelShape({{"side", {0, 0}}})
		{
			setTransparency(0.5f);
			setTransparentOcclusionGroup("water");
		}

	protected:
		void _constructRenderFaces() override
		{
			mutableRenderFaces().outer(spk::VoxelAxisPlane::NegativeX).emplace(createRectangle("side", {0.25f, 0, 0}, {0.25f, 0, 1}, {0.25f, 0.75f, 1}, {0.25f, 0.75f, 0}));
		}
	};

	class MidBandTransparentShape : public spk::VoxelShape
	{
	public:
		MidBandTransparentShape() :
			spk::VoxelShape({{"side", {0, 0}}})
		{
			setTransparency(0.5f);
			setTransparentOcclusionGroup("water");
		}

	protected:
		void _constructRenderFaces() override
		{
			mutableRenderFaces().outer(spk::VoxelAxisPlane::NegativeX).emplace(createVerticalRectangle("side", {0, 0.25f, 0}, {0, 0.25f, 1}, {0, 0.5f, 1}, {0, 0.5f, 0}));
		}
	};

	class DiamondBoundaryShape : public spk::VoxelShape
	{
	public:
		DiamondBoundaryShape() :
			spk::VoxelShape({{"side", {0, 0}}})
		{
		}

	protected:
		void _constructRenderFaces() override
		{
			const std::array positions = {
				spk::Vector3{0, 0, 0.5f},
				spk::Vector3{0, 0.5f, 1},
				spk::Vector3{0, 1, 0.5f},
				spk::Vector3{0, 0.5f, 0}};
			const std::array uvs = {
				spk::Vector2{0, 0.5f},
				spk::Vector2{0.5f, 1},
				spk::Vector2{1, 0.5f},
				spk::Vector2{0.5f, 0}};
			mutableRenderFaces().outer(spk::VoxelAxisPlane::NegativeX).emplace(createPolygon("side", positions, uvs));
		}
	};

	class OverlappingBoundaryShape : public spk::VoxelShape
	{
	public:
		OverlappingBoundaryShape() :
			spk::VoxelShape({{"side", {0, 0}}})
		{
		}

	protected:
		void _constructRenderFaces() override
		{
			spk::VoxelShapeFace face(createVerticalRectangle(
				"side", {0, 0, 0}, {0, 0, 0.6f}, {0, 1, 0.6f}, {0, 1, 0}));
			face.addPolygon(createVerticalRectangle(
				"side", {0, 0, 0}, {0, 0, 0.6f}, {0, 1, 0.6f}, {0, 1, 0}));
			mutableRenderFaces().outer(spk::VoxelAxisPlane::NegativeX) = std::move(face);
		}
	};

	[[nodiscard]] std::unique_ptr<spk::VoxelShape> makeTransparentCube(
		float p_transparency,
		std::string p_group = {})
	{
		auto shape = std::make_unique<spk::CubeVoxelShape>(spk::AtlasCell{0, 0});
		shape->setTransparency(p_transparency);
		shape->setTransparentOcclusionGroup(std::move(p_group));
		return shape;
	}

	[[nodiscard]] std::unique_ptr<spk::VoxelShape> makeTransparentSlab(
		float p_height,
		float p_transparency,
		std::string p_group = {})
	{
		auto shape = std::make_unique<spk::SlabVoxelShape>(spk::AtlasCell{0, 0}, p_height);
		shape->setTransparency(p_transparency);
		shape->setTransparentOcclusionGroup(std::move(p_group));
		return shape;
	}

	[[nodiscard]] std::unique_ptr<spk::VoxelShape> makeTransparentCuboid(
		const spk::Vector3 &p_minimum,
		const spk::Vector3 &p_maximum,
		float p_transparency,
		std::string p_group)
	{
		auto shape = std::make_unique<spk::CuboidVoxelShape>(spk::AtlasCell{0, 0}, p_minimum, p_maximum);
		shape->setTransparency(p_transparency);
		shape->setTransparentOcclusionGroup(std::move(p_group));
		return shape;
	}

	struct TestRegistry
	{
		spk::VoxelRegistry registry;
		std::int32_t cube = 0;
		std::int32_t slab = 0;
		std::int32_t cross = 0;
		std::int32_t water = 0;		// half-transparent cube
		std::int32_t deepWater = 0; // same optical medium as water
		std::int32_t shallowWater = 0;
		std::int32_t tallWater = 0;
		std::int32_t midBandWater = 0;
		std::int32_t insetTransparentSide = 0;
		std::int32_t glass = 0; // another optical medium and transparency
		std::int32_t ghost = 0; // fully transparent, never rendered
		std::int32_t slope = 0;
		std::int32_t stair = 0;

		TestRegistry()
		{
			cube = registry.registerShape(std::make_unique<spk::CubeVoxelShape>(spk::AtlasCell{0, 0}));
			slab = registry.registerShape(std::make_unique<SlabShape>());
			cross = registry.registerShape(std::make_unique<CrossShape>());
			water = registry.registerShape(makeTransparentCube(0.5f, "water"));
			deepWater = registry.registerShape(makeTransparentCube(0.5f, "water"));
			shallowWater = registry.registerShape(makeTransparentSlab(0.25f, 0.5f, "water"));
			tallWater = registry.registerShape(makeTransparentSlab(0.75f, 0.5f, "water"));
			midBandWater = registry.registerShape(std::make_unique<MidBandTransparentShape>());
			insetTransparentSide = registry.registerShape(std::make_unique<InsetTransparentSideShape>());
			glass = registry.registerShape(makeTransparentCube(0.25f, "glass"));
			ghost = registry.registerShape(makeTransparentCube(1.0f));
			slope = registry.registerShape(std::make_unique<spk::SlopeVoxelShape>(spk::AtlasCell{0, 0}));
			stair = registry.registerShape(std::make_unique<spk::StairVoxelShape>(spk::AtlasCell{0, 0}, 2));
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

	class SwitchingLookup final : public spk::IVoxelCellLookup
	{
	private:
		spk::Vector3Int _target;
		spk::VoxelCell _cell;
		bool _occupiedFirst = false;
		mutable std::size_t _targetCalls = 0;

	public:
		SwitchingLookup(
			const spk::Vector3Int &p_target,
			std::int32_t p_id,
			bool p_occupiedFirst) :
			_target(p_target),
			_cell{p_id},
			_occupiedFirst(p_occupiedFirst)
		{
		}

		[[nodiscard]] const spk::VoxelCell *tryCell(const spk::Vector3Int &p_worldCell) const override
		{
			if (p_worldCell != _target)
			{
				return nullptr;
			}

			const bool occupied = _targetCalls++ == 0 ? _occupiedFirst : !_occupiedFirst;
			return occupied ? &_cell : nullptr;
		}

		[[nodiscard]] std::size_t targetCalls() const noexcept
		{
			return _targetCalls;
		}
	};

	[[nodiscard]] spk::Vector3 normalForPlane(spk::VoxelAxisPlane p_plane)
	{
		switch (p_plane)
		{
		case spk::VoxelAxisPlane::PositiveX:
			return {1, 0, 0};
		case spk::VoxelAxisPlane::NegativeX:
			return {-1, 0, 0};
		case spk::VoxelAxisPlane::PositiveY:
			return {0, 1, 0};
		case spk::VoxelAxisPlane::NegativeY:
			return {0, -1, 0};
		case spk::VoxelAxisPlane::PositiveZ:
			return {0, 0, 1};
		case spk::VoxelAxisPlane::NegativeZ:
			return {0, 0, -1};
		case spk::VoxelAxisPlane::Count:
			break;
		}
		throw std::invalid_argument("invalid plane");
	}

	[[nodiscard]] spk::Vector3Int offsetForPlane(spk::VoxelAxisPlane p_plane)
	{
		switch (p_plane)
		{
		case spk::VoxelAxisPlane::PositiveX:
			return {1, 0, 0};
		case spk::VoxelAxisPlane::NegativeX:
			return {-1, 0, 0};
		case spk::VoxelAxisPlane::PositiveY:
			return {0, 1, 0};
		case spk::VoxelAxisPlane::NegativeY:
			return {0, -1, 0};
		case spk::VoxelAxisPlane::PositiveZ:
			return {0, 0, 1};
		case spk::VoxelAxisPlane::NegativeZ:
			return {0, 0, -1};
		case spk::VoxelAxisPlane::Count:
			break;
		}
		throw std::invalid_argument("invalid plane");
	}

	[[nodiscard]] float planeCoordinate(const spk::Vector3 &p_position, spk::VoxelAxisPlane p_plane)
	{
		switch (p_plane)
		{
		case spk::VoxelAxisPlane::PositiveX:
		case spk::VoxelAxisPlane::NegativeX:
			return p_position.x;
		case spk::VoxelAxisPlane::PositiveY:
		case spk::VoxelAxisPlane::NegativeY:
			return p_position.y;
		case spk::VoxelAxisPlane::PositiveZ:
		case spk::VoxelAxisPlane::NegativeZ:
			return p_position.z;
		case spk::VoxelAxisPlane::Count:
			break;
		}
		throw std::invalid_argument("invalid plane");
	}

	[[nodiscard]] float surfaceAreaOnBoundary(
		const spk::VoxelMesh3D &p_mesh,
		spk::VoxelAxisPlane p_plane,
		float p_coordinate)
	{
		const spk::Vector3 normal = normalForPlane(p_plane);
		float result = 0.0f;
		for (std::size_t index = 0; index + 2 < p_mesh.indexes().size(); index += 3)
		{
			const spk::VoxelVertex3D &a = p_mesh.vertices()[p_mesh.indexes()[index]];
			const spk::VoxelVertex3D &b = p_mesh.vertices()[p_mesh.indexes()[index + 1]];
			const spk::VoxelVertex3D &c = p_mesh.vertices()[p_mesh.indexes()[index + 2]];
			if (a.normal != normal || b.normal != normal || c.normal != normal ||
				std::abs(planeCoordinate(a.position, p_plane) - p_coordinate) > 0.0001f ||
				std::abs(planeCoordinate(b.position, p_plane) - p_coordinate) > 0.0001f ||
				std::abs(planeCoordinate(c.position, p_plane) - p_coordinate) > 0.0001f)
			{
				continue;
			}
			result += (b.position - a.position).cross(c.position - a.position).length() * 0.5f;
		}
		return result;
	}

	[[nodiscard]] bool containsPolygon(
		const spk::VoxelMesh3D &p_mesh,
		const spk::VoxelShapePolygon &p_polygon,
		const spk::Vector3 &p_offset,
		float p_alpha)
	{
		if (p_polygon.size() > p_mesh.vertices().size())
		{
			return false;
		}
		for (std::size_t start = 0; start + p_polygon.size() <= p_mesh.vertices().size(); ++start)
		{
			bool matches = true;
			for (std::size_t index = 0; index < p_polygon.size(); ++index)
			{
				const spk::VoxelVertex3D &actual = p_mesh.vertices()[start + index];
				const spk::VoxelShapeVertex &expected = p_polygon[index];
				if (actual.position != expected.position + p_offset ||
					actual.normal != p_polygon.normal() ||
					actual.uv != expected.data ||
					std::abs(actual.alpha - p_alpha) > 0.0001f)
				{
					matches = false;
					break;
				}
			}
			if (matches)
			{
				return true;
			}
		}
		return false;
	}
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

	spk::VoxelGrid surrounded({3, 3, 3}, {test.cube});
	surrounded.cell(1, 1, 1) = {test.cross};
	const spk::VoxelMesh3D &surroundedMesh = spk::VoxelMesher::buildRenderMesh(surrounded, test.registry).opaque;
	for (const spk::VoxelShapeFace &face : test.registry.shape(test.cross).renderFaces().innerFaces)
	{
		for (const spk::VoxelShapePolygon &polygon : face.polygons())
		{
			EXPECT_TRUE(containsPolygon(surroundedMesh, polygon, {1, 1, 1}, 1.0f));
		}
	}
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

TEST(VoxelMesher, StatefulLookupIsSampledOnceBeforeOpaqueEmission)
{
	const TestRegistry test;
	const spk::VoxelGrid grid({1, 1, 1}, {test.cube});

	SwitchingLookup emptyThenCube({1, 0, 0}, test.cube, false);
	EXPECT_EQ(spk::VoxelMesher::buildRenderMesh(grid, test.registry, emptyThenCube, {}).opaque.nbShape(), 6u);
	EXPECT_EQ(emptyThenCube.targetCalls(), 1u);

	SwitchingLookup cubeThenEmpty({1, 0, 0}, test.cube, true);
	EXPECT_EQ(spk::VoxelMesher::buildRenderMesh(grid, test.registry, cubeThenEmpty, {}).opaque.nbShape(), 5u);
	EXPECT_EQ(cubeThenEmpty.targetCalls(), 1u);
}

TEST(VoxelMesher, StatefulLookupCannotChangeTransparentPlanDuringEmission)
{
	const TestRegistry test;
	const spk::VoxelGrid grid({1, 1, 1}, {test.water});

	// Had the second observation been used, the mid-band would split one counted quad
	// into two emitted polygons. The retained plan observes only the initial empty cell.
	SwitchingLookup emptyThenBand({1, 0, 0}, test.midBandWater, false);
	EXPECT_EQ(spk::VoxelMesher::buildRenderMesh(grid, test.registry, emptyThenBand, {}).transparent.nbShape(), 6u);
	EXPECT_EQ(emptyThenBand.targetCalls(), 1u);

	// The inverse transition retains both planned remnants instead of emitting fewer
	// polygons than were counted.
	SwitchingLookup bandThenEmpty({1, 0, 0}, test.midBandWater, true);
	EXPECT_EQ(spk::VoxelMesher::buildRenderMesh(grid, test.registry, bandThenEmpty, {}).transparent.nbShape(), 7u);
	EXPECT_EQ(bandThenEmpty.targetCalls(), 1u);
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
	// Two ids explicitly assigned to the same optical medium behave like one continuous
	// body: their internal faces are culled just like between two opaque cubes.
	EXPECT_EQ(spk::VoxelMesher::buildRenderMesh(grid, test.registry).transparent.nbShape(), 10u);
}

TEST(VoxelMesher, EqualAlphaDifferentMediaKeepTheirInterface)
{
	spk::VoxelRegistry registry;
	const std::int32_t water = registry.registerShape(makeTransparentCube(0.5f, "water"));
	const std::int32_t glass = registry.registerShape(makeTransparentCube(0.5f, "glass"));
	spk::VoxelGrid grid({2, 1, 1});
	grid.cell(0, 0, 0) = {water};
	grid.cell(1, 0, 0) = {glass};

	EXPECT_EQ(spk::VoxelMesher::buildRenderMesh(grid, registry).transparent.nbShape(), 12u);
}

TEST(VoxelMesher, SameMediumDoesNotRequireExactAlphaEquality)
{
	spk::VoxelRegistry registry;
	const std::int32_t first = registry.registerShape(makeTransparentCube(0.5f, "water"));
	const std::int32_t second = registry.registerShape(makeTransparentCube(0.50005f, "water"));
	spk::VoxelGrid grid({2, 1, 1});
	grid.cell(0, 0, 0) = {first};
	grid.cell(1, 0, 0) = {second};

	EXPECT_EQ(spk::VoxelMesher::buildRenderMesh(grid, registry).transparent.nbShape(), 10u);
}

TEST(VoxelMesher, RepeatedTransparentShapeIsItsOwnDefaultMedium)
{
	spk::VoxelRegistry registry;
	const std::int32_t water = registry.registerShape(makeTransparentCube(0.5f));
	const spk::VoxelGrid grid({2, 1, 1}, {water});

	EXPECT_EQ(spk::VoxelMesher::buildRenderMesh(grid, registry).transparent.nbShape(), 10u);
}

TEST(VoxelMesher, SameTransparencySlabsCullTheirSharedFacesDespiteDifferentFillHeights)
{
	const TestRegistry test;
	spk::VoxelGrid grid({2, 1, 1});
	grid.cell(0, 0, 0) = {test.tallWater};
	grid.cell(1, 0, 0) = {test.shallowWater};

	const spk::VoxelRenderMeshes meshes = spk::VoxelMesher::buildRenderMesh(grid, test.registry);

	EXPECT_EQ(meshes.transparent.nbShape(), 11u);

	std::vector<float> sharedSideY;
	for (const spk::VoxelVertex3D &vertex : meshes.transparent.vertices())
	{
		if (vertex.normal == spk::Vector3(1.0f, 0.0f, 0.0f) && vertex.position.x == 1.0f)
		{
			sharedSideY.push_back(vertex.position.y);
		}
	}
	std::ranges::sort(sharedSideY);

	ASSERT_EQ(sharedSideY.size(), 4u);
	EXPECT_FLOAT_EQ(sharedSideY[0], 0.25f);
	EXPECT_FLOAT_EQ(sharedSideY[1], 0.25f);
	EXPECT_FLOAT_EQ(sharedSideY[2], 0.75f);
	EXPECT_FLOAT_EQ(sharedSideY[3], 0.75f);
}

TEST(VoxelMesher, VerticalFacesWithSharedYButDisjointHorizontalRangesDoNotCull)
{
	spk::VoxelRegistry registry;
	const std::int32_t left = registry.registerShape(makeTransparentCuboid(
		{0, 0.2f, 0.0f}, {1, 0.8f, 0.4f}, 0.5f, "water"));
	const std::int32_t right = registry.registerShape(makeTransparentCuboid(
		{0, 0.2f, 0.6f}, {1, 0.8f, 1.0f}, 0.5f, "water"));
	spk::VoxelGrid grid({2, 1, 1});
	grid.cell(0, 0, 0) = {left};
	grid.cell(1, 0, 0) = {right};

	const spk::VoxelMesh3D &mesh = spk::VoxelMesher::buildRenderMesh(grid, registry).transparent;
	EXPECT_NEAR(surfaceAreaOnBoundary(mesh, spk::VoxelAxisPlane::PositiveX, 1.0f), 0.24f, 0.0001f);
	EXPECT_NEAR(surfaceAreaOnBoundary(mesh, spk::VoxelAxisPlane::NegativeX, 1.0f), 0.24f, 0.0001f);
}

TEST(VoxelMesher, HorizontalEqualAreaDisjointFootprintsDoNotCull)
{
	spk::VoxelRegistry registry;
	const std::int32_t bottom = registry.registerShape(makeTransparentCuboid(
		{0.0f, 0, 0.0f}, {0.4f, 1, 0.4f}, 0.5f, "water"));
	const std::int32_t top = registry.registerShape(makeTransparentCuboid(
		{0.6f, 0, 0.6f}, {1.0f, 1, 1.0f}, 0.5f, "water"));
	spk::VoxelGrid grid({1, 2, 1});
	grid.cell(0, 0, 0) = {bottom};
	grid.cell(0, 1, 0) = {top};

	const spk::VoxelMesh3D &mesh = spk::VoxelMesher::buildRenderMesh(grid, registry).transparent;
	EXPECT_NEAR(surfaceAreaOnBoundary(mesh, spk::VoxelAxisPlane::PositiveY, 1.0f), 0.16f, 0.0001f);
	EXPECT_NEAR(surfaceAreaOnBoundary(mesh, spk::VoxelAxisPlane::NegativeY, 1.0f), 0.16f, 0.0001f);
}

TEST(VoxelMesher, PartialBoundaryOverlapRemovesOnlyTheIntersection)
{
	spk::VoxelRegistry registry;
	const std::int32_t cube = registry.registerShape(makeTransparentCube(0.5f, "water"));
	const std::int32_t patch = registry.registerShape(makeTransparentCuboid(
		{0, 0.25f, 0.25f}, {0.8f, 0.75f, 0.75f}, 0.5f, "water"));
	spk::VoxelGrid grid({2, 1, 1});
	grid.cell(0, 0, 0) = {cube};
	grid.cell(1, 0, 0) = {patch};

	const spk::VoxelMesh3D &mesh = spk::VoxelMesher::buildRenderMesh(grid, registry).transparent;
	EXPECT_NEAR(surfaceAreaOnBoundary(mesh, spk::VoxelAxisPlane::PositiveX, 1.0f), 0.75f, 0.0001f);

	bool foundClippedVertex = false;
	for (const spk::VoxelVertex3D &vertex : mesh.vertices())
	{
		if (vertex.normal != spk::Vector3(1.0f, 0.0f, 0.0f) || std::abs(vertex.position.x - 1.0f) > 0.0001f)
		{
			continue;
		}
		EXPECT_NEAR(vertex.uv.x, (1.0f - vertex.position.z) / 8.0f, 0.0001f);
		EXPECT_NEAR(vertex.uv.y, (1.0f - vertex.position.y) / 8.0f, 0.0001f);
		foundClippedVertex = foundClippedVertex ||
			std::abs(vertex.position.y - 0.25f) <= 0.0001f ||
			std::abs(vertex.position.y - 0.75f) <= 0.0001f ||
			std::abs(vertex.position.z - 0.25f) <= 0.0001f ||
			std::abs(vertex.position.z - 0.75f) <= 0.0001f;
	}
	EXPECT_TRUE(foundClippedVertex);
}

TEST(VoxelMesher, DiamondExtremaDoNotMasqueradeAsFullBoundaryCoverage)
{
	spk::VoxelRegistry registry;
	const std::int32_t cube = registry.registerShape(std::make_unique<spk::CubeVoxelShape>(spk::AtlasCell{0, 0}));
	const std::int32_t diamond = registry.registerShape(std::make_unique<DiamondBoundaryShape>());
	const spk::VoxelShape &diamondShape = registry.shape(diamond);
	EXPECT_FALSE(diamondShape.outerFaceCoversCellBoundary(spk::VoxelAxisPlane::NegativeX));
	EXPECT_NEAR(diamondShape.outerFaceBoundaryCoverage(spk::VoxelAxisPlane::NegativeX), 0.5f, 0.0001f);

	spk::VoxelGrid grid({2, 1, 1});
	grid.cell(0, 0, 0) = {cube};
	grid.cell(1, 0, 0) = {diamond};
	const spk::VoxelMesh3D &mesh = spk::VoxelMesher::buildRenderMesh(grid, registry).opaque;
	EXPECT_NEAR(surfaceAreaOnBoundary(mesh, spk::VoxelAxisPlane::PositiveX, 1.0f), 0.5f, 0.0001f);
}

TEST(VoxelMesher, OverlappingBoundaryPolygonsContributeUnionCoverageOnly)
{
	spk::VoxelRegistry registry;
	const std::int32_t cube = registry.registerShape(std::make_unique<spk::CubeVoxelShape>(spk::AtlasCell{0, 0}));
	const std::int32_t strips = registry.registerShape(std::make_unique<OverlappingBoundaryShape>());
	const spk::VoxelShape &stripShape = registry.shape(strips);
	EXPECT_FALSE(stripShape.outerFaceCoversCellBoundary(spk::VoxelAxisPlane::NegativeX));
	EXPECT_NEAR(stripShape.outerFaceBoundaryCoverage(spk::VoxelAxisPlane::NegativeX), 0.6f, 0.0001f);

	spk::VoxelGrid grid({2, 1, 1});
	grid.cell(0, 0, 0) = {cube};
	grid.cell(1, 0, 0) = {strips};
	const spk::VoxelMesh3D &mesh = spk::VoxelMesher::buildRenderMesh(grid, registry).opaque;
	EXPECT_NEAR(surfaceAreaOnBoundary(mesh, spk::VoxelAxisPlane::PositiveX, 1.0f), 0.4f, 0.0001f);
}

TEST(VoxelMesher, BoundaryOverlapTracksNeighborOrientationAndVerticalFlip)
{
	const TestRegistry test;
	constexpr std::array orientations = {
		spk::VoxelOrientation::PositiveZ,
		spk::VoxelOrientation::PositiveX,
		spk::VoxelOrientation::NegativeZ,
		spk::VoxelOrientation::NegativeX};
	constexpr std::array flips = {spk::VoxelFlip::PositiveY, spk::VoxelFlip::NegativeY};
	constexpr std::array planes = {
		spk::VoxelAxisPlane::PositiveX,
		spk::VoxelAxisPlane::NegativeX,
		spk::VoxelAxisPlane::PositiveY,
		spk::VoxelAxisPlane::NegativeY,
		spk::VoxelAxisPlane::PositiveZ,
		spk::VoxelAxisPlane::NegativeZ};

	for (const spk::VoxelOrientation orientation : orientations)
	{
		for (const spk::VoxelFlip flip : flips)
		{
			spk::VoxelAxisPlane sharedPlane = spk::VoxelAxisPlane::Count;
			for (const spk::VoxelAxisPlane candidate : planes)
			{
				const spk::VoxelAxisPlane opposite = static_cast<spk::VoxelAxisPlane>(
					static_cast<int>(candidate) % 2 == 0 ? static_cast<int>(candidate) + 1 : static_cast<int>(candidate) - 1);
				if (spk::mapWorldPlaneToLocal(opposite, orientation, flip) == spk::VoxelAxisPlane::NegativeX)
				{
					sharedPlane = candidate;
					break;
				}
			}
			ASSERT_NE(sharedPlane, spk::VoxelAxisPlane::Count);

			spk::VoxelGrid grid({3, 3, 3});
			const spk::Vector3Int center{1, 1, 1};
			grid.cell(center.x, center.y, center.z) = {test.water};
			const spk::Vector3Int neighbor = center + offsetForPlane(sharedPlane);
			grid.cell(neighbor.x, neighbor.y, neighbor.z) = {test.midBandWater, orientation, flip};

			const spk::VoxelMesh3D &mesh = spk::VoxelMesher::buildRenderMesh(grid, test.registry).transparent;
			const bool positivePlane = static_cast<int>(sharedPlane) % 2 == 0;
			const float coordinate = planeCoordinate(spk::Vector3(center), sharedPlane) + (positivePlane ? 1.0f : 0.0f);
			EXPECT_NEAR(surfaceAreaOnBoundary(mesh, sharedPlane, coordinate), 0.75f, 0.0001f)
				<< "orientation " << static_cast<int>(orientation) << " flip " << static_cast<int>(flip);
		}
	}
}

TEST(VoxelMesher, InsetTransparentFaceDoesNotOccludeNeighborBoundary)
{
	const TestRegistry test;
	spk::VoxelGrid grid({2, 1, 1});
	grid.cell(0, 0, 0) = {test.tallWater};
	grid.cell(1, 0, 0) = {test.insetTransparentSide};

	// Six slab faces plus the inset face: neither surface is on the other's boundary.
	EXPECT_EQ(spk::VoxelMesher::buildRenderMesh(grid, test.registry).transparent.nbShape(), 7u);
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

TEST(VoxelMesher, MixedShapeInnerFacesDependOnTrueCellEnclosure)
{
	const TestRegistry test;
	constexpr std::array orientations = {
		spk::VoxelOrientation::PositiveZ,
		spk::VoxelOrientation::PositiveX,
		spk::VoxelOrientation::NegativeZ,
		spk::VoxelOrientation::NegativeX};
	constexpr std::array flips = {spk::VoxelFlip::PositiveY, spk::VoxelFlip::NegativeY};
	constexpr std::array planes = {
		spk::VoxelAxisPlane::PositiveX,
		spk::VoxelAxisPlane::NegativeX,
		spk::VoxelAxisPlane::PositiveY,
		spk::VoxelAxisPlane::NegativeY,
		spk::VoxelAxisPlane::PositiveZ,
		spk::VoxelAxisPlane::NegativeZ};

	for (const std::int32_t shapeId : {test.slope, test.stair})
	{
		const spk::VoxelShape &shape = test.registry.shape(shapeId);
		for (const spk::VoxelOrientation orientation : orientations)
		{
			for (const spk::VoxelFlip flip : flips)
			{
				spk::VoxelAxisPlane openPlane = spk::VoxelAxisPlane::Count;
				for (const spk::VoxelAxisPlane candidate : planes)
				{
					if (spk::mapWorldPlaneToLocal(candidate, orientation, flip) == spk::VoxelAxisPlane::NegativeZ)
					{
						openPlane = candidate;
						break;
					}
				}
				ASSERT_NE(openPlane, spk::VoxelAxisPlane::Count);

				const spk::Vector3Int center{1, 1, 1};
				spk::VoxelGrid enclosed({3, 3, 3}, {test.cube});
				enclosed.cell(center.x, center.y, center.z) = {shapeId, orientation, flip};
				const spk::VoxelMesh3D &enclosedMesh = spk::VoxelMesher::buildRenderMesh(enclosed, test.registry).opaque;

				spk::VoxelGrid exposed = enclosed;
				const spk::Vector3Int opening = center + offsetForPlane(openPlane);
				exposed.cell(opening.x, opening.y, opening.z) = {};
				const spk::VoxelMesh3D &exposedMesh = spk::VoxelMesher::buildRenderMesh(exposed, test.registry).opaque;

				for (std::size_t faceIndex = 0; faceIndex < shape.renderFaces().innerFaces.size(); ++faceIndex)
				{
					const spk::VoxelShapeFace &inner = shape.transformedInnerFace(faceIndex, orientation, flip);
					for (const spk::VoxelShapePolygon &polygon : inner.polygons())
					{
						EXPECT_FALSE(containsPolygon(enclosedMesh, polygon, spk::Vector3(center), 1.0f));
						EXPECT_TRUE(containsPolygon(exposedMesh, polygon, spk::Vector3(center), 1.0f))
							<< "shape " << shapeId << " orientation " << static_cast<int>(orientation)
							<< " flip " << static_cast<int>(flip);
					}
				}
			}
		}
	}
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
