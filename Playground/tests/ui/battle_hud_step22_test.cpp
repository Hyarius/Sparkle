#include "abilities/ability.hpp"
#include "abilities/effects/effect_types.hpp"
#include "battle/battle_context.hpp"
#include "battle/phases/battle_coordinator.hpp"
#include "board/board_builder.hpp"
#include "core/event_center.hpp"
#include "creatures/creature_form.hpp"
#include "creatures/creature_species.hpp"
#include "creatures/creature_unit.hpp"
#include "feats/feat_board.hpp"
#include "support/board_fixture.hpp"
#include "support/creature_fixture.hpp"
#include "taming/taming_profile.hpp"
#include "taming/wild_battle_unit.hpp"
#include "ui/ability_card_widget.hpp"
#include "ui/battle_result_screen.hpp"
#include "ui/creature_info_window.hpp"
#include "ui/passive_card_widget.hpp"
#include "statuses/status.hpp"

#include <gtest/gtest.h>

#include <memory>

namespace
{
	pg::Attributes attributes()
	{
		return {
			.health = 30,
			.ap = 6,
			.mp = 3,
			.attack = 2,
			.armor = 1,
			.magic = 4,
			.resistance = 1,
			.stamina = 4.0f,
			.staminaRate = 1.0f};
	}

	class TestTamingCondition final : public pg::BattleCondition
	{
	protected:
		float evaluateEventProgress(const pg::BattleEvent &, const pg::BattleUnit *) const override
		{
			return 0.0f;
		}
	};
}

TEST(PlacementPhase, SelectPlaceSwapAndConfirm)
{
	pg::test::BoardFixture fixture({"#####", "#####", "#####"});
	pg::EventCenter events;
	pg::BattleContext context(
		events, pg::BoardBuilder::fromGrid(fixture.grid(), fixture.registry(), 1));
	pg::BattleUnit &first = context.addUnit(
		pg::test::creature("first", attributes()), pg::BattleSide::Player);
	pg::BattleUnit &second = context.addUnit(
		pg::test::creature("second", attributes()), pg::BattleSide::Player);
	pg::BattleUnit &enemy = context.addUnit(
		pg::test::creature("enemy", attributes()), pg::BattleSide::Enemy);
	pg::BattleCoordinator coordinator(context, 7, true);

	coordinator.start();
	pg::PlacementPhase &placement = coordinator.placementPhase();
	ASSERT_EQ(coordinator.currentPhaseName(), "Placement");
	EXPECT_TRUE(placement.active());
	EXPECT_TRUE(enemy.boardPosition.has_value());
	EXPECT_FALSE(first.boardPosition.has_value());
	EXPECT_FALSE(placement.canConfirm());

	const auto &zone = context.board.deploymentZones().player;
	ASSERT_GE(zone.size(), 2u);
	ASSERT_TRUE(placement.select(&first));
	ASSERT_TRUE(placement.placeSelected(zone[0]));
	ASSERT_TRUE(placement.select(&second));
	ASSERT_TRUE(placement.placeSelected(zone[1]));
	EXPECT_TRUE(placement.canConfirm());

	ASSERT_TRUE(placement.select(&first));
	ASSERT_TRUE(placement.placeSelected(zone[1]));
	EXPECT_EQ(first.boardPosition, zone[1]);
	EXPECT_EQ(second.boardPosition, zone[0]);
	EXPECT_TRUE(placement.canConfirm());
	EXPECT_TRUE(placement.confirm());
	EXPECT_NE(coordinator.currentPhaseName(), "Placement");
}

TEST(PlacementPhase, AutoPlacesPlayerButStillWaitsForConfirmation)
{
	pg::test::BoardFixture fixture({"#####", "#####", "#####"});
	pg::EventCenter events;
	pg::BattleContext context(
		events, pg::BoardBuilder::fromGrid(fixture.grid(), fixture.registry(), 1));
	context.addUnit(pg::test::creature("player", attributes()), pg::BattleSide::Player);
	context.addUnit(pg::test::creature("enemy", attributes()), pg::BattleSide::Enemy);
	pg::BattleCoordinator coordinator(context, 3, true);
	coordinator.start();

	ASSERT_TRUE(coordinator.placementPhase().autoPlacePlayer());
	EXPECT_EQ(coordinator.currentPhaseName(), "Placement");
	EXPECT_TRUE(coordinator.placementPhase().canConfirm());
}

TEST(AbilityCardWidget, GeneratesMetadataAndComposedEffectRules)
{
	pg::Ability ability{
		.id = "arcane-drain",
		.displayName = "Arcane Drain",
		.apCost = 3,
		.mpCost = 2,
		.minimumRange = 1,
		.maximumRange = 4,
		.rangeShape = pg::RangeShape::Line,
		.requiresLineOfSight = false,
		.areaShape = pg::AreaShape::Cross,
		.areaValue = 1,
		.targetProfile = pg::TargetProfile::Enemy};
	auto damage = std::make_shared<pg::DamageEffect>();
	damage->baseDamage = 5;
	damage->damageKind = pg::DamageKind::Magical;
	damage->magicRatio = 1.0f;
	auto steal = std::make_shared<pg::StealResourceEffect>();
	steal->resource = pg::EffectResource::MP;
	steal->value = 2;
	ability.effects = {damage, steal};

	EXPECT_EQ(
		pg::AbilityCardWidget::rulesText(ability),
		"Deals 5 magical damage (0 attack, 1 magic). Steals up to 2 MP.");
	EXPECT_NE(pg::AbilityCardWidget::metadataText(ability).find("Range 1-4 line"), std::string::npos);
	EXPECT_NE(pg::AbilityCardWidget::metadataText(ability).find("ignores LoS"), std::string::npos);
}

TEST(PassiveCardWidget, GeneratesTriggerAndEffectRules)
{
	pg::Status passive{.id = "renewal", .displayName = "Renewal", .hookPoint = pg::StatusHookPoint::TurnStart};
	auto heal = std::make_shared<pg::HealEffect>();
	heal->baseHealing = 3;
	heal->magicRatio = 0.5f;
	passive.effects = {heal};
	EXPECT_EQ(
		pg::PassiveCardWidget::rulesText(passive),
		"At turn start: Restores 3 health (0 attack, 0.5 magic)." );
}

TEST(BattleResultScreen, AssemblesProgressedAndCompletedNodeNamesFromSnapshot)
{
	pg::FeatBoard board;
	const spk::UUID nodeId = spk::UUID::generate();
	const spk::UUID requirementId = spk::UUID::generate();
	board.rootNodeUuid = nodeId;
	board.nodes.push_back(pg::FeatNode{.uuid = nodeId, .displayName = "First Spark"});
	pg::CreatureSpecies species{
		.id = "spark",
		.displayName = "Spark",
		.attributes = attributes(),
		.featBoard = &board,
		.defaultFormId = "base",
		.forms = {{"base", pg::CreatureForm{.displayName = "Spark", .modelId = "placeholder-cube"}}}};
	pg::CreatureUnit creature(species);
	pg::FeatNodeProgress &nodeProgress = *creature.featBoardProgress.findProgress(nodeId);
	nodeProgress.completionCount = 0;
	nodeProgress.requirements = {{requirementId, nullptr, {.progress = 10.0f}}};
	const pg::CreatureFeatSnapshot before = pg::BattleResultScreen::captureFeatProgress(creature);

	nodeProgress.requirements[0].advancement.progress = 45.0f;
	pg::FeatSummaryRow progressed = pg::BattleResultScreen::assembleFeatSummary(creature, before);
	EXPECT_EQ(progressed.progressedNodes, (std::vector<std::string>{"First Spark"}));
	EXPECT_TRUE(progressed.completedNodes.empty());

	nodeProgress.completionCount = 1;
	pg::FeatSummaryRow completed = pg::BattleResultScreen::assembleFeatSummary(creature, before);
	EXPECT_EQ(completed.completedNodes, (std::vector<std::string>{"First Spark"}));
}

TEST(CreatureInfoWindow, SelectsOwnedWildAndTrainerContent)
{
	pg::test::BoardFixture fixture({"###"});
	pg::EventCenter events;
	pg::BattleContext context(
		events, pg::BoardBuilder::fromGrid(fixture.grid(), fixture.registry()));

	pg::CreatureSpecies wildSpecies{
		.id = "wild",
		.displayName = "Wild",
		.attributes = attributes(),
		.defaultFormId = "base",
		.forms = {{"base", pg::CreatureForm{.displayName = "Wild", .modelId = "placeholder-cube"}}}};
	wildSpecies.tamingProfile.conditions.push_back(std::make_unique<TestTamingCondition>());
	pg::CreatureUnit ownedCreature(wildSpecies);
	pg::CreatureUnit wildCreature(wildSpecies);
	pg::CreatureUnit trainerCreature(wildSpecies);

	pg::BattleUnit &owned = context.addUnit(ownedCreature, pg::BattleSide::Player);
	context.allowsTaming = true;
	pg::BattleUnit &wild = context.addUnit(wildCreature, pg::BattleSide::Enemy);
	context.allowsTaming = false;
	pg::BattleUnit &trainer = context.addUnit(trainerCreature, pg::BattleSide::Enemy);

	EXPECT_EQ(
		pg::CreatureInfoWindow::contentFor(owned).kind,
		pg::CreatureInfoWindow::ContentKind::Owned);
	const auto wildContent = pg::CreatureInfoWindow::contentFor(wild);
	EXPECT_EQ(wildContent.kind, pg::CreatureInfoWindow::ContentKind::WildEnemy);
	EXPECT_NE(wildContent.taming.find("???"), std::string::npos);
	auto *wildUnit = dynamic_cast<pg::WildBattleUnit *>(&wild);
	ASSERT_NE(wildUnit, nullptr);
	wildUnit->tamingProgress.advancements[0].progress = 25.0f;
	const auto revealed = pg::CreatureInfoWindow::contentFor(wild);
	EXPECT_EQ(revealed.taming.find("???"), std::string::npos);
	EXPECT_NE(revealed.taming.find("25%"), std::string::npos);
	EXPECT_EQ(
		pg::CreatureInfoWindow::contentFor(trainer).kind,
		pg::CreatureInfoWindow::ContentKind::TrainerEnemy);
	EXPECT_TRUE(pg::CreatureInfoWindow::contentFor(trainer).taming.empty());
}

TEST(BattleResultScreen, CollectsFeatAndRecruitEventsAndGatesContinuation)
{
	pg::FeatBoard board;
	const spk::UUID nodeId = spk::UUID::generate();
	const spk::UUID requirementId = spk::UUID::generate();
	board.rootNodeUuid = nodeId;
	board.nodes.push_back(pg::FeatNode{.uuid = nodeId, .displayName = "Bright Path"});
	pg::CreatureSpecies species{
		.id = "spark",
		.displayName = "Spark",
		.attributes = attributes(),
		.featBoard = &board,
		.defaultFormId = "base",
		.forms = {{"base", pg::CreatureForm{.displayName = "Spark", .modelId = "placeholder-cube"}}}};
	pg::CreatureUnit creature(species);
	pg::FeatNodeProgress &progress = *creature.featBoardProgress.findProgress(nodeId);
	progress.completionCount = 0;
	progress.requirements = {{requirementId, nullptr, {.progress = 5.0f}}};

	pg::test::BoardFixture fixture({"##"});
	pg::EventCenter events;
	pg::BattleContext context(
		events, pg::BoardBuilder::fromGrid(fixture.grid(), fixture.registry()));
	context.addUnit(creature, pg::BattleSide::Player);
	pg::BattleResultScreen screen("Result");
	screen.bind(context);

	progress.requirements[0].advancement.progress = 55.0f;
	events.featProgressUpdated.trigger(&creature, 0);
	pg::CreatureUnit recruit(species);
	events.creatureRecruited.trigger(&recruit);
	bool confirmed = false;
	screen.show(pg::BattleSide::Player, [&] { confirmed = true; });

	ASSERT_EQ(screen.featRows().size(), 1u);
	EXPECT_EQ(screen.featRows()[0].progressedNodes, (std::vector<std::string>{"Bright Path"}));
	EXPECT_EQ(screen.recruits(), (std::vector<std::string>{"Spark"}));
	EXPECT_FALSE(confirmed);
	EXPECT_TRUE(screen.isActivated());
	screen.confirm();
	EXPECT_TRUE(confirmed);
	EXPECT_FALSE(screen.isActivated());
}
