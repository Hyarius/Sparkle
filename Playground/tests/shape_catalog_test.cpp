#include <gtest/gtest.h>

#include "voxel/shape_catalog.hpp"

#include "structures/voxel/spk_cross_plane_voxel_shape.hpp"
#include "structures/voxel/spk_cube_voxel_shape.hpp"
#include "structures/voxel/spk_cuboid_voxel_shape.hpp"
#include "structures/voxel/spk_hexa_plane_voxel_shape.hpp"
#include "structures/voxel/spk_slab_voxel_shape.hpp"
#include "structures/voxel/spk_slope_voxel_shape.hpp"
#include "structures/voxel/spk_stair_voxel_shape.hpp"

#include <filesystem>
#include <memory>
#include <vector>

namespace
{
	// The JSON shape catalog was ported from the legacy C++ shape classes; these tests pin
	// that migration by comparing the full polygon soup (outer and inner faces flattened)
	// of every catalog shape against its legacy counterpart. Classification may differ on
	// purpose - the data shapes compute outer/inner from the geometry - but the rendered
	// polygons themselves (positions, atlas UVs, normals) must match exactly.
	class ShapeCatalogParity : public ::testing::Test
	{
	protected:
		static pg::ShapeCatalog _catalog;
		static bool _loaded;

		static void SetUpTestSuite()
		{
			if (!_loaded)
			{
				spk::loadJsonDirectory(
					_catalog,
					std::filesystem::path(PG_RESOURCE_DIR) / "data" / "shapes",
					[](std::string_view p_id, pg::JsonReader &p_reader) {
						pg::ShapeDefinition definition = pg::parseShapeDefinition(p_reader);
						definition.id = p_id;
						return definition;
					});
				_loaded = true;
			}
		}

		[[nodiscard]] static std::vector<const spk::VoxelShapePolygon *> allPolygons(const spk::VoxelShape &p_shape)
		{
			std::vector<const spk::VoxelShapePolygon *> result;
			for (const auto &face : p_shape.renderFaces().outerShell)
			{
				if (!face.has_value())
				{
					continue;
				}
				for (const spk::VoxelShapePolygon &polygon : face->polygons())
				{
					result.push_back(&polygon);
				}
			}
			for (const spk::VoxelShapeFace &face : p_shape.renderFaces().innerFaces)
			{
				for (const spk::VoxelShapePolygon &polygon : face.polygons())
				{
					result.push_back(&polygon);
				}
			}
			return result;
		}

		[[nodiscard]] static bool samePolygon(const spk::VoxelShapePolygon &p_left, const spk::VoxelShapePolygon &p_right)
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

		static void expectSamePolygonSoup(const std::string &p_shapeId, spk::VoxelShape &p_legacy, spk::VoxelShape::TextureSlots p_textures)
		{
			std::unique_ptr<spk::VoxelShape> dataShape = _catalog.get(p_shapeId).instantiate(std::move(p_textures));
			dataShape->initialize();
			p_legacy.initialize();

			const std::vector<const spk::VoxelShapePolygon *> dataPolygons = allPolygons(*dataShape);
			const std::vector<const spk::VoxelShapePolygon *> legacyPolygons = allPolygons(p_legacy);
			ASSERT_EQ(dataPolygons.size(), legacyPolygons.size()) << "shape '" << p_shapeId << "'";

			std::vector<bool> matched(dataPolygons.size(), false);
			for (const spk::VoxelShapePolygon *legacyPolygon : legacyPolygons)
			{
				bool found = false;
				for (std::size_t index = 0; index < dataPolygons.size(); ++index)
				{
					if (!matched[index] && samePolygon(*legacyPolygon, *dataPolygons[index]))
					{
						matched[index] = true;
						found = true;
						break;
					}
				}
				EXPECT_TRUE(found) << "shape '" << p_shapeId << "' misses a legacy polygon with normal ("
								   << legacyPolygon->normal().x << ", " << legacyPolygon->normal().y << ", "
								   << legacyPolygon->normal().z << ")";
			}
		}
	};

	pg::ShapeCatalog ShapeCatalogParity::_catalog;
	bool ShapeCatalogParity::_loaded = false;
}

TEST_F(ShapeCatalogParity, CubeMatchesLegacyShape)
{
	const spk::AtlasCell cell{1, 2};
	spk::CubeVoxelShape legacy(cell);
	expectSamePolygonSoup(
		"cube",
		legacy,
		{{"top", cell}, {"bottom", cell}, {"side", cell}});
}

TEST_F(ShapeCatalogParity, DirectionalCubeMatchesLegacyShape)
{
	spk::CubeVoxelShape legacy(
		{{"top", {4, 0}}, {"bottom", {3, 0}}, {"posX", {5, 0}}, {"negX", {5, 1}}, {"posZ", {6, 0}}, {"negZ", {6, 1}}});
	expectSamePolygonSoup(
		"cube-directional",
		legacy,
		{{"top", {4, 0}}, {"bottom", {3, 0}}, {"posX", {5, 0}}, {"negX", {5, 1}}, {"posZ", {6, 0}}, {"negZ", {6, 1}}});
}

TEST_F(ShapeCatalogParity, SlabMatchesLegacyShape)
{
	const spk::AtlasCell cell{3, 0};
	spk::SlabVoxelShape legacy(cell, 0.5f);
	expectSamePolygonSoup(
		"slab",
		legacy,
		{{"top", cell}, {"bottom", cell}, {"side", cell}});
}

TEST_F(ShapeCatalogParity, SlopeMatchesLegacyShape)
{
	const spk::AtlasCell cell{0, 1};
	spk::SlopeVoxelShape legacy(cell);
	expectSamePolygonSoup(
		"slope",
		legacy,
		{{"slope", cell}, {"back", cell}, {"bottom", cell}, {"sideLeft", cell}, {"sideRight", cell}});
}

TEST_F(ShapeCatalogParity, StairMatchesLegacyShape)
{
	const spk::AtlasCell cell{2, 0};
	spk::StairVoxelShape legacy(cell, 2);
	expectSamePolygonSoup(
		"stair",
		legacy,
		{{"top", cell}, {"riser", cell}, {"back", cell}, {"bottom", cell}, {"sideLeft", cell}, {"sideRight", cell}});
}

TEST_F(ShapeCatalogParity, CrossPlaneMatchesLegacyShape)
{
	const spk::AtlasCell cell{0, 5};
	spk::DiagonalCrossVoxelShape legacy(cell);
	expectSamePolygonSoup("cross-plane", legacy, {{"plane", cell}});
}

TEST_F(ShapeCatalogParity, HexaPlaneMatchesLegacyShape)
{
	const spk::AtlasCell cell{3, 1};
	spk::HexaPlaneVoxelShape legacy(cell);
	expectSamePolygonSoup("hexa-plane", legacy, {{"plane", cell}});
}

TEST_F(ShapeCatalogParity, CuboidMatchesLegacyShape)
{
	// oak-trunk was authored as cuboid min {0.31, 0, 0.31} / max {0.69, 1, 0.69}.
	const spk::AtlasCell cell{5, 3};
	spk::CuboidVoxelShape legacy(cell, {0.31f, 0.0f, 0.31f}, {0.69f, 1.0f, 0.69f});
	expectSamePolygonSoup(
		"oak-trunk",
		legacy,
		{{"top", cell}, {"bottom", cell}, {"side", cell}});
}

TEST_F(ShapeCatalogParity, HeightsCarryTheLegacyParserFormulas)
{
	const pg::CardinalHeightCollection &slab = _catalog.get("slab").heights;
	EXPECT_FLOAT_EQ(slab.positiveY.stationary, 0.5f);
	EXPECT_FLOAT_EQ(slab.negativeY.stationary, 1.0f);

	const pg::CardinalHeightCollection &stair = _catalog.get("stair").heights;
	EXPECT_FLOAT_EQ(stair.positiveY.positiveZ, 1.0f);
	EXPECT_FLOAT_EQ(stair.positiveY.negativeZ, 0.25f); // first-step midpoint at stepCount 2
	EXPECT_FLOAT_EQ(stair.positiveY.stationary, 0.5f);

	const pg::CardinalHeightCollection &crossPlane = _catalog.get("cross-plane").heights;
	EXPECT_FLOAT_EQ(crossPlane.positiveY.stationary, 0.0f);
	EXPECT_FLOAT_EQ(crossPlane.negativeY.stationary, 0.0f);
}
