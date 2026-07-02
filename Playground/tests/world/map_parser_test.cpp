#include "core/json.hpp"
#include "core/registries.hpp"
#include "core/registry.hpp"
#include "world/map_definition.hpp"
#include "world/prefab_definition.hpp"

#include <gtest/gtest.h>

#include <filesystem>

namespace
{
	[[nodiscard]] const pg::Registries &registries()
	{
		static const pg::Registries result = [] {
			pg::Registries loaded;
			loaded.loadAll(std::filesystem::path(PG_RESOURCE_DIR) / "data");
			return loaded;
		}();
		return result;
	}

	[[nodiscard]] pg::MapDefinition parseMap(nlohmann::json p_json)
	{
		pg::JsonReader reader(p_json, "map-test.json");
		pg::Registry<pg::PrefabDefinition> noPrefabs;
		return pg::parseMapDefinition(reader, registries().voxels(), noPrefabs);
	}
}

TEST(MapParser, AppliesFillsInOrderThenSparseOverrides)
{
	nlohmann::json json = nlohmann::json::parse(R"json({
		"version": 1,
		"size": [2, 1, 1],
		"palette": {"0": null, "1": "stone-block", "2": "grass-block"},
		"fill": [
			{"min": [0,0,0], "max": [1,0,0], "voxel": "1"},
			{"min": [1,0,0], "max": [1,0,0], "voxel": "2"}
		],
		"cells": [{"at": [0,0,0], "voxel": "0", "orientation": "negativeX", "flip": "negativeY"}],
		"biome": "test"
	})json");
	const pg::MapDefinition map = parseMap(std::move(json));

	EXPECT_TRUE(map.grid.cell(0, 0, 0).isEmpty());
	EXPECT_EQ(map.grid.cell(1, 0, 0).id, registries().voxels().numericId("grass-block"));
	EXPECT_EQ(map.biome, "test");
}

TEST(MapParser, ParsesAuthoredMapStampsMarkersAndOrientation)
{
	const pg::MapDefinition &map = registries().maps().get("m1-testground");
	EXPECT_EQ(map.size(), spk::Vector3Int(64, 16, 64));
	EXPECT_EQ(map.stamps.size(), 3);
	EXPECT_EQ(map.biome, "forest");
	ASSERT_NE(map.marker("playerSpawn"), nullptr);
	EXPECT_EQ(map.marker("playerSpawn")->at, spk::Vector3Int(32, 3, 40));

	const pg::VoxelCell &rotatedBush = map.grid.cell(45, 3, 14);
	EXPECT_EQ(rotatedBush.id, registries().voxels().numericId("bush"));
	EXPECT_EQ(rotatedBush.orientation, pg::VoxelOrientation::PositiveX);
	EXPECT_EQ(map.grid.cell(14, 3, 11).flip, pg::VoxelFlip::NegativeY);
}

TEST(MapParser, ParsesPortalTargetsWithoutResolvingFutureMaps)
{
	nlohmann::json json = nlohmann::json::parse(R"json({
		"version": 1,
		"size": [1, 1, 1],
		"palette": {"0": null},
		"portals": [{
			"name": "exit", "at": [0,0,0],
			"target": {"map": "future-map", "portal": "entry"}
		}]
	})json");
	const pg::MapDefinition map = parseMap(std::move(json));

	ASSERT_EQ(map.portals.size(), 1);
	EXPECT_EQ(map.portals.front().target.map, "future-map");
	EXPECT_EQ(map.portals.front().target.portal, "entry");
}

TEST(MapParser, RejectsUnknownPaletteKeys)
{
	nlohmann::json json = nlohmann::json::parse(R"json({
		"version": 1,
		"size": [1, 1, 1],
		"palette": {"0": null},
		"fill": [{"min": [0,0,0], "max": [0,0,0], "voxel": "missing"}]
	})json");
	try
	{
		(void)parseMap(std::move(json));
		FAIL() << "Expected JsonError";
	}
	catch (const pg::JsonError &error)
	{
		EXPECT_NE(std::string(error.what()).find("$.fill[0].voxel"), std::string::npos);
	}
}

TEST(PrefabParser, LoadsDenseContentAndNamedAnchors)
{
	const pg::PrefabDefinition &prefab = registries().prefabs().get("bush-cluster");
	EXPECT_EQ(prefab.size(), spk::Vector3Int(3, 1, 3));
	ASSERT_EQ(prefab.anchors.size(), 1);
	EXPECT_EQ(prefab.anchors.front().name, "center");
	EXPECT_EQ(prefab.anchors.front().at, spk::Vector3Int(1, 0, 1));
	EXPECT_EQ(prefab.grid.cell(1, 0, 1).id, registries().voxels().numericId("bush"));
}
