#include "core/json.hpp"
#include "voxel/shapes/cross_plane_shape.hpp"
#include "voxel/shapes/cube_shape.hpp"
#include "voxel/shapes/slab_shape.hpp"
#include "voxel/shapes/slope_shape.hpp"
#include "voxel/shapes/stair_shape.hpp"
#include "voxel/voxel_definition.hpp"
#include "voxel/voxel_registry.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace
{
	std::filesystem::path voxelResourceDirectory()
	{
		return std::filesystem::path(PG_RESOURCE_DIR) / "data" / "voxels";
	}

	nlohmann::json validCubeJson()
	{
		return nlohmann::json::parse(R"json(
			{
				"version": 1,
				"traversal": "solid",
				"tags": ["ground"],
				"shape": {
					"type": "cube",
					"textures": {
						"top": [0, 0],
						"bottom": [1, 0],
						"side": [2, 0]
					}
				}
			}
		)json");
	}

	template <typename TFunction>
	void expectJsonErrorAt(TFunction p_function, const std::string &p_path)
	{
		try
		{
			p_function();
			FAIL() << "Expected pg::JsonError";
		} catch (const pg::JsonError &error)
		{
			EXPECT_NE(std::string(error.what()).find("sample.json:" + p_path), std::string::npos) << error.what();
		}
	}

	void parseExpectingError(nlohmann::json p_json, const std::string &p_path)
	{
		pg::JsonReader reader(p_json, "sample.json");
		expectJsonErrorAt([&reader]() {
			(void)pg::parseVoxelDefinition(reader);
		},
						  p_path);
	}
}

TEST(VoxelParser, LoadsEveryAuthoredDefinitionAndShapeType)
{
	pg::VoxelRegistry registry;
	registry.load(voxelResourceDirectory());

	ASSERT_EQ(registry.size(), 8);
	EXPECT_NE(dynamic_cast<const pg::CrossPlaneShape *>(registry.get("bush").shape.get()), nullptr);
	EXPECT_NE(dynamic_cast<const pg::CubeShape *>(registry.get("grass-block").shape.get()), nullptr);
	EXPECT_NE(dynamic_cast<const pg::SlabShape *>(registry.get("slab-stone").shape.get()), nullptr);
	EXPECT_NE(dynamic_cast<const pg::SlopeShape *>(registry.get("slope-grass").shape.get()), nullptr);
	EXPECT_NE(dynamic_cast<const pg::StairShape *>(registry.get("stair-stone").shape.get()), nullptr);
	EXPECT_EQ(registry.get("bush").data.traversal, pg::VoxelTraversal::Passable);
	EXPECT_TRUE(registry.get("bush").data.hasTag("BUSH"));
	EXPECT_TRUE(registry.get("bush").data.hasTag("losTransparent"));
}

TEST(VoxelParser, RejectsMissingRequiredField)
{
	nlohmann::json json = validCubeJson();
	json.erase("traversal");
	parseExpectingError(std::move(json), "$.traversal");
}

TEST(VoxelParser, RejectsUnknownRootField)
{
	nlohmann::json json = validCubeJson();
	json["unexpected"] = true;
	parseExpectingError(std::move(json), "$.unexpected");
}

TEST(VoxelParser, RejectsBadTraversalEnum)
{
	nlohmann::json json = validCubeJson();
	json["traversal"] = "liquid";
	parseExpectingError(std::move(json), "$.traversal");
}

TEST(VoxelParser, RejectsWrongVersion)
{
	nlohmann::json json = validCubeJson();
	json["version"] = 2;
	parseExpectingError(std::move(json), "$.version");
}

TEST(VoxelParser, RejectsUnknownShapeType)
{
	nlohmann::json json = validCubeJson();
	json["shape"]["type"] = "sphere";
	parseExpectingError(std::move(json), "$.shape.type");
}

TEST(VoxelParser, RejectsMissingShapeTextureSlot)
{
	nlohmann::json json = validCubeJson();
	json["shape"]["textures"].erase("bottom");
	parseExpectingError(std::move(json), "$.shape.textures.bottom");
}

TEST(VoxelParser, RejectsUnknownShapeTextureSlot)
{
	nlohmann::json json = validCubeJson();
	json["shape"]["textures"]["extra"] = {0, 0};
	parseExpectingError(std::move(json), "$.shape.textures.extra");
}

TEST(VoxelParser, RejectsAtlasCellOutsideGrid)
{
	nlohmann::json json = validCubeJson();
	json["shape"]["textures"]["top"] = {8, 0};
	parseExpectingError(std::move(json), "$.shape.textures.top");
}

TEST(VoxelParser, RejectsInvalidShapeParameter)
{
	nlohmann::json json = validCubeJson();
	json["shape"] = {
		{"type", "slab"},
		{"height", 1.5},
		{"textures", {{"top", {0, 0}}, {"bottom", {1, 0}}, {"side", {2, 0}}}}};
	parseExpectingError(std::move(json), "$.shape.height");
}

TEST(VoxelRegistry, AssignsStableNumericIdsInSortedStringOrder)
{
	pg::VoxelRegistry registry;
	registry.load(voxelResourceDirectory());

	const std::vector<std::string> expected = {
		"bush",
		"dirt-block",
		"grass-block",
		"slab-stone",
		"slope-grass",
		"stair-stone",
		"stone-block",
		"wall-stone"};
	EXPECT_EQ(registry.ids(), expected);
	for (std::size_t index = 0; index < expected.size(); ++index)
	{
		const auto numericId = static_cast<std::int32_t>(index);
		EXPECT_EQ(registry.numericId(expected[index]), numericId);
		EXPECT_EQ(registry.stringId(numericId), expected[index]);
		EXPECT_EQ(registry.get(numericId).id, expected[index]);
	}
	EXPECT_EQ(registry.tryGet(-1), nullptr);
}
