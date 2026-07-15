#include <gtest/gtest.h>

#include "battle/ai/battle_ai_planner.hpp"
#include "battle/battle_pump.hpp"
#include "battle/battle_session.hpp"
#include "core/paths.hpp"
#include "core/registries.hpp"
#include "fixtures/board_fixture.hpp"
#include "player/new_game_definition.hpp"
#include "player/player_data.hpp"

#include <memory>
#include <variant>

namespace
{
	struct BuiltBattle
	{
		std::unique_ptr<pgtest::BoardFixture> fixture;
		std::unique_ptr<pg::BattleSession> session;
	};

	[[nodiscard]] BuiltBattle makeBattle()
	{
		static pg::Registries registries = [] {
			pg::Registries value;
			value.loadAll(pg::resourceRoot() / "data");
			return value;
		}();
		const pg::PlayerData player = pg::makeNewPlayerData(registries.newGame(), registries);
		const pg::CreatureUnit &creature = *player.roster.team().front();
		auto fixture = std::make_unique<pgtest::BoardFixture>(pgtest::BoardFixture::flat(5, 5, 1, 1, 1));
		const pg::BoardCell playerCell = fixture->board().deployment().cells(pg::BattleSide::Player).front();
		pg::BattleConstructionRequest request;
		request.descriptor = {
			.battleId = pg::deriveBattleId(45123),
			.stableEncounterIdentity = "ai-test",
			.encounterDefinitionId = "debug-battle",
			.encounterDisplayName = "ai-test",
			.encounterKind = pg::EncounterKind::Debug,
			.boardSource = fixture->board().sourceDescriptor(),
			.combatSeed = 45123,
			.resolvedTeamId = "ai-test"};
		request.participants = {
			{.side = pg::BattleSide::Player, .rosterOrder = 0, .persistentCreatureId = creature.id, .speciesId = creature.speciesId, .formId = creature.derived.formId, .attributes = creature.derived.attributes, .abilityIds = creature.derived.abilityIds},
			{.side = pg::BattleSide::Enemy, .rosterOrder = 0, .encounterSpawnMemberId = "ai-enemy", .speciesId = creature.speciesId, .formId = creature.derived.formId, .attributes = creature.derived.attributes, .abilityIds = creature.derived.abilityIds, .aiBehaviourId = "training-aggressive"}};
		request.enemyPlacement = pg::ByLineOpponentPlacementPolicy{};
		pg::BattleConstructionResult constructed = pg::BattleSession::create(std::move(request), registries, std::move(fixture->board()));
		EXPECT_TRUE(constructed.succeeded());
		if (!constructed.succeeded())
		{
			return {std::move(fixture), {}};
		}
		auto session = std::move(constructed.session);
		EXPECT_TRUE(std::holds_alternative<pg::AcceptedCommand>(session->submit({pg::PlaceUnitCommand{pg::BattleUnitId{1}, playerCell}}, {pg::CommandController::Player})));
		EXPECT_TRUE(std::holds_alternative<pg::AcceptedCommand>(session->submit(pg::ConfirmDeploymentCommand{}, {pg::CommandController::Player})));
		EXPECT_TRUE(std::holds_alternative<pg::SchedulerAdvanceResult>(session->advanceUntilActivation()));
		return {std::move(fixture), std::move(session)};
	}
}

TEST(BattleAI, PlannerFallsThroughIllegalCastToCanonicalApproachWithoutRng)
{
	BuiltBattle battle = makeBattle();
	ASSERT_NE(battle.session, nullptr);
	const pg::AIBehaviourDefinition &behaviour = battle.session->registries().aiBehaviours().get("training-aggressive");
	const std::uint64_t digest = battle.session->gameplayProgressDigest();
	const pg::BattleAIPlanner planner;
	const pg::AIPlanResult result = planner.chooseNextCommand(*battle.session, pg::BattleUnitId{1}, behaviour);
	ASSERT_TRUE(std::holds_alternative<pg::AIPlannedCommand>(result));
	const pg::AIPlannedCommand &plan = std::get<pg::AIPlannedCommand>(result);
	EXPECT_EQ(plan.ruleId, "approach");
	EXPECT_TRUE(std::holds_alternative<pg::MoveCommand>(plan.command));
	ASSERT_EQ(plan.evaluatedRules.size(), 2U);
	EXPECT_EQ(plan.evaluatedRules[0].ruleId, "strike-nearest");
	EXPECT_TRUE(plan.evaluatedRules[0].conditionsMatched);
	EXPECT_FALSE(plan.evaluatedRules[0].decisionProducedLegalCommand);
	EXPECT_EQ(battle.session->gameplayProgressDigest(), digest);
}

TEST(BattleAI, PumpSeparatesSchedulerAndOneEnemyGatewayCommand)
{
	BuiltBattle battle = makeBattle();
	ASSERT_NE(battle.session, nullptr);
	ASSERT_TRUE(std::holds_alternative<pg::AcceptedCommand>(battle.session->submit(pg::EndTurnCommand{pg::BattleUnitId{1}, pg::EndTurnRequestCause::Explicit}, {pg::CommandController::Player})));

	pg::BattlePump pump;
	const pg::BattlePumpResult scheduled = pump.advanceUntilPlayerInput(*battle.session, 1);
	EXPECT_EQ(scheduled.state, pg::BattlePumpState::WaitingForAI);
	EXPECT_EQ(scheduled.logicalOperations, 1U);
	ASSERT_EQ(battle.session->snapshot().activeUnit, std::optional<pg::BattleUnitId>(pg::BattleUnitId{2}));
	const pg::BoardCell enemyBefore = *battle.session->snapshot().units[1].cell;

	const pg::BattlePumpResult driven = pump.advanceUntilPlayerInput(*battle.session, 1);
	EXPECT_EQ(driven.state, pg::BattlePumpState::WaitingForAI);
	EXPECT_EQ(driven.logicalOperations, 1U);
	EXPECT_NE(battle.session->snapshot().units[1].cell, std::optional<pg::BoardCell>(enemyBefore));
	EXPECT_TRUE(pump.driver().guardState().has_value());
}
