#include <gtest/gtest.h>

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
	pg::BattleParticipantSeed playerSeed(const pg::CreatureUnit &p_creature, std::uint32_t p_order)
	{
		return pg::BattleParticipantSeed{
			.side = pg::BattleSide::Player,
			.rosterOrder = p_order,
			.persistentCreatureId = p_creature.id,
			.speciesId = p_creature.speciesId,
			.formId = p_creature.derived.formId,
			.attributes = p_creature.derived.attributes,
			.abilityIds = p_creature.derived.abilityIds,
			.passiveStatusIds = p_creature.derived.passiveStatusIds};
	}

	pg::BattleParticipantSeed enemySeed(const pg::CreatureUnit &p_template)
	{
		return pg::BattleParticipantSeed{
			.side = pg::BattleSide::Enemy,
			.rosterOrder = 0,
			.encounterSpawnMemberId = "test-enemy",
			.speciesId = p_template.speciesId,
			.formId = p_template.derived.formId,
			.attributes = p_template.derived.attributes,
			.abilityIds = p_template.derived.abilityIds,
			.passiveStatusIds = p_template.derived.passiveStatusIds,
			.aiBehaviourId = "training-aggressive"};
	}
}

TEST(BattleSessionTest, ProjectsUnitsAndCommitsOnlyAcceptedDeploymentCommands)
{
	pg::Registries registries;
	ASSERT_NO_THROW(registries.loadAll(pg::resourceRoot() / "data"));
	const pg::PlayerData player = pg::makeNewPlayerData(registries.newGame(), registries);
	const pg::CreatureUnit &starter = *player.roster.team()[0];

	pgtest::BoardFixture fixture(pgtest::BoardFixture::flat(4, 5, 1, 2, 1));
	ASSERT_TRUE(fixture.built());
	const std::vector<pg::BoardCell> playerCells(
		fixture.board().deployment().cells(pg::BattleSide::Player).begin(),
		fixture.board().deployment().cells(pg::BattleSide::Player).end());

	pg::BattleConstructionRequest request;
	request.descriptor = pg::BattleDescriptor{
		.battleId = pg::deriveBattleId(123456U),
		.stableEncounterIdentity = "test-encounter-1",
		.encounterDefinitionId = "debug-battle",
		.encounterDisplayName = "encounter.debug",
		.encounterKind = pg::EncounterKind::Debug,
		.boardSource = fixture.board().sourceDescriptor(),
		.combatSeed = 123456U,
		.resolvedTeamId = "test-team"};
	request.participants = {playerSeed(starter, 0), playerSeed(starter, 2), enemySeed(starter)};
	request.participants[1].persistentCreatureId = pg::CreatureInstanceId::fromSerial(2);
	request.enemyPlacement = pg::ByLineOpponentPlacementPolicy{};

	pg::BattleConstructionResult construction =
		pg::BattleSession::create(std::move(request), registries, std::move(fixture.board()));
	ASSERT_TRUE(construction.succeeded()) << construction.error->diagnosticDetail;
	std::unique_ptr<pg::BattleSession> session = std::move(construction.session);

	const pg::BattleSnapshot initial = session->snapshot();
	ASSERT_EQ(initial.units.size(), 3U);
	EXPECT_EQ(initial.units[0].id.value(), 1U);
	EXPECT_EQ(initial.units[1].id.value(), 2U);
	EXPECT_EQ(initial.units[1].rosterOrder, 2U);
	EXPECT_EQ(initial.units[2].id.value(), 3U);
	EXPECT_TRUE(initial.units[0].persistentCreatureId.has_value());
	EXPECT_FALSE(initial.units[2].persistentCreatureId.has_value());
	EXPECT_TRUE(initial.units[2].encounterSpawnMemberId.has_value());
	EXPECT_TRUE(initial.units[2].placed);
	EXPECT_EQ(initial.units[0].health, starter.derived.attributes.maxHealth);
	EXPECT_EQ(initial.units[0].actionPoints, 0);
	EXPECT_EQ(initial.units[0].movementPoints, 0);

	const std::vector<pg::CommittedBattleBatch> initialBatches = session->takeCommittedBatches();
	ASSERT_EQ(initialBatches.size(), 1U);
	EXPECT_EQ(initialBatches[0].id.value, 1U);
	EXPECT_FALSE(initialBatches[0].action.has_value());
	EXPECT_EQ(initialBatches[0].events.count(), 3U);
	const std::vector<pg::BattleEvent> constructionEvents = session->copyEvents(initialBatches[0].events);
	ASSERT_EQ(constructionEvents.size(), 3U);
	EXPECT_TRUE(std::holds_alternative<pg::BattleStarted>(constructionEvents[0].payload));
	EXPECT_EQ(std::get<pg::BattleStarted>(constructionEvents[0].payload).boardSource, initial.boardSource);

	const std::uint64_t digestBeforeRejection = session->gameplayProgressDigest();
	const pg::CommandResult rejected = session->submit(
		pg::PlaceUnitCommand{initial.units[0].id, playerCells[0]}, {pg::CommandController::EnemyAi});
	EXPECT_EQ(std::get<pg::RejectedCommand>(rejected).reason, pg::CommandRejection::WrongController);
	EXPECT_EQ(session->gameplayProgressDigest(), digestBeforeRejection);
	EXPECT_TRUE(session->takeCommittedBatches().empty());

	const pg::CommandResult firstPlacement =
		session->submit(pg::PlaceUnitCommand{initial.units[0].id, playerCells[0]}, {pg::CommandController::Player});
	ASSERT_TRUE(std::holds_alternative<pg::AcceptedCommand>(firstPlacement));
	EXPECT_EQ(std::get<pg::AcceptedCommand>(firstPlacement).actionId.value, 1U);

	const pg::CommandResult secondPlacement =
		session->submit(pg::PlaceUnitCommand{initial.units[1].id, playerCells[1]}, {pg::CommandController::Player});
	ASSERT_TRUE(std::holds_alternative<pg::AcceptedCommand>(secondPlacement));
	EXPECT_EQ(std::get<pg::AcceptedCommand>(secondPlacement).actionId.value, 2U);

	const pg::CommandResult confirmation =
		session->submit(pg::ConfirmDeploymentCommand{}, {pg::CommandController::Player});
	ASSERT_TRUE(std::holds_alternative<pg::AcceptedCommand>(confirmation));
	EXPECT_EQ(std::get<pg::AcceptedCommand>(confirmation).actionId.value, 3U);
	EXPECT_EQ(session->archivedBatches().size(), 4U);
	EXPECT_EQ(session->snapshot().units[0].cell, playerCells[0]);
	EXPECT_EQ(session->snapshot().units[1].cell, playerCells[1]);
}
