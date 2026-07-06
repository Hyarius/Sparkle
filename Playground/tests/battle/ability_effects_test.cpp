#include "abilities/effect.hpp"
#include "abilities/effects/effect_types.hpp"
#include "battle/battle_action.hpp"
#include "battle/battle_context.hpp"
#include "battle/rules/battle_action_resolver.hpp"
#include "core/event_center.hpp"
#include "core/json.hpp"
#include "support/board_fixture.hpp"
#include "support/creature_fixture.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <vector>

namespace
{
	pg::Attributes effectAttributes(int p_health = 30)
	{
		return {.health = p_health, .ap = 10, .mp = 10, .attack = 2, .armor = 0, .magic = 2, .resistance = 0, .stamina = 2.0f, .staminaRate = 1.0f};
	}

	pg::Ability effectAbility(std::shared_ptr<const pg::Effect> p_effect)
	{
		pg::Ability result;
		result.id = "test";
		result.displayName = "Test";
		result.apCost = 1;
		result.minimumRange = 1;
		result.maximumRange = 4;
		result.targetProfile = pg::TargetProfile::Enemy;
		result.effects.push_back(std::move(p_effect));
		return result;
	}
}

TEST(EffectFactory, ParsesAndDescribesEveryCatalogType)
{
	const std::vector<nlohmann::json> samples = {
		{{"type", "damage"}, {"baseDamage", 2}, {"damageKind", "physical"}, {"attackRatio", 1.0}, {"magicRatio", 0.0}},
		{{"type", "heal"}, {"baseHealing", 2}, {"attackRatio", 0.0}, {"magicRatio", 1.0}},
		{{"type", "applyStatus"}, {"status", "burn"}, {"duration", {{"type", "turnBased"}, {"turns", 2}}}},
		{{"type", "removeStatus"}, {"status", "burn"}},
		{{"type", "consumeStatus"}, {"status", "charge"}},
		{{"type", "cleanse"}, {"tags", {"harmful"}}},
		{{"type", "revive"}, {"healthRestored", 3}},
		{{"type", "applyShield"}, {"kind", "magical"}, {"amount", 4}, {"durationTurns", -1}},
		{{"type", "resourceChange"}, {"resource", "ap"}, {"value", -1}},
		{{"type", "stealResource"}, {"resource", "health"}, {"value", 2}},
		{{"type", "displacement"}, {"orientation", "awayFromCaster"}, {"force", 2}},
		{{"type", "swapPosition"}},
		{{"type", "swapPositionWithCaster"}},
		{{"type", "teleportSelf"}},
		{{"type", "adjustTurnBarTime"}, {"delta", -0.5}},
		{{"type", "adjustTurnBarDuration"}, {"delta", 1.0}},
		{{"type", "placeObject"}, {"object", "trap"}, {"duration", {{"type", "infinite"}}}},
		{{"type", "removeObject"}, {"tags", {"trap"}}}};
	for (std::size_t index = 0; index < samples.size(); ++index)
	{
		pg::JsonReader reader(samples[index], "effect.json", "$[" + std::to_string(index) + "]");
		auto effect = pg::parseEffect(reader);
		ASSERT_NE(effect, nullptr);
		EXPECT_FALSE(effect->type().empty());
		EXPECT_FALSE(pg::describe(*effect).empty());
	}
}

TEST(EffectFactory, RejectsUnknownTypesAndInvalidFields)
{
	nlohmann::json unknown = {{"type", "notAnEffect"}};
	pg::JsonReader unknownReader(unknown, "effect.json");
	EXPECT_THROW((void)pg::parseEffect(unknownReader), pg::JsonError);
	nlohmann::json invalid = {{"type", "displacement"}, {"orientation", "awayFromCaster"}, {"force", 0}};
	pg::JsonReader invalidReader(invalid, "effect.json");
	EXPECT_THROW((void)pg::parseEffect(invalidReader), pg::JsonError);
}

TEST(BattleEffectResolver, AppliesCrossAreaAnchorFirstInStableOrder)
{
	pg::test::BoardFixture fixture({"#####"});
	pg::EventCenter events;
	pg::BattleContext context(events, pg::BoardBuilder::fromGrid(fixture.grid(), fixture.registry()));
	auto damage = std::make_shared<pg::DamageEffect>();
	damage->baseDamage = 2;
	pg::Ability ability = effectAbility(damage);
	ability.areaShape = pg::AreaShape::Cross;
	ability.areaValue = 1;
	auto &caster = context.addUnit(pg::test::creature("caster", effectAttributes(), {&ability}), pg::BattleSide::Player);
	auto &left = context.addUnit(pg::test::creature("left", effectAttributes()), pg::BattleSide::Enemy);
	auto &anchor = context.addUnit(pg::test::creature("anchor", effectAttributes()), pg::BattleSide::Enemy);
	auto &right = context.addUnit(pg::test::creature("right", effectAttributes()), pg::BattleSide::Enemy);
	ASSERT_TRUE(context.tryPlaceUnit(caster, {0, 0, 0}));
	ASSERT_TRUE(context.tryPlaceUnit(left, {1, 0, 0}));
	ASSERT_TRUE(context.tryPlaceUnit(anchor, {2, 0, 0}));
	ASSERT_TRUE(context.tryPlaceUnit(right, {3, 0, 0}));
	context.currentTurn = {.activeUnit = &caster, .turnIndex = 1};
	ASSERT_TRUE(pg::BattleActionResolver::resolve(context, pg::AbilityAction(caster, ability, {{2, 0, 0}})));
	ASSERT_EQ(context.log.events().size(), 8u);
	EXPECT_EQ(pg::battleEventContext(context.log.events()[2]).target, &anchor);
	EXPECT_EQ(pg::battleEventContext(context.log.events()[4]).target, &left);
	EXPECT_EQ(pg::battleEventContext(context.log.events()[6]).target, &right);
}

TEST(BattleEffectResolver, LifestealUsesActualHealthDamage)
{
	pg::test::BoardFixture fixture({"##"});
	pg::EventCenter events;
	pg::BattleContext context(events, pg::BoardBuilder::fromGrid(fixture.grid(), fixture.registry()));
	auto damage = std::make_shared<pg::DamageEffect>();
	damage->baseDamage = 10;
	damage->damageKind = pg::DamageKind::Physical;
	pg::Ability ability = effectAbility(damage);
	auto casterCreature = pg::test::creature("caster", effectAttributes(), {&ability});
	auto &caster = context.addUnit(casterCreature, pg::BattleSide::Player);
	auto &target = context.addUnit(pg::test::creature("target", effectAttributes(4)), pg::BattleSide::Enemy);
	caster.attributes.hp.setCurrent(20);
	caster.attributes.lifeSteal = 0.5f;
	ASSERT_TRUE(context.tryPlaceUnit(caster, {0, 0, 0}));
	ASSERT_TRUE(context.tryPlaceUnit(target, {1, 0, 0}));
	ASSERT_TRUE(pg::BattleActionResolver::resolve(context, pg::AbilityAction(caster, ability, {{1, 0, 0}})));
	EXPECT_EQ(caster.attributes.hp.current(), 22);
}

TEST(BattleEffectResolver, DisplacementStopsBeforeOccupiedCell)
{
	pg::test::BoardFixture fixture({"#####"});
	pg::EventCenter events;
	pg::BattleContext context(events, pg::BoardBuilder::fromGrid(fixture.grid(), fixture.registry()));
	auto push = std::make_shared<pg::DisplacementEffect>();
	push->force = 3;
	pg::Ability ability = effectAbility(push);
	auto &caster = context.addUnit(pg::test::creature("caster", effectAttributes(), {&ability}), pg::BattleSide::Player);
	auto &target = context.addUnit(pg::test::creature("target", effectAttributes()), pg::BattleSide::Enemy);
	auto &blocker = context.addUnit(pg::test::creature("blocker", effectAttributes()), pg::BattleSide::Enemy);
	ASSERT_TRUE(context.tryPlaceUnit(caster, {0, 0, 0}));
	ASSERT_TRUE(context.tryPlaceUnit(target, {1, 0, 0}));
	ASSERT_TRUE(context.tryPlaceUnit(blocker, {3, 0, 0}));
	ASSERT_TRUE(pg::BattleActionResolver::resolve(context, pg::AbilityAction(caster, ability, {{1, 0, 0}})));
	EXPECT_EQ(target.boardPosition, spk::Vector3Int(2, 0, 0));
}

TEST(BattleEffectResolver, TeleportSelfMovesCasterToEmptyAnchor)
{
	pg::test::BoardFixture fixture({"####"});
	pg::EventCenter events;
	pg::BattleContext context(events, pg::BoardBuilder::fromGrid(fixture.grid(), fixture.registry()));
	pg::Ability ability = effectAbility(std::make_shared<pg::TeleportSelfEffect>());
	ability.targetProfile = pg::TargetProfile::Empty;
	auto &caster = context.addUnit(pg::test::creature("caster", effectAttributes(), {&ability}), pg::BattleSide::Player);
	ASSERT_TRUE(context.tryPlaceUnit(caster, {0, 0, 0}));
	ASSERT_TRUE(pg::BattleActionResolver::resolve(context, pg::AbilityAction(caster, ability, {{3, 0, 0}})));
	EXPECT_EQ(caster.boardPosition, spk::Vector3Int(3, 0, 0));
}
