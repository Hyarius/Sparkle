#include <gtest/gtest.h>

#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include "structures/voxel/spk_cross_plane_voxel_shape.hpp"
#include "structures/voxel/spk_cube_voxel_shape.hpp"
#include "structures/voxel/spk_slab_voxel_shape.hpp"
#include "structures/voxel/spk_slope_voxel_shape.hpp"
#include "structures/voxel/spk_stair_voxel_shape.hpp"
#include "structures/voxel/spk_voxel_mesher.hpp"
#include "structures/voxel/spk_voxel_mesher_occlusion.hpp"
#include "structures/voxel/spk_voxel_orientation.hpp"

#include "structures/voxel/voxel_mesher_golden_reference.hpp"

// The voxel mesher throughput optimizations (enclosure guard, remnant memoization,
// descriptor-based planning, plane-mapping table, slab neighbor snapshot) must never
// change the emitted geometry. These tests pin the meshes of one representative grid
// byte-for-byte against a captured reference and cross-check the memoized occlusion
// remnants against a fresh computation.

namespace
{
	struct ReferenceRegistry
	{
		spk::VoxelRegistry registry;
		spk::VoxelRuntimeId cube{};
		spk::VoxelRuntimeId slab{};
		spk::VoxelRuntimeId slope{};
		spk::VoxelRuntimeId stair{};
		spk::VoxelRuntimeId cross{};
		spk::VoxelRuntimeId water{};
		spk::VoxelRuntimeId glass{};

		ReferenceRegistry()
		{
			cube = registry.registerShape(std::make_unique<spk::CubeVoxelShape>(spk::AtlasCell{0, 0}));
			slab = registry.registerShape(std::make_unique<spk::SlabVoxelShape>(spk::AtlasCell{1, 0}));
			slope = registry.registerShape(std::make_unique<spk::SlopeVoxelShape>(spk::AtlasCell{2, 0}));
			stair = registry.registerShape(std::make_unique<spk::StairVoxelShape>(spk::AtlasCell{3, 0}, 4));
			cross = registry.registerShape(std::make_unique<spk::DiagonalCrossVoxelShape>(spk::AtlasCell{4, 0}));

			auto waterShape = std::make_unique<spk::CubeVoxelShape>(spk::AtlasCell{5, 0});
			waterShape->setTransparency(0.5f);
			waterShape->setTransparentOcclusionGroup("water");
			water = registry.registerShape(std::move(waterShape));

			auto glassShape = std::make_unique<spk::CubeVoxelShape>(spk::AtlasCell{6, 0});
			glassShape->setTransparency(0.25f);
			glassShape->setTransparentOcclusionGroup("glass");
			glass = registry.registerShape(std::move(glassShape));
		}
	};

	// Fixed 6x4x6 grid mixing every geometry class the mesher handles: full cubes (one
	// fully enclosed), sealed pockets around an inner-only cross and a mixed stair,
	// stairs in all four orientations and both flips, slabs and slopes wedged against
	// cubes and one another (partial-occlusion remnants), and a transparent water body
	// with a same-group internal face and a cross-group glass interface.
	[[nodiscard]] spk::VoxelGrid buildReferenceGrid(const ReferenceRegistry &p_registry)
	{
		spk::VoxelGrid grid({6, 4, 6});

		for (int z = 0; z < 6; ++z)
		{
			for (int x = 0; x < 6; ++x)
			{
				grid.cell(x, 0, z) = {p_registry.cube};
			}
		}

		// Solid block whose center cell (1, 1, 1) is fully enclosed.
		for (int y = 1; y <= 2; ++y)
		{
			for (int z = 0; z <= 2; ++z)
			{
				for (int x = 0; x <= 2; ++x)
				{
					grid.cell(x, y, z) = {p_registry.cube};
				}
			}
		}

		// Sealed pockets: the inner-only cross keeps its quads, the sealed mixed stair
		// suppresses its inner steps.
		constexpr std::array pocketCubes = {
			spk::Vector3Int{3, 2, 2},
			spk::Vector3Int{5, 2, 2},
			spk::Vector3Int{4, 1, 2},
			spk::Vector3Int{4, 3, 2},
			spk::Vector3Int{4, 2, 1},
			spk::Vector3Int{4, 2, 3},
			spk::Vector3Int{3, 2, 4},
			spk::Vector3Int{5, 2, 4},
			spk::Vector3Int{4, 1, 4},
			spk::Vector3Int{4, 3, 4},
			spk::Vector3Int{4, 2, 5}};
		for (const spk::Vector3Int &position : pocketCubes)
		{
			grid.cell(position) = {p_registry.cube};
		}
		grid.cell(4, 2, 2) = {p_registry.cross};
		grid.cell(4, 2, 4) = {p_registry.stair, spk::VoxelOrientation::NegativeX, spk::VoxelFlip::NegativeY};

		// Stair row covering the four orientations and both flips, ending against a
		// cube so stair-vs-stair and stair-vs-cube partial occlusion recurs.
		grid.cell(0, 1, 4) = {p_registry.stair, spk::VoxelOrientation::PositiveX, spk::VoxelFlip::PositiveY};
		grid.cell(1, 1, 4) = {p_registry.stair, spk::VoxelOrientation::PositiveZ, spk::VoxelFlip::NegativeY};
		grid.cell(2, 1, 4) = {p_registry.stair, spk::VoxelOrientation::NegativeX, spk::VoxelFlip::PositiveY};
		grid.cell(3, 1, 4) = {p_registry.stair, spk::VoxelOrientation::NegativeZ, spk::VoxelFlip::NegativeY};

		// Slab against a full cube: the canonical partial occluder.
		grid.cell(5, 1, 4) = {p_registry.slab};

		// Slopes and a slab wedged between the block, the stair row and one another.
		grid.cell(0, 1, 3) = {p_registry.slope, spk::VoxelOrientation::PositiveZ, spk::VoxelFlip::PositiveY};
		grid.cell(1, 1, 3) = {p_registry.slope, spk::VoxelOrientation::NegativeX, spk::VoxelFlip::NegativeY};
		grid.cell(2, 1, 3) = {p_registry.slab, spk::VoxelOrientation::PositiveX, spk::VoxelFlip::NegativeY};

		// Water body (internal face culled) with a glass interface (kept). With the
		// world lookup the z == 5 water faces also cull against external water.
		grid.cell(0, 1, 5) = {p_registry.water};
		grid.cell(1, 1, 5) = {p_registry.water};
		grid.cell(0, 2, 5) = {p_registry.water};
		grid.cell(2, 1, 5) = {p_registry.glass};

		// Exposed, flipped shapes floating clear of everything else.
		grid.cell(0, 3, 0) = {p_registry.stair, spk::VoxelOrientation::PositiveX, spk::VoxelFlip::NegativeY};
		grid.cell(3, 3, 0) = {p_registry.cross};
		grid.cell(5, 3, 5) = {p_registry.slab, spk::VoxelOrientation::PositiveZ, spk::VoxelFlip::NegativeY};

		return grid;
	}

	// Deterministic world around the grid: solid ground below it, an opaque wall one
	// cell west of it, and a water volume one cell south of it, all expressed in world
	// coordinates so a non-zero world origin is exercised too.
	class ReferenceWorldLookup final : public spk::IVoxelCellLookup
	{
	private:
		spk::Vector3Int _origin;
		spk::VoxelCell _cube;
		spk::VoxelCell _water;

	public:
		ReferenceWorldLookup(
			const spk::Vector3Int &p_origin,
			spk::VoxelRuntimeId p_cubeId,
			spk::VoxelRuntimeId p_waterId) :
			_origin(p_origin),
			_cube{p_cubeId},
			_water{p_waterId}
		{
		}

		[[nodiscard]] const spk::VoxelCell *tryCell(const spk::Vector3Int &p_worldCell) const override
		{
			const spk::Vector3Int local = p_worldCell - _origin;
			if (local.y < 0)
			{
				return &_cube;
			}
			if (local.x == -1 && local.y < 4 && local.z >= 0 && local.z < 6)
			{
				return &_cube;
			}
			if (local.z == 6 && local.y < 4 && local.x >= 0 && local.x < 6)
			{
				return &_water;
			}
			return nullptr;
		}
	};

	constexpr spk::Vector3Int ReferenceWorldOrigin{32, 0, -16};

	[[nodiscard]] std::vector<std::uint32_t> vertexBitsOf(const spk::VoxelMesh3D &p_mesh)
	{
		static_assert(sizeof(spk::VoxelVertex3D) == 9 * sizeof(std::uint32_t));
		std::vector<std::uint32_t> result;
		result.reserve(p_mesh.vertices().size() * 9u);
		for (const spk::VoxelVertex3D &vertex : p_mesh.vertices())
		{
			const auto words = std::bit_cast<std::array<std::uint32_t, 9>>(vertex);
			result.insert(result.end(), words.begin(), words.end());
		}
		return result;
	}

	void expectMeshMatchesGolden(
		const spk::VoxelMesh3D &p_mesh,
		std::span<const std::uint32_t> p_vertexBits,
		std::span<const std::uint32_t> p_indexes,
		const char *p_label)
	{
		const std::vector<std::uint32_t> vertexBits = vertexBitsOf(p_mesh);
		ASSERT_EQ(vertexBits.size(), p_vertexBits.size()) << p_label << ": vertex count changed";
		for (std::size_t index = 0; index < vertexBits.size(); ++index)
		{
			if (vertexBits[index] != p_vertexBits[index])
			{
				ADD_FAILURE() << p_label << ": vertex data diverges at word " << index
							  << " (vertex " << index / 9 << ", field " << index % 9 << "): 0x"
							  << std::hex << vertexBits[index] << " != golden 0x" << p_vertexBits[index];
				break;
			}
		}

		const std::span<const std::uint32_t> indexes = p_mesh.indexes();
		ASSERT_EQ(indexes.size(), p_indexes.size()) << p_label << ": index count changed";
		for (std::size_t index = 0; index < indexes.size(); ++index)
		{
			if (indexes[index] != p_indexes[index])
			{
				ADD_FAILURE() << p_label << ": index buffer diverges at " << index << ": "
							  << indexes[index] << " != golden " << p_indexes[index];
				break;
			}
		}
	}

	void writeGoldenArray(std::ostream &p_out, const char *p_name, std::span<const std::uint32_t> p_words)
	{
		p_out << "\tinline constexpr std::array<std::uint32_t, " << std::dec << p_words.size() << "> " << p_name;
		if (p_words.empty())
		{
			p_out << "{};\n\n";
			return;
		}
		p_out << " = {";
		for (std::size_t index = 0; index < p_words.size(); ++index)
		{
			if (index % 12 == 0)
			{
				p_out << "\n\t\t";
			}
			p_out << "0x" << std::hex << p_words[index] << "u,";
		}
		p_out << "};\n\n";
	}

	[[nodiscard]] bool samePolygon(const spk::VoxelShapePolygon &p_left, const spk::VoxelShapePolygon &p_right)
	{
		if (p_left.size() != p_right.size() || p_left.normal() != p_right.normal())
		{
			return false;
		}
		for (std::size_t index = 0; index < p_left.size(); ++index)
		{
			if (p_left[index].position != p_right[index].position || p_left[index].data != p_right[index].data)
			{
				return false;
			}
		}
		return true;
	}

	[[nodiscard]] bool containsPolygon(
		const spk::VoxelMesh3D &p_mesh,
		const spk::VoxelShapePolygon &p_polygon,
		const spk::Vector3 &p_offset)
	{
		for (std::size_t start = 0; start + p_polygon.size() <= p_mesh.vertices().size(); ++start)
		{
			bool matches = true;
			for (std::size_t index = 0; index < p_polygon.size(); ++index)
			{
				const spk::VoxelVertex3D &actual = p_mesh.vertices()[start + index];
				const spk::VoxelShapeVertex &expected = p_polygon[index];
				if (actual.position != expected.position + p_offset ||
					actual.normal != p_polygon.normal() ||
					actual.uv != expected.data)
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

TEST(VoxelMesherInvariance, MeshMatchesGoldenReferenceWithoutWorldLookup)
{
	if (!voxel_mesher_golden::Generated)
	{
		GTEST_SKIP() << "Golden reference not generated yet; run DISABLED_RegenerateGoldenReferenceHeader";
	}

	const ReferenceRegistry test;
	const spk::VoxelGrid grid = buildReferenceGrid(test);
	const spk::VoxelRenderMeshes meshes = spk::VoxelMesher::buildRenderMesh(grid, test.registry);

	expectMeshMatchesGolden(
		meshes.opaque,
		voxel_mesher_golden::noLookupOpaqueVertexBits,
		voxel_mesher_golden::noLookupOpaqueIndexes,
		"opaque (no lookup)");
	expectMeshMatchesGolden(
		meshes.transparent,
		voxel_mesher_golden::noLookupTransparentVertexBits,
		voxel_mesher_golden::noLookupTransparentIndexes,
		"transparent (no lookup)");
}

TEST(VoxelMesherInvariance, MeshMatchesGoldenReferenceWithWorldLookup)
{
	if (!voxel_mesher_golden::Generated)
	{
		GTEST_SKIP() << "Golden reference not generated yet; run DISABLED_RegenerateGoldenReferenceHeader";
	}

	const ReferenceRegistry test;
	const spk::VoxelGrid grid = buildReferenceGrid(test);
	const ReferenceWorldLookup lookup(ReferenceWorldOrigin, test.cube, test.water);
	const spk::VoxelRenderMeshes meshes =
		spk::VoxelMesher::buildRenderMesh(grid, test.registry, lookup, ReferenceWorldOrigin);

	expectMeshMatchesGolden(
		meshes.opaque,
		voxel_mesher_golden::withLookupOpaqueVertexBits,
		voxel_mesher_golden::withLookupOpaqueIndexes,
		"opaque (with lookup)");
	expectMeshMatchesGolden(
		meshes.transparent,
		voxel_mesher_golden::withLookupTransparentVertexBits,
		voxel_mesher_golden::withLookupTransparentIndexes,
		"transparent (with lookup)");
}

// The per-bake remnant cache must return exactly the polygons a fresh subtraction
// produces, in the same order, and reuse them on repeated queries.
TEST(VoxelMesherInvariance, RemnantCacheMatchesFreshVisibleDifference)
{
	const ReferenceRegistry test;
	const spk::VoxelShape &cubeShape = test.registry.shape(test.cube);
	const spk::VoxelShape &slabShape = test.registry.shape(test.slab);

	// Cube +X face against a slab's half-height -X face: a genuine partial occlusion.
	const spk::VoxelShapeFace &source = cubeShape.transformedOuterFace(
		spk::VoxelAxisPlane::PositiveX, spk::VoxelOrientation::PositiveZ, spk::VoxelFlip::PositiveY);
	const spk::VoxelShapeFace &occluder = slabShape.transformedOuterFace(
		spk::VoxelAxisPlane::NegativeX, spk::VoxelOrientation::PositiveZ, spk::VoxelFlip::PositiveY);

	const std::vector<spk::VoxelShapePolygon> fresh = spk::visibleFaceRemnants(source, occluder);
	ASSERT_FALSE(fresh.empty());

	// The subtraction actually removed area: the remnants cover less than the source.
	float remnantArea = 0.0f;
	for (const spk::VoxelShapePolygon &polygon : fresh)
	{
		remnantArea += polygon.area();
	}
	float sourceArea = 0.0f;
	for (const spk::VoxelShapePolygon &polygon : source.polygons())
	{
		sourceArea += polygon.area();
	}
	EXPECT_LT(remnantArea, sourceArea - 0.1f);

	spk::VoxelFaceRemnantCache cache;
	const std::vector<spk::VoxelShapePolygon> &cached = cache.remnants(source, occluder);
	ASSERT_EQ(cached.size(), fresh.size());
	for (std::size_t index = 0; index < cached.size(); ++index)
	{
		EXPECT_TRUE(samePolygon(cached[index], fresh[index])) << "remnant " << index << " differs";
	}

	// Repeated queries return the same storage: the memoization actually hits.
	EXPECT_EQ(&cache.remnants(source, occluder), &cached);
}

// Optimization A only skips the enclosure probe for shapes without inner faces; the
// observable behavior of mixed and inner-only shapes must stay exactly as before.
TEST(VoxelMesherInvariance, SealedMixedShapeStillSuppressesInnerFaces)
{
	const ReferenceRegistry test;
	constexpr spk::VoxelOrientation orientation = spk::VoxelOrientation::NegativeZ;
	constexpr spk::VoxelFlip flip = spk::VoxelFlip::NegativeY;
	const spk::VoxelShape &stairShape = test.registry.shape(test.stair);
	ASSERT_FALSE(stairShape.renderFaces().innerFaces.empty());

	spk::VoxelGrid sealed({3, 3, 3}, {test.cube});
	sealed.cell(1, 1, 1) = {test.stair, orientation, flip};
	const spk::VoxelMesh3D sealedMesh = spk::VoxelMesher::buildRenderMesh(sealed, test.registry).opaque;

	spk::VoxelGrid exposed = sealed;
	exposed.cell(1, 1, 2) = {};
	const spk::VoxelMesh3D exposedMesh = spk::VoxelMesher::buildRenderMesh(exposed, test.registry).opaque;

	for (std::size_t faceIndex = 0; faceIndex < stairShape.renderFaces().innerFaces.size(); ++faceIndex)
	{
		const spk::VoxelShapeFace &inner = stairShape.transformedInnerFace(faceIndex, orientation, flip);
		for (const spk::VoxelShapePolygon &polygon : inner.polygons())
		{
			EXPECT_FALSE(containsPolygon(sealedMesh, polygon, {1, 1, 1}));
			EXPECT_TRUE(containsPolygon(exposedMesh, polygon, {1, 1, 1}));
		}
	}
}

TEST(VoxelMesherInvariance, SealedInnerOnlyShapeKeepsItsGeometry)
{
	const ReferenceRegistry test;
	const spk::VoxelShape &crossShape = test.registry.shape(test.cross);
	ASSERT_FALSE(crossShape.hasOuterFaces());

	spk::VoxelGrid sealed({3, 3, 3}, {test.cube});
	sealed.cell(1, 1, 1) = {test.cross};
	const spk::VoxelMesh3D mesh = spk::VoxelMesher::buildRenderMesh(sealed, test.registry).opaque;

	for (const spk::VoxelShapeFace &face : crossShape.renderFaces().innerFaces)
	{
		for (const spk::VoxelShapePolygon &polygon : face.polygons())
		{
			EXPECT_TRUE(containsPolygon(mesh, polygon, {1, 1, 1}));
		}
	}
}

TEST(VoxelMesherInvariance, FullyEnclosedCubeRegionEmitsOnlyTheShell)
{
	const ReferenceRegistry test;
	const spk::VoxelGrid filled({3, 3, 3}, {test.cube});
	// 26 shell cubes expose 54 faces; the center cube contributes nothing.
	EXPECT_EQ(spk::VoxelMesher::buildRenderMesh(filled, test.registry).opaque.nbShape(), 54u);
}

// The compile-time table is built from mapWorldPlaneToLocal; both must agree over the
// entire 6 x 4 x 2 domain.
TEST(VoxelMesherInvariance, WorldToLocalPlaneTableMatchesMapWorldPlaneToLocal)
{
	constexpr std::array orientations = {
		spk::VoxelOrientation::PositiveX,
		spk::VoxelOrientation::PositiveZ,
		spk::VoxelOrientation::NegativeX,
		spk::VoxelOrientation::NegativeZ};
	constexpr std::array flips = {spk::VoxelFlip::PositiveY, spk::VoxelFlip::NegativeY};

	for (const spk::VoxelOrientation orientation : orientations)
	{
		for (const spk::VoxelFlip flip : flips)
		{
			const auto &row = spk::VoxelWorldToLocalPlaneTable[spk::voxelTransformVariantIndex(orientation, flip)];
			for (std::size_t planeIndex = 0; planeIndex < static_cast<std::size_t>(spk::VoxelAxisPlane::Count); ++planeIndex)
			{
				const auto plane = static_cast<spk::VoxelAxisPlane>(planeIndex);
				EXPECT_EQ(row[planeIndex], spk::mapWorldPlaneToLocal(plane, orientation, flip))
					<< "plane " << planeIndex << " orientation " << static_cast<int>(orientation)
					<< " flip " << static_cast<int>(flip);
			}
		}
	}
}

// Regenerates the captured reference header. Only run it deliberately, after a change
// that is supposed to alter the emitted geometry:
//   SparkleTest --gtest_also_run_disabled_tests
//     --gtest_filter=VoxelMesherInvariance.DISABLED_RegenerateGoldenReferenceHeader
// SPARKLE_VOXEL_MESHER_GOLDEN_PATH selects the output file (default: header name in
// the working directory).
TEST(VoxelMesherInvariance, DISABLED_RegenerateGoldenReferenceHeader)
{
	const ReferenceRegistry test;
	const spk::VoxelGrid grid = buildReferenceGrid(test);
	const ReferenceWorldLookup lookup(ReferenceWorldOrigin, test.cube, test.water);

	const spk::VoxelRenderMeshes noLookup = spk::VoxelMesher::buildRenderMesh(grid, test.registry);
	const spk::VoxelRenderMeshes withLookup =
		spk::VoxelMesher::buildRenderMesh(grid, test.registry, lookup, ReferenceWorldOrigin);

	const char *environmentPath = std::getenv("SPARKLE_VOXEL_MESHER_GOLDEN_PATH");
	const std::string path = environmentPath != nullptr ? environmentPath : "voxel_mesher_golden_reference.hpp";
	std::ofstream out(path);
	ASSERT_TRUE(out.is_open()) << "cannot write " << path;

	out << "#pragma once\n\n"
		<< "// Captured reference meshes for VoxelMesherInvariance (see\n"
		<< "// voxel_mesher_invariance_test.cpp). Vertex floats are stored as raw IEEE-754 bit\n"
		<< "// patterns (9 words per VoxelVertex3D: position, normal, uv, alpha) so the comparison\n"
		<< "// is byte-exact: any change to the emitted geometry, ordering included, fails the test.\n"
		<< "//\n"
		<< "// Regenerate deliberately (and only for intentional geometry changes) by running:\n"
		<< "//   SparkleTest --gtest_also_run_disabled_tests \\\n"
		<< "//     --gtest_filter=VoxelMesherInvariance.DISABLED_RegenerateGoldenReferenceHeader\n"
		<< "// with SPARKLE_VOXEL_MESHER_GOLDEN_PATH pointing at this file. Captured on MSVC x64\n"
		<< "// Release; the CI test runners share that toolchain.\n\n"
		<< "#include <array>\n"
		<< "#include <cstdint>\n\n"
		<< "namespace voxel_mesher_golden\n"
		<< "{\n"
		<< "\tinline constexpr bool Generated = true;\n\n";

	writeGoldenArray(out, "noLookupOpaqueVertexBits", vertexBitsOf(noLookup.opaque));
	writeGoldenArray(out, "noLookupOpaqueIndexes", noLookup.opaque.indexes());
	writeGoldenArray(out, "noLookupTransparentVertexBits", vertexBitsOf(noLookup.transparent));
	writeGoldenArray(out, "noLookupTransparentIndexes", noLookup.transparent.indexes());
	writeGoldenArray(out, "withLookupOpaqueVertexBits", vertexBitsOf(withLookup.opaque));
	writeGoldenArray(out, "withLookupOpaqueIndexes", withLookup.opaque.indexes());
	writeGoldenArray(out, "withLookupTransparentVertexBits", vertexBitsOf(withLookup.transparent));
	writeGoldenArray(out, "withLookupTransparentIndexes", withLookup.transparent.indexes());

	out << "}\n";
	out.close();
	ASSERT_TRUE(out.good());

	std::cout << "[ GOLDEN   ] reference written to " << path << " ("
			  << noLookup.opaque.vertices().size() << " opaque / "
			  << noLookup.transparent.vertices().size() << " transparent vertices without lookup, "
			  << withLookup.opaque.vertices().size() << " / "
			  << withLookup.transparent.vertices().size() << " with lookup)" << std::endl;
}
