#include "battle/battle_event.hpp"
#include "battle/battle_unit.hpp"
#include "core/json.hpp"
#include "core/registries.hpp"
#include "creatures/apply_progress.hpp"
#include "creatures/creature_unit.hpp"
#include "feats/battle_condition.hpp"
#include "feats/feat_board.hpp"
#include "feats/feat_progress.hpp"
#include "feats/feat_requirement.hpp"

#include <gtest/gtest.h>

#include <filesystem>

namespace
{
	class AmountCondition final : public pg::BattleCondition
	{
	protected:
		float evaluateEventProgress(const pg::BattleEvent &p_event, const pg::BattleUnit *) const override
		{
			const pg::DamageEvent *damage = p_event.getIf<pg::DamageEvent>();
			return damage != nullptr ? static_cast<float>(damage->amount) : 0.0f;
		}
	};

	const pg::Registries &registries()
	{
		static const pg::Registries result = [] {
			pg::Registries value; value.loadAll(std::filesystem::path(PG_RESOURCE_DIR) / "data"); return value;
		}();
		return result;
	}

	std::vector<const pg::BattleEvent *> pointers(const std::vector<pg::BattleEvent> &p_events)
	{
		std::vector<const pg::BattleEvent *> result;
		for (const pg::BattleEvent &event : p_events) result.push_back(&event);
		return result;
	}

	const pg::FeatNode &namedNode(const pg::FeatBoard &p_board, const std::string &p_name)
	{
		const auto found = std::ranges::find_if(p_board.nodes, [&](const pg::FeatNode &p_node) { return p_node.displayName == p_name; });
		if (found == p_board.nodes.end()) throw std::logic_error("missing test feat node");
		return *found;
	}
}

TEST(BattleCondition, AppliesAbilityTurnFightAndGameWindowSemantics)
{
	std::vector<pg::BattleEvent> events = {
		pg::DamageEvent{.context = {.turnIndex = 1}, .amount = 60},
		pg::DamageEvent{.context = {.turnIndex = 1}, .amount = 40},
		pg::DamageEvent{.context = {.turnIndex = 2}, .amount = 50}};
	const auto eventPointers = pointers(events);

	AmountCondition ability; ability.scope = pg::BattleCondition::Scope::Ability; ability.requiredRepeatCount = 2;
	auto advancement = ability.evaluateEvents(eventPointers, {});
	EXPECT_EQ(advancement.completedRepeatCount, 0); EXPECT_FLOAT_EQ(advancement.progress, 60.0f);

	AmountCondition turn; turn.scope = pg::BattleCondition::Scope::Turn; turn.requiredRepeatCount = 2;
	advancement = turn.evaluateEvents(eventPointers, {});
	EXPECT_EQ(advancement.completedRepeatCount, 1); EXPECT_FLOAT_EQ(advancement.progress, 50.0f);

	AmountCondition fight; fight.scope = pg::BattleCondition::Scope::Fight; fight.requiredRepeatCount = 3;
	advancement = fight.evaluateEvents(eventPointers, {});
	EXPECT_EQ(advancement.completedRepeatCount, 1); EXPECT_FLOAT_EQ(advancement.progress, 50.0f);

	AmountCondition game; game.scope = pg::BattleCondition::Scope::Game; game.requiredRepeatCount = 3;
	advancement = game.evaluateEvents(std::span(eventPointers).first(1), {});
	advancement = game.evaluateEvents(std::span(eventPointers).subspan(1), advancement);
	EXPECT_EQ(advancement.completedRepeatCount, 1); EXPECT_FLOAT_EQ(advancement.progress, 50.0f);
}

TEST(FeatRequirementFactory, ParsesAndOrCombinators)
{
	nlohmann::json json = {
		{"type", "and"},
		{"children", {
			{{"type", "dealDamage"}, {"scope", "fight"}, {"requiredAmount", 10}, {"damageKind", "any"}, {"sourceAbilities", nlohmann::json::array()}},
			{{"type", "healHealth"}, {"scope", "fight"}, {"requiredAmount", 5}}
		}}
	};
	pg::JsonReader reader(json, "condition-test.json");
	std::unique_ptr<pg::FeatRequirement> condition = pg::parseFeatRequirement(reader);
	std::vector<pg::BattleEvent> events = {
		pg::DamageEvent{.amount = 10}, pg::HealEvent{.amount = 5}};
	const auto eventPointers = pointers(events);
	EXPECT_TRUE(condition->isComplete(condition->evaluateEvents(eventPointers, {})));
}

TEST(FeatRequirementFactory, DistinguishesDamageCasterAndTargetRoles)
{
	const pg::CreatureSpecies &species = registries().creatures().get("sprout");
	pg::CreatureUnit casterCreature(species);
	pg::CreatureUnit targetCreature(species);
	pg::BattleUnit caster(&casterCreature, pg::BattleSide::Player);
	pg::BattleUnit target(&targetCreature, pg::BattleSide::Enemy);
	std::vector<pg::BattleEvent> events = {pg::DamageEvent{
		.context = {.turnIndex = 1, .caster = &caster, .target = &target},
		.amount = 30,
		.kind = pg::DamageKind::Physical}};
	const auto eventPointers = pointers(events);

	nlohmann::json dealJson = {{"type", "dealDamage"}, {"scope", "fight"}, {"requiredAmount", 30}, {"damageKind", "any"}, {"sourceAbilities", nlohmann::json::array()}};
	pg::JsonReader dealReader(dealJson, "condition-test.json");
	std::unique_ptr<pg::FeatRequirement> deal = pg::parseFeatRequirement(dealReader);
	EXPECT_TRUE(deal->isComplete(deal->evaluateEvents(eventPointers, {}, &caster)));
	EXPECT_FALSE(deal->isComplete(deal->evaluateEvents(eventPointers, {}, &target)));

	nlohmann::json takeJson = {{"type", "takeDamage"}, {"scope", "fight"}, {"requiredAmount", 30}, {"damageKind", "any"}};
	pg::JsonReader takeReader(takeJson, "condition-test.json");
	std::unique_ptr<pg::FeatRequirement> take = pg::parseFeatRequirement(takeReader);
	EXPECT_FALSE(take->isComplete(take->evaluateEvents(eventPointers, {}, &caster)));
	EXPECT_TRUE(take->isComplete(take->evaluateEvents(eventPointers, {}, &target)));
}

TEST(BattleEvent, StoresOnlyTheConcretePayloadAndSharedContext)
{
	pg::BattleEvent event = pg::DamageEvent{.context = {.turnIndex = 7}, .amount = 12, .kind = pg::DamageKind::Magical};
	EXPECT_EQ(pg::battleEventType(event), pg::BattleEventType::Damage);
	EXPECT_EQ(pg::battleEventName(event), "Damage");
	EXPECT_EQ(pg::battleEventContext(event).turnIndex, 7);
	ASSERT_NE(event.getIf<pg::DamageEvent>(), nullptr);
	EXPECT_EQ(event.getIf<pg::DamageEvent>()->amount, 12);
	EXPECT_EQ(event.getIf<pg::HealEvent>(), nullptr);
}

TEST(FeatRegistry, LoadsBoardsAndMaintainsGlobalNodeLookup)
{
	const pg::FeatBoard &sprout = registries().featBoards().get("sprout-board");
	EXPECT_EQ(sprout.nodes.size(), 8);
	const pg::FeatNodeLocation *location = registries().featBoards().findNode(sprout.rootNodeUuid);
	ASSERT_NE(location, nullptr); EXPECT_EQ(location->board, &sprout); EXPECT_EQ(location->node, &sprout.node(sprout.rootNodeUuid));
}

TEST(FeatBoard, RejectsDuplicateAndDanglingNodeUuids)
{
	nlohmann::json json = pg::JsonLoader::parseFile(std::filesystem::path(PG_RESOURCE_DIR) / "data" / "featboards" / "sprout-board.json");
	json["nodes"][1]["uuid"] = json["nodes"][0]["uuid"];
	pg::JsonReader duplicateReader(json, "board-test.json");
	EXPECT_THROW((void)pg::parseFeatBoard(duplicateReader, registries().abilities(), registries().statuses()), pg::JsonError);

	json = pg::JsonLoader::parseFile(std::filesystem::path(PG_RESOURCE_DIR) / "data" / "featboards" / "sprout-board.json");
	json["nodes"][0]["neighbours"][0] = "ffffffff-ffff-4fff-8fff-ffffffffffff";
	pg::JsonReader danglingReader(json, "board-test.json");
	EXPECT_THROW((void)pg::parseFeatBoard(danglingReader, registries().abilities(), registries().statuses()), pg::JsonError);
}

TEST(FeatProgress, RoundTripsKnownUuidsAndDropsUnknownOnLoad)
{
	const pg::FeatBoard &board = registries().featBoards().get("sprout-board");
	const pg::FeatNode &node = namedNode(board, "Deep Roots");
	pg::FeatBoardProgress original;
	pg::FeatNodeProgress &nodeProgress = original.getOrCreateProgress(node);
	nodeProgress.completionCount = 2;
	nodeProgress.requirements.front().advancement = {.progress = 42.5f, .completedRepeatCount = 0};
	nlohmann::json json = original.toJson();
	json["ffffffff-ffff-4fff-8fff-ffffffffffff"] = {{"completionCount", 9}};

	pg::FeatBoardProgress loaded;
	EXPECT_EQ(loaded.fromJson(json, board), 1);
	const pg::FeatNodeProgress *roundTrip = loaded.findProgress(node.uuid);
	ASSERT_NE(roundTrip, nullptr); EXPECT_EQ(roundTrip->completionCount, 2);
	EXPECT_FLOAT_EQ(roundTrip->requirements.front().advancement.progress, 42.5f);
	EXPECT_FALSE(loaded.toJson().contains("ffffffff-ffff-4fff-8fff-ffffffffffff"));
}

TEST(FeatRewards, ReplayIsDeterministicAndDerivesAbilitiesPassivesStatsAndForm)
{
	const pg::Registries &all = registries();
	pg::CreatureUnit creature(all.creatures().get("sprout"));
	const pg::FeatBoard &board = *creature.species->featBoard;
	EXPECT_TRUE(pg::completeNodeDirect(creature, namedNode(board, "Deep Roots").uuid));
	EXPECT_TRUE(pg::completeNodeDirect(creature, namedNode(board, "First Spark").uuid));
	EXPECT_TRUE(pg::completeNodeDirect(creature, namedNode(board, "Thorn Skin").uuid));
	EXPECT_TRUE(pg::completeNodeDirect(creature, namedNode(board, "Blaze Bloom").uuid));
	EXPECT_EQ(creature.attributes.health, creature.species->attributes.health + 2);
	EXPECT_NE(std::ranges::find(creature.abilities, &all.abilities().get("ember")), creature.abilities.end());
	EXPECT_NE(std::ranges::find(creature.permanentPassives, &all.statuses().get("regrowth")), creature.permanentPassives.end());
	EXPECT_EQ(creature.currentFormId, "blaze");
	const pg::Attributes attributes = creature.attributes; const auto abilities = creature.abilities; const auto passives = creature.permanentPassives;
	pg::applyProgress(creature);
	EXPECT_EQ(creature.attributes.health, attributes.health); EXPECT_EQ(creature.abilities, abilities); EXPECT_EQ(creature.permanentPassives, passives); EXPECT_EQ(creature.currentFormId, "blaze");
}

TEST(FeatBoard, RootCompletionUnlocksNeighboursAndFormTierLocksAfterEvolution)
{
	pg::CreatureUnit creature(registries().creatures().get("sprout"));
	const pg::FeatBoard &board = *creature.species->featBoard;
	EXPECT_TRUE(board.isReachable(namedNode(board, "Deep Roots"), creature));
	const pg::FeatNode &form = namedNode(board, "Blaze Bloom");
	EXPECT_FALSE(board.isReachable(form, creature));
	EXPECT_TRUE(pg::completeNodeDirect(creature, namedNode(board, "Steady Growth").uuid));
	EXPECT_TRUE(board.isReachable(form, creature));
	EXPECT_TRUE(pg::completeNodeDirect(creature, form.uuid));
	EXPECT_FALSE(board.isReachable(form, creature));
}
