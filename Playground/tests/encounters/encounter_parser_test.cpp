#include "core/json.hpp"
#include "core/registries.hpp"
#include "encounters/biome.hpp"
#include "encounters/encounter_table.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <string>

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

	[[nodiscard]] nlohmann::json validTableJson()
	{
		return nlohmann::json::parse(R"json({
			"version": 1,
			"tiers": {
				"noBadge": {"weightedTeams": [{
					"displayName": "pair", "weight": 2,
					"team": [
						{"species": "sprout", "ai": "aggressive", "completedNodes": ["node-a"]},
						null, null, null, null, null
					]
				}]},
				"oneBadge": {"weightedTeams": []},
				"twoBadges": {"weightedTeams": []},
				"threeBadges": {"weightedTeams": []},
				"fourBadges": {"weightedTeams": []},
				"fiveBadges": {"weightedTeams": []},
				"sixBadges": {"weightedTeams": []},
				"sevenBadges": {"weightedTeams": []},
				"eightBadges": {"weightedTeams": []},
				"postGame": {"weightedTeams": []}
			}
		})json");
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
			EXPECT_NE(std::string(error.what()).find("encounter-test.json:" + p_path), std::string::npos)
				<< error.what();
		}
	}
}

TEST(EncounterTableParser, LoadsNullPaddedTeamsAndAuthoredData)
{
	nlohmann::json json = validTableJson();
	pg::JsonReader reader(json, "encounter-test.json");
	const pg::EncounterTable table = pg::parseEncounterTable(reader);

	const pg::EncounterTier *tier = table.tierForBadges(0);
	ASSERT_NE(tier, nullptr);
	ASSERT_EQ(tier->weightedTeams.size(), 1);
	EXPECT_EQ(tier->weightedTeams.front().displayName, "pair");
	ASSERT_EQ(tier->weightedTeams.front().team.size(), 1);
	EXPECT_EQ(tier->weightedTeams.front().team.front().speciesId, "sprout");
	EXPECT_EQ(tier->weightedTeams.front().team.front().completedNodeUuids, (std::vector<std::string>{"node-a"}));

	const pg::EncounterTable &authored = registries().encounterTables().get("forest-basic");
	ASSERT_NE(authored.tierForBadges(0), nullptr);
	EXPECT_EQ(authored.tierForBadges(0)->weightedTeams.front().team.front().speciesId, "sprout");
}

TEST(EncounterTableParser, RejectsInvalidWeightAndSlotCount)
{
	nlohmann::json badWeight = validTableJson();
	badWeight["tiers"]["noBadge"]["weightedTeams"][0]["weight"] = 0;
	pg::JsonReader weightReader(badWeight, "encounter-test.json");
	expectJsonErrorAt([&] {
		(void)pg::parseEncounterTable(weightReader);
	},
					  "$.tiers.noBadge.weightedTeams[0].weight");

	nlohmann::json badSlots = validTableJson();
	badSlots["tiers"]["noBadge"]["weightedTeams"][0]["team"].erase(5);
	pg::JsonReader slotsReader(badSlots, "encounter-test.json");
	expectJsonErrorAt([&] {
		(void)pg::parseEncounterTable(slotsReader);
	},
					  "$.tiers.noBadge.weightedTeams[0].team");
}

TEST(BiomeParser, LoadsRulesAndRejectsBadReferencesAndChance)
{
	nlohmann::json json = nlohmann::json::parse(R"json({
		"version": 1,
		"displayName": "Test forest",
		"palette": {
			"surface": "grass-block", "subsurface": "dirt-block",
			"deep": "stone-block", "flora": ["bush"]
		},
		"encounterRules": [{"trigger": "bush", "table": "forest-basic", "chancePerStep": 0.25}]
	})json");
	pg::JsonReader reader(json, "encounter-test.json");
	const pg::BiomeDefinition biome = pg::parseBiomeDefinition(
		reader, registries().encounterTables(), registries().voxels());
	ASSERT_EQ(biome.encounterRules.size(), 1);
	EXPECT_EQ(biome.encounterRules.front().table, &registries().encounterTables().get("forest-basic"));

	nlohmann::json unknownTable = json;
	unknownTable["encounterRules"][0]["table"] = "missing";
	pg::JsonReader tableReader(unknownTable, "encounter-test.json");
	expectJsonErrorAt(
		[&] {
			(void)pg::parseBiomeDefinition(tableReader, registries().encounterTables(), registries().voxels());
		},
		"$.encounterRules[0].table");

	nlohmann::json badChance = json;
	badChance["encounterRules"][0]["chancePerStep"] = 1.1;
	pg::JsonReader chanceReader(badChance, "encounter-test.json");
	expectJsonErrorAt(
		[&] {
			(void)pg::parseBiomeDefinition(chanceReader, registries().encounterTables(), registries().voxels());
		},
		"$.encounterRules[0].chancePerStep");
}
