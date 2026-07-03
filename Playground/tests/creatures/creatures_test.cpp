#include "animation/model_definition.hpp"
#include "battle/battle_context.hpp"
#include "core/game_context.hpp"
#include "core/json.hpp"
#include "core/registries.hpp"
#include "creatures/apply_progress.hpp"
#include "creatures/creature_species.hpp"
#include "creatures/creature_unit.hpp"
#include "creatures/player_data.hpp"
#include "encounters/encounter_service.hpp"
#include "world/voxel_world.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <memory>

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

	[[nodiscard]] nlohmann::json sproutJson()
	{
		return pg::JsonLoader::parseFile(
			std::filesystem::path(PG_RESOURCE_DIR) / "data" / "creatures" / "sprout.json");
	}
}

TEST(CreatureSpeciesParser, ResolvesAbilitiesAndFormsFromAuthoredData)
{
	const pg::CreatureSpecies &sprout = registries().creatures().get("sprout");
	EXPECT_EQ(sprout.displayName, "Sprout");
	EXPECT_EQ(sprout.attributes.health, 12);
	ASSERT_EQ(sprout.defaultAbilities.size(), 4);
	EXPECT_EQ(sprout.defaultAbilities.front(), &registries().abilities().get("tackle"));
	EXPECT_EQ(sprout.defaultAbilities[2], &registries().abilities().get("bulwark"));
	EXPECT_EQ(sprout.form(sprout.defaultFormId).modelId, "placeholder-cube");

	const pg::CreatureSpecies &emberFox = registries().creatures().get("ember-fox");
	EXPECT_NE(emberFox.attributes.stamina, sprout.attributes.stamina);
	ASSERT_EQ(emberFox.defaultAbilities.size(), 4);
	EXPECT_EQ(emberFox.defaultAbilities.front(), &registries().abilities().get("spark"));
	EXPECT_EQ(emberFox.defaultAbilities[1], &registries().abilities().get("ember"));
}

TEST(CreatureSpeciesParser, RejectsUnknownAbilityAndDefaultForm)
{
	nlohmann::json unknownAbility = sproutJson();
	unknownAbility["defaultAbilities"] = {"missing"};
	pg::JsonReader abilityReader(unknownAbility, "creature-test.json");
	EXPECT_THROW((void)pg::parseCreatureSpecies(abilityReader, registries().abilities()), pg::JsonError);

	nlohmann::json unknownForm = sproutJson();
	unknownForm["defaultFormId"] = "missing";
	pg::JsonReader formReader(unknownForm, "creature-test.json");
	EXPECT_THROW((void)pg::parseCreatureSpecies(formReader, registries().abilities()), pg::JsonError);
}

TEST(ModelDefinitionParser, BuildsPartMeshesAndRejectsInvertedBoxes)
{
	const pg::ModelDefinition &model = registries().models().get("placeholder-cube");
	ASSERT_EQ(model.parts.size(), 1);
	ASSERT_NE(model.parts.front().mesh, nullptr);
	EXPECT_EQ(model.parts.front().mesh->nbShape(), 6);
	float minimumX = 1.0f;
	float maximumX = -1.0f;
	float minimumY = 1.0f;
	float maximumY = -1.0f;
	float minimumZ = 1.0f;
	float maximumZ = -1.0f;
	for (const spk::TextureVertex3D &vertex : model.parts.front().mesh->vertices())
	{
		minimumX = std::min(minimumX, vertex.position.x);
		maximumX = std::max(maximumX, vertex.position.x);
		minimumY = std::min(minimumY, vertex.position.y);
		maximumY = std::max(maximumY, vertex.position.y);
		minimumZ = std::min(minimumZ, vertex.position.z);
		maximumZ = std::max(maximumZ, vertex.position.z);
	}
	EXPECT_FLOAT_EQ(minimumX, -0.5f);
	EXPECT_FLOAT_EQ(maximumX, 0.5f);
	EXPECT_FLOAT_EQ(minimumZ, -0.5f);
	EXPECT_FLOAT_EQ(maximumZ, 0.5f);
	EXPECT_FLOAT_EQ(minimumY, -0.5f);
	EXPECT_FLOAT_EQ(maximumY, 0.5f);

	nlohmann::json invalid = pg::JsonLoader::parseFile(
		std::filesystem::path(PG_RESOURCE_DIR) / "data" / "models" / "placeholder-cube.json");
	invalid["parts"][0]["voxels"][0]["max"] = {0, 1, 1};
	pg::JsonReader reader(invalid, "model-test.json");
	EXPECT_THROW((void)pg::parseModelDefinition(reader), pg::JsonError);
}

TEST(ApplyProgress, ResetsEveryDerivedFieldAndPreservesRegistryPointerIdentity)
{
	const pg::CreatureSpecies &species = registries().creatures().get("sprout");
	pg::CreatureUnit creature(species);
	creature.attributes.health = 999;
	creature.abilities.clear();
	creature.currentFormId = "corrupted";
	creature.permanentPassives.push_back(nullptr);

	pg::applyProgress(creature);

	EXPECT_EQ(creature.attributes.health, species.attributes.health);
	EXPECT_EQ(creature.currentFormId, species.defaultFormId);
	EXPECT_EQ(creature.abilities, species.defaultAbilities);
	EXPECT_TRUE(creature.permanentPassives.empty());
	EXPECT_EQ(creature.abilities.front(), &registries().abilities().get("tackle"));
}

TEST(PlayerData, FillsSixTeamSlotsThenUsesUnboundedStorage)
{
	pg::PlayerData player;
	const pg::CreatureSpecies &species = registries().creatures().get("sprout");
	for (std::size_t index = 0; index < pg::PlayerData::TeamCapacity + 2; ++index)
	{
		player.addCreatureToTeamOrStorage(std::make_unique<pg::CreatureUnit>(species));
	}
	EXPECT_EQ(player.teamSize(), 6);
	EXPECT_EQ(player.storage.size(), 2);
}

TEST(BattleUnit, SourcesAttributesAndIdentityFromCreature)
{
	pg::CreatureUnit creature(registries().creatures().get("ember-fox"));
	pg::BattleUnit unit(&creature, pg::BattleSide::Player);
	EXPECT_EQ(unit.source(), &creature);
	EXPECT_EQ(unit.displayName(), "Ember Fox");
	EXPECT_EQ(unit.attributes.hp.max(), creature.attributes.health);
	EXPECT_FLOAT_EQ(unit.attributes.turnBar.max(), creature.attributes.stamina);
}

TEST(GameContext, NewGameStartsWithTwoAuthoredSpecies)
{
	pg::GameContext game;
	game.newGame(registries());
	ASSERT_EQ(game.player.teamSize(), 2);
	EXPECT_EQ(game.player.team[0]->species, &registries().creatures().get("sprout"));
	EXPECT_EQ(game.player.team[1]->species, &registries().creatures().get("ember-fox"));
}

TEST(EncounterService, InstantiatesBothSidesFromCreatureSpecies)
{
	pg::GameContext game;
	game.newGame(registries());
	game.world.world = std::make_unique<pg::VoxelWorld>(registries().voxels());
	game.world.world->loadFromMap(registries().maps().get("m1-testground"));
	const spk::Vector3Int center{32, 3, 40};
	pg::EncounterService service(game, registries(), [center] {
		return center;
	});
	pg::EncounterSpawn spawn{
		.displayName = "wild sprout",
		.enemyTeam = {{.speciesId = "sprout"}},
		.boardSize = {7, 7}};
	game.events.encounterTriggered.trigger(spawn);

	ASSERT_NE(service.activeBattle(), nullptr);
	ASSERT_EQ(service.activeBattle()->getUnits(pg::BattleSide::Player).size(), 2);
	ASSERT_EQ(service.activeBattle()->getUnits(pg::BattleSide::Enemy).size(), 1);
	EXPECT_EQ(service.activeBattle()->getUnits(pg::BattleSide::Enemy).front()->source()->species, &registries().creatures().get("sprout"));
}

TEST(FeatBoardProgressStub, RoundTripsUnknownPayloadUntilStep17)
{
	pg::FeatBoardProgress progress;
	const nlohmann::json payload = {{"futureNode", {{"completionCount", 2}}}};
	progress.fromJson(payload);
	EXPECT_EQ(progress.toJson(), payload);
}
