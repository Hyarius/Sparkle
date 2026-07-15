#include <gtest/gtest.h>

#include "battle/battle_session.hpp"
#include "battle/presentation/battle_interaction_controller.hpp"
#include "core/paths.hpp"
#include "core/registries.hpp"
#include "fixtures/board_fixture.hpp"
#include "player/new_game_definition.hpp"
#include "player/player_data.hpp"

namespace
{
	pg::BattleParticipantSeed playerSeed(const pg::CreatureUnit &p_creature)
	{
		return {
			.side = pg::BattleSide::Player,
			.rosterOrder = 0,
			.persistentCreatureId = p_creature.id,
			.speciesId = p_creature.speciesId,
			.formId = p_creature.derived.formId,
			.attributes = p_creature.derived.attributes,
			.abilityIds = p_creature.derived.abilityIds,
			.passiveStatusIds = p_creature.derived.passiveStatusIds};
	}

	pg::BattleParticipantSeed enemySeed(const pg::CreatureUnit &p_creature)
	{
		return {
			.side = pg::BattleSide::Enemy,
			.rosterOrder = 0,
			.encounterSpawnMemberId = "test-enemy",
			.speciesId = p_creature.speciesId,
			.formId = p_creature.derived.formId,
			.attributes = p_creature.derived.attributes,
			.abilityIds = p_creature.derived.abilityIds,
			.passiveStatusIds = p_creature.derived.passiveStatusIds,
			.aiBehaviourId = "training-aggressive"};
	}
}

TEST(BattleInteractionController, ShowsDeploymentCellsBeforeSelectingACreature)
{
	pg::Registries registries;
	ASSERT_NO_THROW(registries.loadAll(pg::resourceRoot() / "data"));
	const pg::PlayerData player = pg::makeNewPlayerData(registries.newGame(), registries);
	const pg::CreatureUnit &starter = *player.roster.team()[0];

	pgtest::BoardFixture fixture(pgtest::BoardFixture::flat(5, 5, 1, 1, 1));
	ASSERT_TRUE(fixture.built());
	const std::size_t deploymentCellCount = fixture.board().deployment().cells(pg::BattleSide::Player).size();

	pg::BattleConstructionRequest request;
	request.descriptor = {
		.battleId = pg::deriveBattleId(314159U),
		.stableEncounterIdentity = "overlay-preview-test",
		.encounterDefinitionId = "debug-battle",
		.encounterDisplayName = "encounter.debug",
		.encounterKind = pg::EncounterKind::Debug,
		.boardSource = fixture.board().sourceDescriptor(),
		.combatSeed = 314159U,
		.resolvedTeamId = "test-team"};
	request.participants = {playerSeed(starter), enemySeed(starter)};
	request.enemyPlacement = pg::ByLineOpponentPlacementPolicy{};
	pg::BattleConstructionResult created = pg::BattleSession::create(std::move(request), registries, std::move(fixture.board()));
	ASSERT_TRUE(created.succeeded()) << created.error->diagnosticDetail;
	std::unique_ptr<pg::BattleSession> session = std::move(created.session);

	pg::BattleInteractionController controller(*session, [&session] {
		return session->snapshot();
	});
	const std::vector<pg::BattleMaskCell> masks = controller.overlayCandidates();
	ASSERT_EQ(masks.size(), deploymentCellCount);
	for (const pg::BattleMaskCell &mask : masks)
	{
		EXPECT_EQ(mask.kind, pg::BattleMaskKind::Deployment);
	}
}

TEST(BattleInteractionController, PlacementCollapsesTheSelectedRosterCardState)
{
	pg::Registries registries;
	ASSERT_NO_THROW(registries.loadAll(pg::resourceRoot() / "data"));
	const pg::PlayerData player = pg::makeNewPlayerData(registries.newGame(), registries);
	const pg::CreatureUnit &starter = *player.roster.team()[0];

	pgtest::BoardFixture fixture(pgtest::BoardFixture::flat(5, 5, 1, 1, 1));
	ASSERT_TRUE(fixture.built());
	pg::BattleConstructionRequest request;
	request.descriptor = {
		.battleId = pg::deriveBattleId(271828U),
		.stableEncounterIdentity = "placement-collapse-test",
		.encounterDefinitionId = "debug-battle",
		.encounterDisplayName = "encounter.debug",
		.encounterKind = pg::EncounterKind::Debug,
		.boardSource = fixture.board().sourceDescriptor(),
		.combatSeed = 271828U,
		.resolvedTeamId = "test-team"};
	request.participants = {playerSeed(starter), enemySeed(starter)};
	request.enemyPlacement = pg::ByLineOpponentPlacementPolicy{};
	pg::BattleConstructionResult created = pg::BattleSession::create(std::move(request), registries, std::move(fixture.board()));
	ASSERT_TRUE(created.succeeded()) << created.error->diagnosticDetail;
	std::unique_ptr<pg::BattleSession> session = std::move(created.session);

	pg::BattleInteractionController controller(*session, [&session] {
		return session->snapshot();
	});
	ASSERT_EQ(controller.selectPlacementCreature(starter.id).disposition, pg::InputHandlingDisposition::Handled);
	const auto *selected = std::get_if<pg::PlacementSelecting>(&controller.state());
	ASSERT_NE(selected, nullptr);
	ASSERT_EQ(selected->selected, starter.id);
	const pg::BoardCell destination = session->board().deployment().cells(pg::BattleSide::Player).front();
	ASSERT_EQ(controller.clickBoardCell(destination).disposition, pg::InputHandlingDisposition::Submitted);

	selected = std::get_if<pg::PlacementSelecting>(&controller.state());
	ASSERT_NE(selected, nullptr);
	EXPECT_FALSE(selected->selected.has_value());
	EXPECT_EQ(controller.overlayCandidates().size(), session->board().deployment().cells(pg::BattleSide::Player).size());
}
