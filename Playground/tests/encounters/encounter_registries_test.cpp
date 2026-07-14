#include <gtest/gtest.h>

#include "core/paths.hpp"
#include "core/registries.hpp"
#include "support/scratch_data.hpp"

#include <exception>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace
{
	// The shipped wild table, with one field swapped out by the case.
	[[nodiscard]] std::string wildTable(
		std::string_view p_species = "training-sprout",
		std::string_view p_ai = "training-aggressive",
		std::string_view p_nodes = "[]")
	{
		return std::string(R"({
			"version": 1,
			"displayNameKey": "encounter.training-wild.name",
			"kind": "wild",
			"allowsTaming": true,
			"repeatable": true,
			"triggerChancePermille": 1000,
			"board": {
				"type": "liveWorld",
				"size": [11, 11],
				"deploymentDepth": 2,
				"opponentPlacement": {"type": "byLine", "rowsFromEnemyEdge": 0, "order": "centerOut"}
			},
			"tiers": [{"tier": 0, "teams": [{
				"id": "lone-sprout",
				"displayNameKey": "encounter.training-wild.lone-sprout.name",
				"weight": 1,
				"members": [{"id": "sprout-a", "species": ")") +
			   std::string(p_species) + R"(", "ai": ")" + std::string(p_ai) + R"(", "completedFeatNodes": )" +
			   std::string(p_nodes) + "}]}]}]}";
	}

	// The shipped meadow, with its version and its wild link swapped out. Patching the real file
	// rather than authoring a synthetic one keeps the case about the migration instead of about
	// reproducing a whole worldgen block.
	[[nodiscard]] std::string meadow(std::string_view p_version, std::string_view p_table)
	{
		std::ifstream stream(pg::resourceRoot() / "data" / "biomes" / "meadow.json", std::ios::binary);
		std::ostringstream buffer;
		buffer << stream.rdbuf();
		std::string text = buffer.str();

		const std::string version = R"("version": 2,)";
		const std::string link = R"("wildEncounterTable": "training-wild",)";
		EXPECT_NE(text.find(version), std::string::npos) << "the shipped meadow is a version-2 biome";
		EXPECT_NE(text.find(link), std::string::npos) << "and it is linked to the training table";

		text.replace(text.find(version), version.size(), std::string(R"("version": )") + std::string(p_version) + ",");
		text.replace(
			text.find(link),
			link.size(),
			p_table.empty() ? std::string()
							: std::string(R"("wildEncounterTable": ")") + std::string(p_table) + R"(",)");
		return text;
	}
}

TEST(EncounterRegistriesTest, LoadsTheShippedEncountersAndTheirEmptyBattleBoardRegistry)
{
	pg::Registries registries;
	ASSERT_NO_THROW(registries.loadAll(pg::resourceRoot() / "data"));

	EXPECT_EQ(registries.encounters().ids(), (std::vector<std::string>{"debug-battle", "training-wild"}));
	EXPECT_EQ(registries.species().ids(), (std::vector<std::string>{"training-sprout"}));
	EXPECT_EQ(registries.aiBehaviours().ids(), (std::vector<std::string>{"training-aggressive"}));

	// The battle-board registry is real, transactional and deliberately empty: both bootstrap
	// encounters overlay the live world, and step 19 authors the first arena.
	EXPECT_EQ(registries.battleBoards().size(), 0U);
	EXPECT_FALSE(registries.battleBoards().contains("verdant-trial-arena"));

	const pg::EncounterDefinition &debug = registries.encounters().get("debug-battle");
	EXPECT_EQ(debug.kind, pg::EncounterKind::Debug);
	EXPECT_FALSE(debug.allowsTaming) << "a debug encounter cannot tame";
	EXPECT_TRUE(debug.repeatable);
	EXPECT_TRUE(std::holds_alternative<pg::LiveWorldBoardPolicyDefinition>(debug.board));
}

TEST(EncounterRegistriesTest, ResolvesEverySpawnReferenceAgainstTheGraphBeingPublished)
{
	pg::Registries registries;
	ASSERT_NO_THROW(registries.loadAll(pg::resourceRoot() / "data"));

	const pgtest::ScratchData unknownSpecies("unknown-species");
	unknownSpecies.write("encounter-tables", "training-wild", wildTable("ghost-sprout"));
	try
	{
		registries.loadAll(unknownSpecies.path());
		FAIL() << "an encounter that fields a species nobody defined must fail the load";
	} catch (const pg::JsonError &error)
	{
		EXPECT_EQ(error.file().filename(), std::filesystem::path("training-wild.json"));
		EXPECT_NE(error.message().find("ghost-sprout"), std::string::npos);
		EXPECT_NE(error.message().find("sprout-a"), std::string::npos) << "and it names the member";
	}

	const pgtest::ScratchData unknownAi("unknown-ai");
	unknownAi.write("encounter-tables", "training-wild", wildTable("training-sprout", "ghost-ai"));
	EXPECT_THROW(registries.loadAll(unknownAi.path()), pg::JsonError);

	// An illegal completed-node preset is caught at load, not at battle entry.
	const pgtest::ScratchData illegalPreset("illegal-preset");
	illegalPreset.write(
		"encounter-tables",
		"training-wild",
		wildTable("training-sprout", "training-aggressive", R"(["learn-snare"])"));
	EXPECT_THROW(registries.loadAll(illegalPreset.path()), pg::JsonError)
		<< "learn-snare gates on a completed node, so it cannot be the only one completed";

	const pgtest::ScratchData unknownNode("unknown-node");
	unknownNode.write(
		"encounter-tables",
		"training-wild",
		wildTable("training-sprout", "training-aggressive", R"(["ghost-node"])"));
	EXPECT_THROW(registries.loadAll(unknownNode.path()), pg::JsonError);

	// A legal preset does load, and the member derives what that preset grants.
	const pgtest::ScratchData legalPreset("legal-preset");
	legalPreset.write(
		"encounter-tables",
		"training-wild",
		wildTable("training-sprout", "training-aggressive", R"(["learn-guard"])"));
	EXPECT_NO_THROW(registries.loadAll(legalPreset.path()));
}

TEST(EncounterRegistriesTest, RefusesAnEnemyAuthoredToCastAMoveItDoesNotKnow)
{
	// training-guard is behind a Feat node, so a fresh sprout does not know it. An AI that casts it
	// would simply never fire that rule - a silent behaviour change, not an error the player sees.
	constexpr std::string_view GuardAi = R"({
		"version": 1,
		"displayNameKey": "ai.training-aggressive.name",
		"rules": [
			{"id": "guard", "conditions": [],
			 "decision": {"type": "castAbility", "ability": "training-guard",
						  "anchor": {"type": "unit", "selector": "self"}}},
			{"id": "finish", "conditions": [], "decision": {"type": "endTurn"}}
		]})";

	pg::Registries registries;
	const pgtest::ScratchData unknownMove("ai-casts-unknown-move");
	unknownMove.write("ai", "training-aggressive", GuardAi);
	try
	{
		registries.loadAll(unknownMove.path());
		FAIL() << "an enemy may not be authored to cast an ability its derived loadout does not hold";
	} catch (const pg::JsonError &error)
	{
		EXPECT_NE(error.message().find("training-guard"), std::string::npos);
	}

	// Completing the node that teaches it makes exactly the same AI legal.
	const pgtest::ScratchData taught("ai-casts-taught-move");
	taught.write("ai", "training-aggressive", GuardAi);
	taught.write(
		"encounter-tables",
		"training-wild",
		wildTable("training-sprout", "training-aggressive", R"(["learn-guard"])"));
	taught.write(
		"encounter-tables",
		"debug-battle",
		std::string(R"({"version": 1, "displayNameKey": "encounter.debug-battle.name", "kind": "debug",
			"allowsTaming": false, "repeatable": true, "triggerChancePermille": 1000,
			"board": {"type": "liveWorld", "size": [11, 11], "deploymentDepth": 2,
					  "opponentPlacement": {"type": "byLine", "rowsFromEnemyEdge": 0, "order": "centerOut"}},
			"tiers": [{"tier": 0, "teams": [{"id": "single-sprout",
				"displayNameKey": "encounter.debug-battle.single-sprout.name", "weight": 1,
				"members": [{"id": "sprout-a", "species": "training-sprout", "ai": "training-aggressive",
							 "completedFeatNodes": ["learn-guard"]}]}]}]})"));
	EXPECT_NO_THROW(registries.loadAll(taught.path()));
}

TEST(EncounterRegistriesTest, MigratesEveryBiomeToVersionTwoAndResolvesItsWildTable)
{
	pg::Registries registries;
	ASSERT_NO_THROW(registries.loadAll(pg::resourceRoot() / "data"));

	// Every generated biome leads its Bushes somewhere; the interior-only cave deliberately does not.
	for (const std::string &id : registries.biomes().ids())
	{
		const pg::BiomeDefinition &biome = registries.biomes().get(id);
		if (biome.worldgen.has_value())
		{
			ASSERT_TRUE(biome.wildEncounterTableId.has_value()) << id;
			const pg::EncounterDefinition &table = registries.encounters().get(*biome.wildEncounterTableId);
			EXPECT_EQ(table.kind, pg::EncounterKind::Wild);
			EXPECT_TRUE(table.repeatable);
		}
		else
		{
			EXPECT_FALSE(biome.wildEncounterTableId.has_value())
				<< id << " has no worldgen block, so nothing can trigger a wild encounter in it";
		}
	}

	// Version 1 is rejected outright: it is a biome nobody decided what a Bush leads to in.
	const pgtest::ScratchData legacy("legacy-biome");
	legacy.write("biomes", "meadow", meadow("1", "training-wild"));
	EXPECT_THROW(registries.loadAll(legacy.path()), pg::JsonError);

	const pgtest::ScratchData missingLink("missing-link");
	missingLink.write("biomes", "meadow", meadow("2", ""));
	EXPECT_THROW(registries.loadAll(missingLink.path()), pg::JsonError);

	const pgtest::ScratchData unknownTable("unknown-table");
	unknownTable.write("biomes", "meadow", meadow("2", "ghost-table"));
	EXPECT_THROW(registries.loadAll(unknownTable.path()), pg::JsonError);

	// A Bush leads to wild creatures, so it may not point at the debug battle.
	const pgtest::ScratchData wrongKind("wrong-kind");
	wrongKind.write("biomes", "meadow", meadow("2", "debug-battle"));
	try
	{
		registries.loadAll(wrongKind.path());
		FAIL() << "a biome may only reference a repeatable wild encounter";
	} catch (const pg::JsonError &error)
	{
		EXPECT_NE(error.message().find("debug-battle"), std::string::npos);
	}
}

TEST(EncounterRegistriesTest, ABrokenDefinitionAnywhereLeavesThePreviousRegistriesUntouched)
{
	pg::Registries registries;
	ASSERT_NO_THROW(registries.loadAll(pg::resourceRoot() / "data"));

	const std::vector<std::string> speciesIds = registries.species().ids();
	const std::vector<std::string> encounterIds = registries.encounters().ids();
	const std::vector<std::string> aiIds = registries.aiBehaviours().ids();
	const std::size_t boardCount = registries.battleBoards().size();
	const std::size_t starters = registries.newGame().starterTeam.size();

	// Each of these parses everything else perfectly and fails on one thing. Nothing may publish.
	const pgtest::ScratchData badEncounter("bad-encounter");
	badEncounter.write("encounter-tables", "training-wild", wildTable("ghost-sprout"));
	EXPECT_THROW(registries.loadAll(badEncounter.path()), std::exception);

	const pgtest::ScratchData badBoard("bad-battle-board");
	badBoard.write(
		"battle-boards",
		"arena",
		R"({"version": 1, "displayNameKey": "battle-board.arena.name", "size": [13, 3, 13],
		   "geometryPrefab": "ghost-geometry", "playerApproach": "positiveZ", "deploymentDepth": 2})");
	EXPECT_THROW(registries.loadAll(badBoard.path()), std::exception);

	const pgtest::ScratchData badStarter("bad-starter");
	badStarter.writeFile(
		"config/new-game.json",
		R"({"version": 1, "starterTeam": [{"species": "ghost-sprout", "completedFeatNodes": []}]})");
	EXPECT_THROW(registries.loadAll(badStarter.path()), std::exception);

	const pgtest::ScratchData badBiome("bad-biome-link");
	badBiome.write("biomes", "meadow", meadow("2", "ghost-table"));
	EXPECT_THROW(registries.loadAll(badBiome.path()), std::exception);

	EXPECT_EQ(registries.species().ids(), speciesIds);
	EXPECT_EQ(registries.encounters().ids(), encounterIds);
	EXPECT_EQ(registries.aiBehaviours().ids(), aiIds);
	EXPECT_EQ(registries.battleBoards().size(), boardCount);
	EXPECT_EQ(registries.newGame().starterTeam.size(), starters);
	EXPECT_TRUE(registries.species().contains("training-sprout"));
}
