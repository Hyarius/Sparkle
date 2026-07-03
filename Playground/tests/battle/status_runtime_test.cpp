#include "abilities/effects/effect_types.hpp"
#include "battle/battle_context.hpp"
#include "battle/rules/battle_resource_rules.hpp"
#include "battle/rules/battle_status_rules.hpp"
#include "battle/rules/battle_turn_rules.hpp"
#include "core/event_center.hpp"
#include "core/json.hpp"
#include "core/registries.hpp"
#include "statuses/status.hpp"
#include "support/board_fixture.hpp"
#include "support/creature_fixture.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <memory>
#include <vector>

namespace
{
	pg::Attributes statusAttributes(int p_health = 20)
	{
		return {.health = p_health, .ap = 6, .mp = 3, .attack = 0, .armor = 0, .magic = 0, .resistance = 0, .stamina = 2.0f, .staminaRate = 1.0f};
	}

	pg::Status makeStatus(
		std::string p_id,
		pg::StatusHookPoint p_hook,
		std::vector<std::string> p_tags = {},
		std::vector<std::shared_ptr<const pg::Effect>> p_effects = {})
	{
		return {.id = std::move(p_id), .displayName = "Test", .tags = std::move(p_tags), .hookPoint = p_hook, .effects = std::move(p_effects)};
	}

	class RecordingEffect final : public pg::Effect
	{
	private:
		std::vector<int> *_output;
		int _value;

	public:
		RecordingEffect(std::vector<int> &p_output, int p_value) :
			_output(&p_output),
			_value(p_value)
		{
		}
		[[nodiscard]] std::string_view type() const noexcept override
		{
			return "recording";
		}
		void apply(pg::BattleAbilityExecutionContext &) const override
		{
			_output->push_back(_value);
		}
	};
}

TEST(StatusParser, LoadsCatalogAndResolvesAbilityStatusReferences)
{
	pg::Registries registries;
	registries.loadAll(std::filesystem::path(PG_RESOURCE_DIR) / "data");
	ASSERT_EQ(registries.statuses().size(), 4u);
	const pg::Status &burn = registries.statuses().get("burn");
	EXPECT_EQ(burn.hookPoint, pg::StatusHookPoint::TurnStart);
	EXPECT_TRUE(std::ranges::find(burn.tags, "harmful") != burn.tags.end());
	const pg::Ability &ember = registries.abilities().get("ember");
	ASSERT_EQ(ember.effects.size(), 2u);
	const auto *apply = dynamic_cast<const pg::ApplyStatusEffect *>(ember.effects[1].get());
	ASSERT_NE(apply, nullptr);
	EXPECT_EQ(apply->statusDefinition, &burn);
}

TEST(BattleStatuses, ApplicationsAreIndependentAndRemovalConsumesInListOrder)
{
	pg::test::BoardFixture fixture({"#"});
	pg::EventCenter events;
	pg::BattleContext context(events, pg::BoardBuilder::fromGrid(fixture.grid(), fixture.registry()));
	auto &unit = context.addUnit(pg::test::creature("unit", statusAttributes()), pg::BattleSide::Player);
	pg::Status poison = makeStatus("poison", pg::StatusHookPoint::TurnStart, {"harmful"});
	(void)unit.statuses.add(poison, 2, {.kind = pg::DurationKind::TurnBased, .turns = 2});
	(void)unit.statuses.add(poison, 3, {.kind = pg::DurationKind::TurnBased, .turns = 4});
	ASSERT_EQ(unit.statuses.entries().size(), 2u);
	const auto removed = unit.statuses.remove("poison", 4);
	EXPECT_EQ(removed.stacks, 4);
	ASSERT_EQ(unit.statuses.entries().size(), 1u);
	EXPECT_EQ(unit.statuses.entries().front().stacks, 1);
	EXPECT_EQ(unit.statuses.entries().front().remaining.turns, 4);
}

TEST(BattleStatusRules, HooksFireInStatusListOrder)
{
	pg::test::BoardFixture fixture({"#"});
	pg::EventCenter events;
	pg::BattleContext context(events, pg::BoardBuilder::fromGrid(fixture.grid(), fixture.registry()));
	auto &unit = context.addUnit(pg::test::creature("unit", statusAttributes()), pg::BattleSide::Player);
	std::vector<int> order;
	pg::Status first = makeStatus("first", pg::StatusHookPoint::TurnStart, {}, {std::make_shared<RecordingEffect>(order, 1)});
	pg::Status second = makeStatus("second", pg::StatusHookPoint::TurnStart, {}, {std::make_shared<RecordingEffect>(order, 2)});
	(void)unit.statuses.add(first, 1, {});
	(void)unit.statuses.add(second, 1, {});
	pg::BattleStatusRules::applyHook(unit, pg::StatusHookPoint::TurnStart);
	EXPECT_EQ(order, (std::vector<int>{1, 2}));
}

TEST(BattleStatusRules, ResourceHooksAreNonReentrant)
{
	pg::test::BoardFixture fixture({"#"});
	pg::EventCenter events;
	pg::BattleContext context(events, pg::BoardBuilder::fromGrid(fixture.grid(), fixture.registry()));
	auto &unit = context.addUnit(pg::test::creature("unit", statusAttributes(10)), pg::BattleSide::Player);
	ASSERT_TRUE(context.tryPlaceUnit(unit, {0, 0, 0}));
	auto damage = std::make_shared<pg::DamageEffect>();
	damage->baseDamage = 1;
	pg::Status thorns = makeStatus("self-damage", pg::StatusHookPoint::OnHPLoss, {}, {damage});
	(void)unit.statuses.add(thorns, 1, {});
	(void)pg::BattleResourceRules::change(unit, pg::BattleResourceKind::Health, -1);
	EXPECT_EQ(unit.attributes.hp.current(), 8);
	EXPECT_EQ(std::ranges::count_if(context.log.events(), [](const pg::BattleEvent &event) {
				  return event.type == pg::BattleEventType::Damage;
			  }),
			  1);
}

TEST(BattleStatuses, CleanseSkipsSourcePassives)
{
	pg::test::BoardFixture fixture({"#"});
	pg::EventCenter events;
	pg::BattleContext context(events, pg::BoardBuilder::fromGrid(fixture.grid(), fixture.registry()));
	auto &unit = context.addUnit(pg::test::creature("unit", statusAttributes()), pg::BattleSide::Player);
	pg::Status harmful = makeStatus("harmful", pg::StatusHookPoint::TurnStart, {"harmful"});
	pg::Status passive = makeStatus("passive", pg::StatusHookPoint::TurnStart, {"harmful"});
	(void)unit.statuses.add(harmful, 1, {.kind = pg::DurationKind::TurnBased, .turns = 2});
	(void)unit.statuses.add(passive, 1, {}, true);
	const auto removed = unit.statuses.removeByTags({"harmful"});
	ASSERT_EQ(removed.size(), 1u);
	EXPECT_EQ(removed.front().definition, &harmful);
	EXPECT_TRUE(unit.statuses.contains("passive"));
	EXPECT_FALSE(unit.statuses.contains("passive", false));
}

TEST(BattleTurnRules, RealStunPausesThenResumesAndExpirationIsReported)
{
	pg::test::BoardFixture fixture({"##"});
	pg::EventCenter events;
	pg::BattleContext context(events, pg::BoardBuilder::fromGrid(fixture.grid(), fixture.registry()));
	auto &stunned = context.addUnit(pg::test::creature("stunned", statusAttributes()), pg::BattleSide::Player);
	auto &other = context.addUnit(pg::test::creature("other", statusAttributes()), pg::BattleSide::Enemy);
	pg::Status stun = makeStatus("stun", pg::StatusHookPoint::TurnStart, {"stun"});
	context.currentTurn.turnIndex = 1;
	(void)stunned.statuses.add(stun, 1, {.kind = pg::DurationKind::TurnBased, .turns = 1});
	pg::BattleTurnRules::advanceTurnBars(context, 1.0f);
	EXPECT_FLOAT_EQ(stunned.attributes.turnBar.current(), 0.0f);
	EXPECT_FLOAT_EQ(other.attributes.turnBar.current(), 1.0f);
	context.currentTurn = {.activeUnit = &other, .turnIndex = 2};
	pg::BattleTurnRules::endTurn(context);
	EXPECT_FALSE(stunned.hasStatusTag("stun"));
	pg::BattleTurnRules::advanceTurnBars(context, 1.0f);
	EXPECT_FLOAT_EQ(stunned.attributes.turnBar.current(), 1.0f);
	EXPECT_TRUE(std::ranges::any_of(context.log.events(), [](const pg::BattleEvent &event) {
		return event.type == pg::BattleEventType::StatusRemoved;
	}));
}

TEST(BattleAttributes, ShieldsMatchKindAbsorbPartiallyBreakAndExpire)
{
	pg::BattleAttributes attributes(statusAttributes(20));
	attributes.addShield(pg::ShieldKind::Magical, 4, -1);
	attributes.addShield(pg::ShieldKind::Physical, 3, 1);
	const auto physical = attributes.absorbDamage(pg::DamageKind::Physical, 5);
	EXPECT_EQ(physical.absorbed, 3);
	EXPECT_EQ(physical.remaining, 2);
	ASSERT_EQ(physical.broken.size(), 1u);
	ASSERT_EQ(attributes.shields.size(), 1u);
	EXPECT_EQ(attributes.shields[0].kind, pg::ShieldKind::Magical);
	attributes.advanceShieldDurations();
	EXPECT_EQ(attributes.shields.size(), 1u);
}

TEST(BattleTurnRules, TurnDurationExpiryEmitsStatusRemoved)
{
	pg::test::BoardFixture fixture({"#"});
	pg::EventCenter events;
	pg::BattleContext context(events, pg::BoardBuilder::fromGrid(fixture.grid(), fixture.registry()));
	auto &unit = context.addUnit(pg::test::creature("unit", statusAttributes()), pg::BattleSide::Player);
	pg::Status status = makeStatus("temporary", pg::StatusHookPoint::TurnEnd);
	(void)unit.statuses.add(status, 2, {.kind = pg::DurationKind::TurnBased, .turns = 1});
	context.currentTurn = {.activeUnit = &unit, .turnIndex = 1};
	pg::BattleTurnRules::endTurn(context);
	ASSERT_FALSE(unit.statuses.contains("temporary"));
	const auto removed = std::ranges::find_if(context.log.events(), [](const pg::BattleEvent &event) {
		return event.type == pg::BattleEventType::StatusRemoved;
	});
	ASSERT_NE(removed, context.log.events().end());
	EXPECT_EQ(removed->status, &status);
	EXPECT_EQ(removed->amount, 2);
}
