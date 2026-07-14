#include <gtest/gtest.h>

#include "creatures/creature_state_derivation.hpp"
#include "creatures/creature_unit.hpp"
#include "feats/feat_board_progress.hpp"
#include "support/json_fixture.hpp"

#include <array>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace
{
	// A hand-built definition graph. The derivation only ever asks these registries whether an id
	// exists, so a case can author the exact board it is about without a resource tree.
	class Graph
	{
	private:
		std::vector<std::unique_ptr<pgtest::Document>> _documents;

	public:
		pg::GameRules gameRules;
		pg::Registry<pg::StatusDefinition> statuses;
		pg::Registry<pg::AbilityDefinition> abilities;
		pg::Registry<pg::FeatBoardDefinition> featBoards;
		pg::Registry<pg::CreatureSpeciesDefinition> species;

		Graph()
		{
			gameRules.battle.minimumStamina = pg::BattleTime::fromTicks(100);
			gameRules.battle.abilitySlotCapacity = 8;

			for (const std::string_view id : {"training-strike", "training-guard", "training-snare", "extra-move"})
			{
				pg::AbilityDefinition ability;
				ability.id = id;
				abilities.add(std::string(id), std::move(ability));
			}

			pg::StatusDefinition thorns;
			thorns.id = "thorns";
			thorns.tags = {"buff"};
			statuses.add("thorns", std::move(thorns));

			pg::StatusDefinition dazed;
			dazed.id = "dazed";
			dazed.tags = {"stun"};
			statuses.add("dazed", std::move(dazed));
		}

		[[nodiscard]] pg::DerivationContext context() const
		{
			return pg::DerivationContext{
				.gameRules = &gameRules,
				.statuses = &statuses,
				.abilities = &abilities,
				.featBoards = &featBoards,
				.species = &species};
		}

		void addBoard(std::string_view p_id, std::string_view p_document)
		{
			_documents.push_back(std::make_unique<pgtest::Document>(p_document, std::string(p_id) + ".json"));
			pg::JsonReader reader = _documents.back()->reader();
			pg::FeatBoardDefinition board = pg::parseFeatBoardDefinition(reader, pg::conditionLimits(gameRules));
			board.id = p_id;
			featBoards.add(std::string(p_id), std::move(board));
		}

		void addSpecies(std::string_view p_id, std::string_view p_document)
		{
			_documents.push_back(std::make_unique<pgtest::Document>(p_document, std::string(p_id) + ".json"));
			pg::JsonReader reader = _documents.back()->reader();
			pg::CreatureSpeciesDefinition definition =
				pg::parseCreatureSpeciesDefinition(reader, gameRules, pg::conditionLimits(gameRules));
			definition.id = p_id;
			species.add(std::string(p_id), std::move(definition));
		}
	};

	[[nodiscard]] std::string requirement(std::string_view p_id)
	{
		return std::string(R"({"id": ")") + std::string(p_id) + R"(", "type": "movement",
			"descriptionKey": "feat.fixture.description", "window": "fight", "windowActor": "any",
			"requiredWindowCount": 1, "windowMode": "cumulative", "actor": "subject",
			"movement": "voluntary", "direction": "any", "aggregation": "sum",
			"comparison": "atLeast", "threshold": 10})";
	}

	[[nodiscard]] std::string node(
		std::string_view p_id,
		std::string_view p_kind,
		std::string_view p_neighbours,
		std::string_view p_rewards,
		std::string_view p_extra = "")
	{
		return std::string(R"({"id": ")") + std::string(p_id) + R"(", "displayNameKey": "feat.fixture.name",
			"descriptionKey": "feat.fixture.description", "position": [0, 0], "kind": ")" +
			   std::string(p_kind) + R"(", "neighbours": )" + std::string(p_neighbours) +
			   R"(, "minimumCompletedNodes": 0)" + std::string(p_extra) + R"(, "requirements": [)" +
			   (p_kind == "root" ? std::string() : requirement(std::string(p_id) + "-req")) + R"(], "rewards": [)" +
			   std::string(p_rewards) + "]}";
	}

	[[nodiscard]] std::string board(std::string_view p_nodes)
	{
		return std::string(R"({"version": 1, "displayNameKey": "feat-board.fixture.name", "rootNode": "root",
			"nodes": [)") +
			   std::string(p_nodes) + "]}";
	}

	constexpr std::string_view Attributes = R"("attributes": {
		"maxHealth": 24, "strength": 6, "magicPower": 3, "armor": 2, "resistance": 2,
		"maxActionPoints": 6, "maxMovementPoints": 4, "staminaSeconds": 4.0, "range": 0})";

	[[nodiscard]] std::string speciesDocument(
		std::string_view p_forms = R"([{"id": "base", "displayNameKey": "creature.fixture.form.base.name", "tier": 0,
			"presentation": {"tint": [1, 2, 3, 255], "scalePermille": 1000}}])",
		std::string_view p_abilities = R"(["training-strike"])",
		std::string_view p_passives = "[]")
	{
		return std::string(R"({"version": 1, "displayNameKey": "creature.fixture.name",
			"descriptionKey": "creature.fixture.description", )") +
			   std::string(Attributes) + R"(, "defaultAbilities": )" + std::string(p_abilities) +
			   R"(, "defaultPassives": )" + std::string(p_passives) +
			   R"(, "featBoard": "fixture-board", "defaultForm": "base", "forms": )" + std::string(p_forms) + "}";
	}

	// A board whose earned nodes cover every reward kind, in a deliberate order.
	[[nodiscard]] std::string rewardBoard()
	{
		return board(
			node("root", "root", R"(["stats"])", R"({"id": "root-hp", "type": "bonusStat", "stat": "maxHealth", "amount": 1})") +
			"," +
			node(
				"stats",
				"stats",
				R"(["root", "learn"])",
				R"({"id": "hp", "type": "bonusStat", "stat": "maxHealth", "amount": 6},
				   {"id": "faster", "type": "bonusStat", "stat": "stamina", "amountSeconds": -0.5})") +
			"," +
			node(
				"learn",
				"ability",
				R"(["stats", "forget"])",
				R"({"id": "learn-guard", "type": "unlockAbility", "ability": "training-guard"},
				   {"id": "learn-snare", "type": "unlockAbility", "ability": "training-snare"})") +
			"," +
			node(
				"forget",
				"ability",
				R"(["learn", "passive"])",
				R"({"id": "forget-strike", "type": "removeAbility", "ability": "training-strike"})") +
			"," +
			node(
				"passive",
				"passive",
				R"(["forget"])",
				R"({"id": "thorny", "type": "unlockPassive", "status": "thorns"})"));
	}
}

TEST(CreatureStateDerivationTest, FreshProgressCompletesTheRootAndNothingElse)
{
	Graph graph;
	graph.addBoard("fixture-board", rewardBoard());

	const pg::FeatBoardProgress progress = pg::makeFreshFeatBoardProgress(graph.featBoards.get("fixture-board"));
	EXPECT_EQ(progress.boardId, "fixture-board");
	ASSERT_EQ(progress.nodes.size(), 5U);

	// Board definition order, and every node has an entry from the start.
	EXPECT_EQ(progress.nodes[0].nodeId, "root");
	EXPECT_TRUE(progress.nodes[0].completed) << "the root has no requirement, so it starts completed";
	for (std::size_t index = 1; index < progress.nodes.size(); ++index)
	{
		EXPECT_FALSE(progress.nodes[index].completed);
		ASSERT_EQ(progress.nodes[index].requirements.size(), 1U);
		// Keyed by the authored condition id, never by an array index.
		ASSERT_EQ(progress.nodes[index].requirements[0].conditionAdvancement.size(), 1U);
		EXPECT_EQ(
			progress.nodes[index].requirements[0].conditionAdvancement[0].conditionId,
			progress.nodes[index].requirements[0].requirementId);
		EXPECT_FALSE(progress.nodes[index].requirements[0].conditionAdvancement[0].completed);
		EXPECT_EQ(progress.nodes[index].requirements[0].conditionAdvancement[0].qualifyingWindows, 0U);
	}
	EXPECT_TRUE(progress.chosenExclusiveGroups.empty());
}

TEST(CreatureStateDerivationTest, GivesEveryNestedCompositeChildItsOwnConditionEntry)
{
	Graph graph;
	graph.addBoard(
		"fixture-board",
		board(
			node("root", "root", R"(["earned"])", "") + "," +
			std::string(R"({"id": "earned", "displayNameKey": "feat.fixture.name",
				"descriptionKey": "feat.fixture.description", "position": [1, 0], "kind": "stats",
				"neighbours": ["root"], "minimumCompletedNodes": 0, "requirements": [
					{"id": "either", "type": "anyOf", "descriptionKey": "feat.fixture.description",
					 "children": [)") +
			requirement("walk") + "," + requirement("run") +
			R"(]}], "rewards": [{"id": "hp", "type": "bonusStat", "stat": "maxHealth", "amount": 2}]})"));

	const pg::FeatBoardProgress progress = pg::makeFreshFeatBoardProgress(graph.featBoards.get("fixture-board"));
	ASSERT_EQ(progress.nodes.size(), 2U);
	ASSERT_EQ(progress.nodes[1].requirements.size(), 1U);

	// The composite keeps an entry of its own, and so does each child: "damage or heal" has to be
	// able to say which half is already done.
	const std::vector<pg::PersistentConditionAdvancement> &advancement =
		progress.nodes[1].requirements[0].conditionAdvancement;
	ASSERT_EQ(advancement.size(), 3U);
	EXPECT_EQ(advancement[0].conditionId, "either");
	EXPECT_EQ(advancement[1].conditionId, "walk");
	EXPECT_EQ(advancement[2].conditionId, "run");
}

TEST(CreatureStateDerivationTest, ReplaysEveryRewardKindInBoardThenRewardOrder)
{
	Graph graph;
	graph.addBoard("fixture-board", rewardBoard());
	graph.addSpecies("sprout", speciesDocument());

	const pg::CreatureSpeciesDefinition &species = graph.species.get("sprout");
	const pg::FeatBoardDefinition &featBoard = graph.featBoards.get("fixture-board");

	// The baseline plus the root's own reward, and nothing else.
	const pg::DerivedCreatureState fresh =
		pg::deriveCreatureState(species, featBoard, pg::makeFreshFeatBoardProgress(featBoard), graph.context());
	EXPECT_EQ(fresh.attributes.maxHealth, 25) << "24 base + 1 from the root, which starts completed";
	EXPECT_EQ(fresh.attributes.stamina.ticks(), 4000);
	EXPECT_EQ(fresh.abilityIds, (std::vector<std::string>{"training-strike"}));
	EXPECT_TRUE(fresh.passiveStatusIds.empty());
	EXPECT_EQ(fresh.formId, "base");

	const std::vector<std::string> everything{"stats", "learn", "forget", "passive"};
	const pg::FeatBoardProgress progress = pg::makePresetFeatBoardProgress(featBoard, "base", everything);
	const pg::DerivedCreatureState derived = pg::deriveCreatureState(species, featBoard, progress, graph.context());

	EXPECT_EQ(derived.attributes.maxHealth, 31) << "24 base + 1 root + 6";
	EXPECT_EQ(derived.attributes.stamina.ticks(), 3500) << "a negative stamina reward makes the creature faster";
	// 'learn' appended two abilities, then 'forget' erased the default one: the order is the board's.
	EXPECT_EQ(derived.abilityIds, (std::vector<std::string>{"training-guard", "training-snare"}));
	EXPECT_EQ(derived.passiveStatusIds, (std::vector<std::string>{"thorns"}));
	EXPECT_EQ(derived.formId, "base") << "no evolution node was completed";
}

TEST(CreatureStateDerivationTest, IsPureIdempotentAndIndependentPerCreature)
{
	Graph graph;
	graph.addBoard("fixture-board", rewardBoard());
	graph.addSpecies("sprout", speciesDocument());

	const pg::CreatureSpeciesDefinition &species = graph.species.get("sprout");
	const pg::FeatBoardDefinition &featBoard = graph.featBoards.get("fixture-board");
	const std::vector<std::string> completed{"stats"};

	const pg::FeatBoardProgress progress = pg::makePresetFeatBoardProgress(featBoard, "base", completed);
	const pg::DerivedCreatureState first = pg::deriveCreatureState(species, featBoard, progress, graph.context());
	const pg::DerivedCreatureState second = pg::deriveCreatureState(species, featBoard, progress, graph.context());
	EXPECT_EQ(first, second) << "the replay is pure: same inputs, byte-identical result, no RNG anywhere";

	// Two creatures of one species differ only by what they have done.
	const pg::CreatureUnit veteran =
		pg::makeCreatureUnit(pg::CreatureInstanceId::fromSerial(1), "sprout", completed, graph.context());
	const pg::CreatureUnit rookie =
		pg::makeCreatureUnit(pg::CreatureInstanceId::fromSerial(2), "sprout", std::vector<std::string>{}, graph.context());

	EXPECT_EQ(veteran.derived.attributes.maxHealth, 31);
	EXPECT_EQ(rookie.derived.attributes.maxHealth, 25) << "no level, no roll: the rookie is on the same baseline";
	EXPECT_EQ(rookie.derived.attributes.strength, veteran.derived.attributes.strength);
	EXPECT_TRUE(rookie.featProgress.isCompleted("root"));
	EXPECT_FALSE(rookie.featProgress.isCompleted("stats"));
	EXPECT_TRUE(veteran.featProgress.isCompleted("stats"));

	// The cache is a cache: drop it and it comes back identical.
	pg::CreatureUnit rebuilt = veteran;
	rebuilt.derived = pg::DerivedCreatureState{};
	pg::rebuildDerivedState(rebuilt, graph.context());
	EXPECT_EQ(rebuilt.derived, veteran.derived);
	EXPECT_EQ(rebuilt, veteran);
}

TEST(CreatureStateDerivationTest, ReplaysAnEvolutionAndRefusesAnIllegalPreset)
{
	Graph graph;
	graph.addBoard(
		"fixture-board",
		board(
			node("root", "root", R"(["evolve"])", "") + "," +
			node(
				"evolve",
				"evolution",
				R"(["root", "late"])",
				R"({"id": "grow", "type": "changeForm", "form": "grown"})") +
			"," +
			// Only reachable once the creature wears the grown form.
			node(
				"late",
				"stats",
				R"(["evolve"])",
				R"({"id": "grown-hp", "type": "bonusStat", "stat": "maxHealth", "amount": 10})",
				R"(, "fromForm": "grown")")));
	graph.addSpecies(
		"sprout",
		speciesDocument(
			R"([{"id": "base", "displayNameKey": "creature.fixture.form.base.name", "tier": 0,
				"presentation": {"tint": [1, 2, 3, 255], "scalePermille": 1000}},
			   {"id": "grown", "displayNameKey": "creature.fixture.form.grown.name", "tier": 1,
				"presentation": {"tint": [4, 5, 6, 255], "scalePermille": 1400}}])"));

	const pg::CreatureSpeciesDefinition &species = graph.species.get("sprout");
	const pg::FeatBoardDefinition &featBoard = graph.featBoards.get("fixture-board");

	// The solver has to find the one legal order - evolve, then the node that gates on the new form -
	// whatever order the author listed them in.
	const pg::FeatBoardProgress progress =
		pg::makePresetFeatBoardProgress(featBoard, "base", std::vector<std::string>{"late", "evolve"});
	const pg::DerivedCreatureState derived = pg::deriveCreatureState(species, featBoard, progress, graph.context());
	EXPECT_EQ(derived.formId, "grown");
	EXPECT_EQ(derived.attributes.maxHealth, 34);

	// Completing the gated node without the evolution that unlocks it is not a legal state.
	EXPECT_THROW(
		auto value = pg::makePresetFeatBoardProgress(featBoard, "base", std::vector<std::string>{"late"}),
		std::invalid_argument);
	// Nor is a node nothing completed is adjacent to, an unknown node, a duplicate, or the root.
	EXPECT_THROW(
		auto value = pg::makePresetFeatBoardProgress(featBoard, "base", std::vector<std::string>{"ghost"}),
		std::invalid_argument);
	EXPECT_THROW(
		auto value = pg::makePresetFeatBoardProgress(featBoard, "base", std::vector<std::string>{"evolve", "evolve"}),
		std::invalid_argument);
	EXPECT_THROW(
		auto value = pg::makePresetFeatBoardProgress(featBoard, "base", std::vector<std::string>{"root"}),
		std::invalid_argument)
		<< "the root is implicit; listing it would be a second spelling of the same fact";
}

TEST(CreatureStateDerivationTest, RefusesTwoBranchesOfOneExclusiveGroupAndRecordsTheChoice)
{
	Graph graph;
	graph.addBoard(
		"fixture-board",
		board(
			node("root", "root", R"(["left", "right"])", "") + "," +
			node(
				"left",
				"evolution",
				R"(["root"])",
				R"({"id": "go-left", "type": "changeForm", "form": "leafy"})",
				R"(, "exclusiveGroup": "path")") +
			"," +
			node(
				"right",
				"evolution",
				R"(["root"])",
				R"({"id": "go-right", "type": "changeForm", "form": "thorny"})",
				R"(, "exclusiveGroup": "path")")));
	graph.addSpecies(
		"sprout",
		speciesDocument(
			R"([{"id": "base", "displayNameKey": "creature.fixture.form.base.name", "tier": 0,
				"presentation": {"tint": [1, 2, 3, 255], "scalePermille": 1000}},
			   {"id": "leafy", "displayNameKey": "creature.fixture.form.leafy.name", "tier": 1,
				"presentation": {"tint": [4, 5, 6, 255], "scalePermille": 1200}},
			   {"id": "thorny", "displayNameKey": "creature.fixture.form.thorny.name", "tier": 1,
				"presentation": {"tint": [7, 8, 9, 255], "scalePermille": 1200}}])"));

	const pg::FeatBoardDefinition &featBoard = graph.featBoards.get("fixture-board");

	const pg::FeatBoardProgress chose =
		pg::makePresetFeatBoardProgress(featBoard, "base", std::vector<std::string>{"left"});
	// Reconstructed from the completed nodes rather than trusted from a map the author could have
	// contradicted.
	ASSERT_EQ(chose.chosenExclusiveGroups.size(), 1U);
	EXPECT_EQ(chose.chosenExclusiveGroups.at("path"), "left");
	EXPECT_EQ(
		pg::deriveCreatureState(graph.species.get("sprout"), featBoard, chose, graph.context()).formId,
		"leafy");

	EXPECT_THROW(
		auto value = pg::makePresetFeatBoardProgress(featBoard, "base", std::vector<std::string>{"left", "right"}),
		std::invalid_argument)
		<< "completing one member of an exclusive group blocks every other, permanently";
}

TEST(CreatureStateDerivationTest, DerivationRejectsAnIllegalResultRatherThanClampingIt)
{
	Graph graph;
	// The board is legal on its own; it is this species' baseline that makes the result illegal.
	graph.addBoard(
		"fixture-board",
		board(
			node("root", "root", R"(["drain"])", "") + "," +
			node(
				"drain",
				"stats",
				R"(["root"])",
				R"({"id": "lose-hp", "type": "bonusStat", "stat": "maxHealth", "amount": -1000})")));
	graph.addSpecies("sprout", speciesDocument());

	const pg::CreatureSpeciesDefinition &species = graph.species.get("sprout");
	const pg::FeatBoardDefinition &featBoard = graph.featBoards.get("fixture-board");
	const pg::FeatBoardProgress progress =
		pg::makePresetFeatBoardProgress(featBoard, "base", std::vector<std::string>{"drain"});

	// 24 - 1000 is not a creature with 1 HP; it is a definition error, and it says where.
	try
	{
		const pg::DerivedCreatureState derived = pg::deriveCreatureState(species, featBoard, progress, graph.context());
		FAIL() << "a derived maxHealth of " << derived.attributes.maxHealth << " must fail rather than clamp";
	} catch (const pg::JsonError &error)
	{
		EXPECT_NE(error.message().find("maxHealth"), std::string::npos);
		EXPECT_NE(error.message().find("sprout"), std::string::npos);
	}
}
