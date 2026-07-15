#include <gtest/gtest.h>

#include "battle/battle_session.hpp"
#include "battle/effects/effect_resolver.hpp"
#include "core/paths.hpp"
#include "core/registries.hpp"
#include "fixtures/board_fixture.hpp"
#include "player/new_game_definition.hpp"
#include "player/player_data.hpp"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
	class RuntimeData
	{
	private:
		std::filesystem::path _path;

	public:
		explicit RuntimeData(std::string_view p_name) :
			_path(std::filesystem::temp_directory_path() / "sparkle-playground-effect-runtime" / p_name)
		{
			std::filesystem::remove_all(_path);
			std::filesystem::create_directories(_path);
			std::filesystem::copy(pg::resourceRoot() / "data", _path, std::filesystem::copy_options::recursive);
		}
		~RuntimeData()
		{
			std::error_code error;
			std::filesystem::remove_all(_path, error);
		}
		[[nodiscard]] const std::filesystem::path &path() const noexcept
		{
			return _path;
		}
		void ability(std::string_view p_id, std::string_view p_json) const
		{
			std::ofstream stream(_path / "abilities" / (std::string(p_id) + ".json"), std::ios::binary);
			stream << p_json;
		}
	};

	[[nodiscard]] std::string ability(
		std::string_view p_effects,
		int p_minimum = 1,
		int p_maximum = 8,
		int p_actionPointCost = 1,
		int p_movementPointCost = 0)
	{
		return std::string(R"({"version":1,"displayNameKey":"ability.runtime.name","descriptionKey":"ability.runtime.description","icon":[0,0],"cost":{"actionPoints":)") +
			   std::to_string(p_actionPointCost) + R"(,"movementPoints":)" + std::to_string(p_movementPointCost) + R"(},"range":{"shape":"diamond","minimum":)" +
			   std::to_string(p_minimum) + R"(,"maximum":)" + std::to_string(p_maximum) + R"(,"requiresLineOfSight":false},"area":{"shape":"single","radius":0},"anchorFilter":{"allowSelf":true,"allowAllies":true,"allowEnemies":true,"allowDefeated":false,"allowEmptyCell":true},"affectedFilter":{"allowSelf":true,"allowAllies":true,"allowEnemies":true,"allowDefeated":false,"allowEmptyCell":true},"effects":)" + std::string(p_effects) + "}";
	}

	[[nodiscard]] pg::BattleParticipantSeed playerSeed(const pg::CreatureUnit &p_creature, std::string p_ability)
	{
		return {.side = pg::BattleSide::Player, .rosterOrder = 0, .persistentCreatureId = p_creature.id, .speciesId = p_creature.speciesId, .formId = p_creature.derived.formId, .attributes = p_creature.derived.attributes, .abilityIds = {std::move(p_ability)}};
	}

	[[nodiscard]] pg::BattleParticipantSeed enemySeed(const pg::CreatureUnit &p_creature)
	{
		return {.side = pg::BattleSide::Enemy, .rosterOrder = 0, .encounterSpawnMemberId = "runtime-enemy", .speciesId = p_creature.speciesId, .formId = p_creature.derived.formId, .attributes = p_creature.derived.attributes, .aiBehaviourId = "training-aggressive"};
	}

	[[nodiscard]] std::unique_ptr<pg::BattleSession> makeSession(
		pg::Registries &p_registries,
		std::string p_ability,
		pgtest::BoardFixture::Request p_boardRequest = pgtest::BoardFixture::flat(5, 5, 1, 1, 1),
		pg::OpponentPlacementPolicy p_enemyPlacement = pg::ByLineOpponentPlacementPolicy{})
	{
		const pg::PlayerData player = pg::makeNewPlayerData(p_registries.newGame(), p_registries);
		const pg::CreatureUnit &creature = *player.roster.team()[0];
		auto board = std::make_unique<pgtest::BoardFixture>(std::move(p_boardRequest));
		const pg::BoardCell deployment = board->board().deployment().cells(pg::BattleSide::Player).front();
		pg::BattleConstructionRequest request;
		request.descriptor = {.battleId = pg::deriveBattleId(9321), .stableEncounterIdentity = "runtime-test", .encounterDefinitionId = "debug-battle", .encounterDisplayName = "runtime-test", .encounterKind = pg::EncounterKind::Debug, .boardSource = board->board().sourceDescriptor(), .combatSeed = 9321, .resolvedTeamId = "runtime"};
		request.participants = {playerSeed(creature, std::move(p_ability)), enemySeed(creature)};
		request.enemyPlacement = std::move(p_enemyPlacement);
		pg::BattleConstructionResult result = pg::BattleSession::create(std::move(request), p_registries, std::move(board->board()));
		EXPECT_TRUE(result.succeeded());
		if (!result.succeeded())
		{
			return {};
		}
		// BoardData retains fixture-owned voxel definitions. Keep this test fixture alive for the
		// session lifetime, exactly as production owns its frozen board source.
		static std::vector<std::unique_ptr<pgtest::BoardFixture>> retainedBoards;
		retainedBoards.push_back(std::move(board));
		std::unique_ptr<pg::BattleSession> session = std::move(result.session);
		EXPECT_TRUE(std::holds_alternative<pg::AcceptedCommand>(session->submit(pg::PlaceUnitCommand{pg::BattleUnitId{1}, deployment}, {pg::CommandController::Player})));
		EXPECT_TRUE(std::holds_alternative<pg::AcceptedCommand>(session->submit(pg::ConfirmDeploymentCommand{}, {pg::CommandController::Player})));
		EXPECT_TRUE(std::holds_alternative<pg::SchedulerAdvanceResult>(session->advanceUntilActivation()));
		return session;
	}

	[[nodiscard]] std::vector<std::string> eventNames(const std::vector<pg::BattleEvent> &p_events)
	{
		std::vector<std::string> names;
		names.reserve(p_events.size());
		for (const pg::BattleEvent &event : p_events)
		{
			names.emplace_back(pg::battleEventName(event.payload));
		}
		return names;
	}

	template <typename T>
	[[nodiscard]] const T &requirePayload(const std::vector<pg::BattleEvent> &p_events)
	{
		const auto found = std::find_if(p_events.begin(), p_events.end(), [](const pg::BattleEvent &event) {
			return std::holds_alternative<T>(event.payload);
		});
		if (found == p_events.end())
		{
			throw std::runtime_error("expected battle event payload was not staged");
		}
		return std::get<T>(found->payload);
	}
}

TEST(EffectResolverRuntime, SupportsTheCompleteStepTenCatalog)
{
	EXPECT_TRUE(pg::EffectResolver::supports(pg::DamageEffectSpec{}));
	EXPECT_TRUE(pg::EffectResolver::supports(pg::ApplyShieldEffectSpec{}));
	EXPECT_TRUE(pg::EffectResolver::supports(pg::TeleportSourceEffectSpec{}));
	EXPECT_TRUE(pg::EffectResolver::supports(pg::ApplyStatusEffectSpec{}));
	EXPECT_TRUE(pg::EffectResolver::supports(pg::RemoveStatusEffectSpec{}));
	EXPECT_TRUE(pg::EffectResolver::supports(pg::CleanseEffectSpec{}));
	EXPECT_TRUE(pg::EffectResolver::supports(pg::PlaceObjectEffectSpec{}));
	EXPECT_TRUE(pg::EffectResolver::supports(pg::RemoveObjectsEffectSpec{}));
}

TEST(EffectResolverRuntime, ApplyStatusCommitsTheRuntimeInstance)
{
	RuntimeData data("unsupported-atomic");
	data.ability("runtime-unsupported", ability(R"([{"id":"poison","type":"applyStatus","applyTo":"affectedUnits","requiresLivingSource":true,"status":"training-guarded","stacks":1,"duration":{"type":"ownerActivations","count":1}}])"));
	pg::Registries registries;
	ASSERT_NO_THROW(registries.loadAll(data.path()));
	auto session = makeSession(registries, "runtime-unsupported");
	ASSERT_NE(session, nullptr);
	const pg::BoardCell enemy = *session->snapshot().units[1].cell;
	const pg::CommandResult result = session->submit(pg::CastAbilityCommand{pg::BattleUnitId{1}, "runtime-unsupported", enemy}, {pg::CommandController::Player});
	ASSERT_TRUE(std::holds_alternative<pg::AcceptedCommand>(result));
	const pg::BattleSnapshot after = session->snapshot();
	ASSERT_EQ(after.units[1].transientStatuses.size(), 1U);
	EXPECT_EQ(after.units[1].transientStatuses[0].definitionId, "training-guarded");
}

TEST(EffectResolverRuntime, EveryClosedPayloadCanEnterTheRuntime)
{
	struct DeferredCase
	{
		std::string name;
		std::string effects;
	};
	const std::vector<DeferredCase> cases{
		{"apply-status", R"([{"id":"deferred","type":"applyStatus","applyTo":"sourceUnit","requiresLivingSource":true,"status":"training-guarded","stacks":1,"duration":{"type":"infinite"}}])"},
		{"remove-status", R"([{"id":"deferred","type":"removeStatus","applyTo":"sourceUnit","requiresLivingSource":true,"status":"training-guarded","stacks":1}])"},
		{"cleanse", R"([{"id":"deferred","type":"cleanse","applyTo":"sourceUnit","requiresLivingSource":true,"tags":["guard"]}])"},
		{"place-object", R"([{"id":"deferred","type":"placeObject","applyTo":"anchorCell","requiresLivingSource":true,"object":"training-snare-object","duration":{"type":"infinite"}}])"},
		{"remove-objects", R"([{"id":"deferred","type":"removeObjects","applyTo":"anchorCell","requiresLivingSource":true,"tags":["snare"]}])"},
		{"damage-then-status", R"([{"id":"hit","type":"damage","applyTo":"primaryUnit","requiresLivingSource":true,"kind":"physical","base":100,"strengthRatioPermille":0,"magicPowerRatioPermille":0},{"id":"deferred","type":"applyStatus","applyTo":"primaryUnit","requiresLivingSource":true,"status":"training-guarded","stacks":1,"duration":{"type":"infinite"}}])"},
		{"shield-then-object", R"([{"id":"ward","type":"applyShield","applyTo":"sourceUnit","requiresLivingSource":true,"kind":"physical","amount":1,"duration":{"type":"infinite"}},{"id":"deferred","type":"placeObject","applyTo":"anchorCell","requiresLivingSource":true,"object":"training-snare-object","duration":{"type":"infinite"}}])"},
		{"heal-then-cleanse", R"([{"id":"heal","type":"heal","applyTo":"sourceUnit","requiresLivingSource":true,"base":1,"strengthRatioPermille":0,"magicPowerRatioPermille":0},{"id":"deferred","type":"cleanse","applyTo":"sourceUnit","requiresLivingSource":true,"tags":["guard"]}])"}};

	for (const DeferredCase &testCase : cases)
	{
		RuntimeData data("unsupported-catalog-" + testCase.name);
		data.ability("runtime-deferred", ability(testCase.effects));
		pg::Registries registries;
		ASSERT_NO_THROW(registries.loadAll(data.path())) << testCase.name;
		auto session = makeSession(registries, "runtime-deferred");
		ASSERT_NE(session, nullptr) << testCase.name;
		const pg::BattleSnapshot before = session->snapshot();
		const pg::BoardCell anchor = *before.units[1].cell;
		EXPECT_FALSE(session->abilityAnchors(pg::BattleUnitId{1}, "runtime-deferred").empty()) << testCase.name;

		const pg::CommandResult result = session->submit(
			pg::CastAbilityCommand{pg::BattleUnitId{1}, "runtime-deferred", anchor}, {pg::CommandController::Player});
		ASSERT_TRUE(std::holds_alternative<pg::AcceptedCommand>(result)) << testCase.name;
		EXPECT_GT(session->archivedBatches().size(), 0U) << testCase.name;
	}
}

TEST(EffectResolverRuntime, AppliesShieldAndCapturesTimelineDuration)
{
	RuntimeData data("shield-duration");
	data.ability("runtime-shield", ability(R"([{"id":"ward","type":"applyShield","applyTo":"sourceUnit","requiresLivingSource":true,"kind":"physical","amount":7,"duration":{"type":"timeline","seconds":2}}])"));
	pg::Registries registries;
	ASSERT_NO_THROW(registries.loadAll(data.path()));
	auto session = makeSession(registries, "runtime-shield");
	ASSERT_NE(session, nullptr);
	const pg::BoardCell enemy = *session->snapshot().units[1].cell;
	ASSERT_TRUE(std::holds_alternative<pg::AcceptedCommand>(session->submit(pg::CastAbilityCommand{pg::BattleUnitId{1}, "runtime-shield", enemy}, {pg::CommandController::Player})));
	const pg::BattleSnapshot snapshot = session->snapshot();
	const auto &shields = snapshot.units[0].shields;
	ASSERT_EQ(shields.size(), 1U);
	EXPECT_EQ(shields[0].id.value(), 1U);
	EXPECT_EQ(shields[0].remainingAmount, 7);
	ASSERT_TRUE(std::holds_alternative<pg::TimelineDurationSnapshot>(shields[0].duration));
	EXPECT_EQ(
		std::get<pg::TimelineDurationSnapshot>(shields[0].duration).expiresAt.ticks(),
		snapshot.elapsed.ticks() + 2000);
}

TEST(EffectResolverRuntime, KeepsAbilityCostAndCurrentResourceEffectsDistinct)
{
	RuntimeData data("resource-effect");
	data.ability("runtime-resource", ability(R"([{"id":"restore","type":"changeResource","applyTo":"sourceUnit","requiresLivingSource":true,"resource":"actionPoints","delta":100}])"));
	pg::Registries registries;
	ASSERT_NO_THROW(registries.loadAll(data.path()));
	auto session = makeSession(registries, "runtime-resource");
	ASSERT_NE(session, nullptr);
	const pg::BoardCell enemy = *session->snapshot().units[1].cell;
	const pg::CommandResult result = session->submit(
		pg::CastAbilityCommand{pg::BattleUnitId{1}, "runtime-resource", enemy}, {pg::CommandController::Player});
	ASSERT_TRUE(std::holds_alternative<pg::AcceptedCommand>(result));
	const std::vector<pg::BattleEvent> events = session->copyEvents(std::get<pg::AcceptedCommand>(result).events);
	ASSERT_GE(events.size(), 3U);
	EXPECT_TRUE(std::holds_alternative<pg::ResourceSpent>(events[0].payload));
	EXPECT_TRUE(std::holds_alternative<pg::AbilityCast>(events[1].payload));
	const auto changed = std::find_if(events.begin(), events.end(), [](const pg::BattleEvent &event) {
		return std::holds_alternative<pg::ResourceChanged>(event.payload);
	});
	ASSERT_NE(changed, events.end());
	const pg::ResourceChanged &resource = std::get<pg::ResourceChanged>(changed->payload);
	EXPECT_EQ(resource.reason, pg::ResourceChangeReason::Effect);
	EXPECT_EQ(resource.after, session->snapshot().units[0].maxActionPoints);
	EXPECT_LT(resource.appliedDelta, resource.requestedDelta);
}

TEST(EffectResolverRuntime, UsesCheckedOffenseAndMitigationFormulaTables)
{
	struct FormulaCase
	{
		std::string name;
		std::string effect;
		int expected = 0;
	};
	const std::vector<FormulaCase> cases{
		{"base", R"({"id":"hit","type":"damage","applyTo":"primaryUnit","requiresLivingSource":true,"kind":"physical","base":10,"strengthRatioPermille":0,"magicPowerRatioPermille":0})", 9},
		{"strength", R"({"id":"hit","type":"damage","applyTo":"primaryUnit","requiresLivingSource":true,"kind":"physical","base":0,"strengthRatioPermille":1000,"magicPowerRatioPermille":0})", 5},
		{"hybrid", R"({"id":"hit","type":"damage","applyTo":"primaryUnit","requiresLivingSource":true,"kind":"physical","base":1,"strengthRatioPermille":500,"magicPowerRatioPermille":500})", 4},
		{"magical", R"({"id":"hit","type":"damage","applyTo":"primaryUnit","requiresLivingSource":true,"kind":"magical","base":0,"strengthRatioPermille":0,"magicPowerRatioPermille":1000})", 2}};

	for (const FormulaCase &testCase : cases)
	{
		RuntimeData data("formula-" + testCase.name);
		data.ability("runtime-formula", ability("[" + testCase.effect + "]"));
		pg::Registries registries;
		ASSERT_NO_THROW(registries.loadAll(data.path()));
		auto session = makeSession(registries, "runtime-formula");
		ASSERT_NE(session, nullptr);
		const pg::BoardCell enemy = *session->snapshot().units[1].cell;
		const pg::CommandResult result = session->submit(pg::CastAbilityCommand{pg::BattleUnitId{1}, "runtime-formula", enemy}, {pg::CommandController::Player});
		ASSERT_TRUE(std::holds_alternative<pg::AcceptedCommand>(result));
		const pg::Damage &damage = requirePayload<pg::Damage>(session->copyEvents(std::get<pg::AcceptedCommand>(result).events));
		EXPECT_EQ(damage.computedDamage, testCase.expected) << testCase.name;
		EXPECT_EQ(damage.appliedToShield, 0) << testCase.name;
		EXPECT_EQ(damage.appliedToHealth, testCase.expected) << testCase.name;
	}
}

TEST(EffectResolverRuntime, OrdersShieldChannelsBreaksAndSpillBeforeDamage)
{
	RuntimeData data("shield-order");
	data.ability("runtime-shield-order", ability(R"([
{"id":"physical-one","type":"applyShield","applyTo":"sourceUnit","requiresLivingSource":true,"kind":"physical","amount":3,"duration":{"type":"infinite"}},
{"id":"physical-two","type":"applyShield","applyTo":"sourceUnit","requiresLivingSource":true,"kind":"physical","amount":5,"duration":{"type":"infinite"}},
{"id":"magical","type":"applyShield","applyTo":"sourceUnit","requiresLivingSource":true,"kind":"magical","amount":7,"duration":{"type":"infinite"}},
{"id":"spill","type":"damage","applyTo":"sourceUnit","requiresLivingSource":true,"kind":"physical","base":15,"strengthRatioPermille":0,"magicPowerRatioPermille":0}
])"));
	pg::Registries registries;
	ASSERT_NO_THROW(registries.loadAll(data.path()));
	auto session = makeSession(registries, "runtime-shield-order");
	ASSERT_NE(session, nullptr);
	const pg::BoardCell enemy = *session->snapshot().units[1].cell;
	const pg::CommandResult result = session->submit(pg::CastAbilityCommand{pg::BattleUnitId{1}, "runtime-shield-order", enemy}, {pg::CommandController::Player});
	ASSERT_TRUE(std::holds_alternative<pg::AcceptedCommand>(result));
	const std::vector<pg::BattleEvent> events = session->copyEvents(std::get<pg::AcceptedCommand>(result).events);
	EXPECT_EQ(eventNames(events), (std::vector<std::string>{"resourceSpent", "abilityCast", "shieldApplied", "shieldApplied", "shieldApplied", "shieldAbsorbed", "shieldBroken", "shieldAbsorbed", "shieldBroken", "damage"}));
	const pg::Damage &damage = requirePayload<pg::Damage>(events);
	EXPECT_EQ(damage.computedDamage, 14);
	EXPECT_EQ(damage.appliedToShield, 8);
	EXPECT_EQ(damage.appliedToHealth, 6);
	EXPECT_TRUE(damage.targetHadAnyShield);
	EXPECT_TRUE(damage.targetHadMatchingShield);
	const pg::BattleSnapshot shieldSnapshot = session->snapshot();
	const auto &shields = shieldSnapshot.units[0].shields;
	ASSERT_EQ(shields.size(), 1U);
	EXPECT_EQ(shields[0].kind, pg::DamageKind::Magical);
	EXPECT_EQ(shields[0].remainingAmount, 7);
}

TEST(EffectResolverRuntime, RecordsResourcePenaltyAndTurnBarAppliedValues)
{
	RuntimeData data("resource-penalty-bar");
	data.ability("runtime-resources", ability(R"([
{"id":"drain-ap","type":"changeResource","applyTo":"sourceUnit","requiresLivingSource":true,"resource":"actionPoints","delta":-100},
{"id":"fill-mp","type":"changeResource","applyTo":"sourceUnit","requiresLivingSource":true,"resource":"movementPoints","delta":100},
{"id":"slow-next","type":"applyNextActivationPenalty","applyTo":"sourceUnit","requiresLivingSource":true,"resource":"movementPoints","amount":2},
{"id":"bar","type":"adjustTurnBar","applyTo":"sourceUnit","requiresLivingSource":true,"deltaSeconds":-1.0}
])"));
	pg::Registries registries;
	ASSERT_NO_THROW(registries.loadAll(data.path()));
	auto session = makeSession(registries, "runtime-resources");
	ASSERT_NE(session, nullptr);
	const pg::BoardCell enemy = *session->snapshot().units[1].cell;
	const pg::CommandResult result = session->submit(pg::CastAbilityCommand{pg::BattleUnitId{1}, "runtime-resources", enemy}, {pg::CommandController::Player});
	ASSERT_TRUE(std::holds_alternative<pg::AcceptedCommand>(result));
	const std::vector<pg::BattleEvent> events = session->copyEvents(std::get<pg::AcceptedCommand>(result).events);
	std::vector<pg::ResourceChanged> resources;
	for (const pg::BattleEvent &event : events)
	{
		if (const auto *changed = std::get_if<pg::ResourceChanged>(&event.payload))
		{
			resources.push_back(*changed);
		}
	}
	ASSERT_EQ(resources.size(), 2U);
	EXPECT_EQ(resources[0].requestedDelta, -100);
	EXPECT_EQ(resources[0].appliedDelta, -5); // AP was six, then one point paid as the cast cost.
	EXPECT_EQ(resources[1].requestedDelta, 100);
	EXPECT_EQ(resources[1].appliedDelta, 0);
	const pg::NextActivationPenaltyApplied &penalty = requirePayload<pg::NextActivationPenaltyApplied>(events);
	EXPECT_EQ(penalty.requested, 2);
	EXPECT_EQ(penalty.applied, 2);
	EXPECT_EQ(penalty.accumulatedBefore, 0);
	EXPECT_EQ(penalty.accumulatedAfter, 2);
	const pg::TurnBarAdjusted &bar = requirePayload<pg::TurnBarAdjusted>(events);
	EXPECT_EQ(bar.requestedDelta.ticks(), -1000);
	EXPECT_EQ(bar.appliedDelta.ticks(), -1000);
	const pg::BattleSnapshot snapshot = session->snapshot();
	EXPECT_EQ(snapshot.units[0].actionPoints, 0);
	EXPECT_EQ(snapshot.units[0].movementPoints, snapshot.units[0].maxMovementPoints);
	EXPECT_EQ(snapshot.units[0].turnBarFill.ticks(), 3000);
}

TEST(EffectResolverRuntime, HealsWithTheSameOffenseFormulaAndNeverOverheals)
{
	RuntimeData data("healing-formula");
	data.ability("runtime-heal", ability(R"([
{"id":"wound","type":"damage","applyTo":"sourceUnit","requiresLivingSource":true,"kind":"physical","base":15,"strengthRatioPermille":0,"magicPowerRatioPermille":0},
{"id":"recover","type":"heal","applyTo":"sourceUnit","requiresLivingSource":true,"base":100,"strengthRatioPermille":0,"magicPowerRatioPermille":0}
])"));
	pg::Registries registries;
	ASSERT_NO_THROW(registries.loadAll(data.path()));
	auto session = makeSession(registries, "runtime-heal");
	ASSERT_NE(session, nullptr);
	const pg::BoardCell enemy = *session->snapshot().units[1].cell;
	const pg::CommandResult result = session->submit(pg::CastAbilityCommand{pg::BattleUnitId{1}, "runtime-heal", enemy}, {pg::CommandController::Player});
	ASSERT_TRUE(std::holds_alternative<pg::AcceptedCommand>(result));
	const pg::Healing &healing = requirePayload<pg::Healing>(session->copyEvents(std::get<pg::AcceptedCommand>(result).events));
	EXPECT_EQ(healing.requestedHealing, 100);
	EXPECT_EQ(healing.healthBefore, 10);
	EXPECT_EQ(healing.appliedHealing, 14);
	EXPECT_EQ(healing.healthAfter, 24);
}

TEST(EffectResolverRuntime, AutomaticallyEndsActivationForNoLegalCommandsAndCommandCap)
{
	RuntimeData data("automatic-cast-end");
	data.ability("runtime-drain", ability(R"([
{"id":"empty-ap","type":"changeResource","applyTo":"sourceUnit","requiresLivingSource":true,"resource":"actionPoints","delta":-100},
{"id":"empty-mp","type":"changeResource","applyTo":"sourceUnit","requiresLivingSource":true,"resource":"movementPoints","delta":-100}
])"));
	data.ability("runtime-zero", ability(R"([{"id":"bar","type":"adjustTurnBar","applyTo":"sourceUnit","requiresLivingSource":true,"deltaSeconds":-1.0}])", 1, 8, 0, 0));
	pg::Registries registries;
	ASSERT_NO_THROW(registries.loadAll(data.path()));
	auto noLegal = makeSession(registries, "runtime-drain");
	ASSERT_NE(noLegal, nullptr);
	const pg::BoardCell noLegalAnchor = *noLegal->snapshot().units[1].cell;
	const pg::CommandResult drained = noLegal->submit(pg::CastAbilityCommand{pg::BattleUnitId{1}, "runtime-drain", noLegalAnchor}, {pg::CommandController::Player});
	ASSERT_TRUE(std::holds_alternative<pg::AcceptedCommand>(drained));
	const pg::ActivationEnded &noLegalEnd = requirePayload<pg::ActivationEnded>(noLegal->copyEvents(std::get<pg::AcceptedCommand>(drained).events));
	EXPECT_EQ(noLegalEnd.reason, pg::ActivationEndReason::NoLegalCommands);
	EXPECT_EQ(noLegal->phase(), pg::BattlePhase::AwaitingActivation);

	auto capped = makeSession(registries, "runtime-zero");
	ASSERT_NE(capped, nullptr);
	const pg::BoardCell cappedAnchor = *capped->snapshot().units[1].cell;
	pg::CommandResult last = pg::RejectedCommand{pg::CommandRejection::CommandUnavailable};
	for (std::size_t command = 0; command < 64; ++command)
	{
		last = capped->submit(pg::CastAbilityCommand{pg::BattleUnitId{1}, "runtime-zero", cappedAnchor}, {pg::CommandController::Player});
		ASSERT_TRUE(std::holds_alternative<pg::AcceptedCommand>(last));
	}
	const pg::ActivationEnded &capEnd = requirePayload<pg::ActivationEnded>(capped->copyEvents(std::get<pg::AcceptedCommand>(last).events));
	EXPECT_EQ(capEnd.reason, pg::ActivationEndReason::CommandCap);
	EXPECT_EQ(capped->phase(), pg::BattlePhase::AwaitingActivation);
}

TEST(EffectResolverRuntime, UsesCapturedTargetsAndCanonicalSpatialOrdering)
{
	RuntimeData data("spatial-captured");
	data.ability("runtime-spatial", ability(R"([
{"id":"pull","type":"displace","applyTo":"primaryUnit","requiresLivingSource":true,"direction":"towardSource","distance":1},
{"id":"swap","type":"swapWithSource","applyTo":"primaryUnit","requiresLivingSource":true},
{"id":"teleport","type":"teleportSource","applyTo":"anchorCell","requiresLivingSource":true}
])"));
	pg::Registries registries;
	ASSERT_NO_THROW(registries.loadAll(data.path()));
	auto session = makeSession(registries, "runtime-spatial");
	ASSERT_NE(session, nullptr);
	const pg::BattleSnapshot before = session->snapshot();
	const pg::BoardCell source = *before.units[0].cell;
	const pg::BoardCell anchor = *before.units[1].cell;
	const pg::CommandResult result = session->submit(pg::CastAbilityCommand{pg::BattleUnitId{1}, "runtime-spatial", anchor}, {pg::CommandController::Player});
	ASSERT_TRUE(std::holds_alternative<pg::AcceptedCommand>(result));
	const std::vector<pg::BattleEvent> events = session->copyEvents(std::get<pg::AcceptedCommand>(result).events);
	EXPECT_EQ(eventNames(events), (std::vector<std::string>{"resourceSpent", "abilityCast", "unitMovementStep", "unitDisplaced", "unitsSwapped", "unitMovementStep", "unitMovementStep", "unitMovementStep", "unitTeleported"}));
	const pg::UnitDisplaced &displaced = requirePayload<pg::UnitDisplaced>(events);
	EXPECT_EQ(displaced.target, pg::BattleUnitId{2});
	EXPECT_EQ(displaced.originalCell, anchor);
	EXPECT_EQ(displaced.finalCell, (pg::BoardCell{anchor.x, anchor.y, anchor.z - 1}));
	EXPECT_EQ(displaced.lockedDirectionX, 0);
	EXPECT_EQ(displaced.lockedDirectionZ, -1);
	EXPECT_EQ(displaced.requestedDistance, 1);
	EXPECT_EQ(displaced.appliedDistance, 1);
	const pg::BattleSnapshot after = session->snapshot();
	EXPECT_EQ(after.units[0].cell, anchor); // teleport uses the captured anchor, not target recapture.
	EXPECT_EQ(after.units[1].cell, source);
}

TEST(EffectResolverRuntime, ReportsMissingPrimaryScopeAndFailedTeleportWithoutReinterpretingScope)
{
	RuntimeData data("spatial-failures");
	data.ability("runtime-swap", ability(R"([{"id":"swap","type":"swapWithSource","applyTo":"primaryUnit","requiresLivingSource":true}])"));
	data.ability("runtime-teleport", ability(R"([{"id":"teleport","type":"teleportSource","applyTo":"anchorCell","requiresLivingSource":true}])"));
	pg::Registries registries;
	ASSERT_NO_THROW(registries.loadAll(data.path()));
	auto session = makeSession(registries, "runtime-swap");
	ASSERT_NE(session, nullptr);
	const auto previews = session->abilityAnchors(pg::BattleUnitId{1}, "runtime-swap");
	const auto empty = std::find_if(previews.begin(), previews.end(), [](const pg::AbilityAnchorPreview &preview) {
		return preview.legal && preview.plan.has_value() && !preview.plan->primaryUnit.has_value();
	});
	ASSERT_NE(empty, previews.end());
	const pg::CommandResult swap = session->submit(pg::CastAbilityCommand{pg::BattleUnitId{1}, "runtime-swap", empty->anchor}, {pg::CommandController::Player});
	ASSERT_TRUE(std::holds_alternative<pg::AcceptedCommand>(swap));
	const auto &skip = requirePayload<pg::EffectApplicationSkipped>(session->copyEvents(std::get<pg::AcceptedCommand>(swap).events));
	EXPECT_EQ(skip.reason, pg::EffectSkipReason::MissingScopeValue);

	// A fresh activation/session keeps AP availability independent of the first diagnostic cast.
	auto blockedSession = makeSession(registries, "runtime-teleport");
	ASSERT_NE(blockedSession, nullptr);
	const pg::BoardCell occupied = *blockedSession->snapshot().units[1].cell;
	const pg::CommandResult teleport = blockedSession->submit(pg::CastAbilityCommand{pg::BattleUnitId{1}, "runtime-teleport", occupied}, {pg::CommandController::Player});
	ASSERT_TRUE(std::holds_alternative<pg::AcceptedCommand>(teleport));
	const auto &blocked = requirePayload<pg::EffectApplicationSkipped>(blockedSession->copyEvents(std::get<pg::AcceptedCommand>(teleport).events));
	EXPECT_EQ(blocked.reason, pg::EffectSkipReason::DestinationBlocked);
	EXPECT_EQ(blocked.targetCell, occupied);
}

TEST(EffectResolverRuntime, ZeroAxisDisplacementRecordsTheAggregateBeforeItsDiagnostic)
{
	RuntimeData data("displace-zero-axis");
	data.ability("runtime-zero-axis", ability(R"([
{"id":"teleport-up","type":"teleportSource","applyTo":"anchorCell","requiresLivingSource":true},
{"id":"zero-axis","type":"displace","applyTo":"affectedUnits","requiresLivingSource":true,"direction":"awayFromSource","distance":2}
])"));
	pg::Registries registries;
	ASSERT_NO_THROW(registries.loadAll(data.path()));

	const auto layer = [](char symbol) {
		return std::string(5, symbol) + "\n" + std::string(5, symbol) + "\n" + std::string(5, symbol) + "\n" + std::string(5, symbol) + "\n" + std::string(5, symbol);
	};
	pgtest::BoardFixture::Request board;
	board.layers = {layer('#'), layer('.'), layer('.'), layer('#'), layer('.'), layer('.')};
	board.deploymentDepth = 1;
	board.playerSeatCount = 1;
	board.enemySeatCount = 1;
	auto session = makeSession(
		registries,
		"runtime-zero-axis",
		std::move(board),
		pg::FixedOpponentPlacementPolicy{{{0, 4}}});
	ASSERT_NE(session, nullptr);
	const pg::BattleSnapshot before = session->snapshot();
	const pg::BoardCell target = *before.units[1].cell;
	const pg::BoardCell elevatedAnchor{target.x, target.y + 3, target.z};
	ASSERT_NE(before.units[0].cell, std::optional<pg::BoardCell>(elevatedAnchor));
	const pg::CommandResult result = session->submit(
		pg::CastAbilityCommand{pg::BattleUnitId{1}, "runtime-zero-axis", elevatedAnchor}, {pg::CommandController::Player});
	ASSERT_TRUE(std::holds_alternative<pg::AcceptedCommand>(result))
		<< (std::holds_alternative<pg::RejectedCommand>(result)
				? std::string(pg::toString(std::get<pg::RejectedCommand>(result).reason))
				: "aborted");
	const std::vector<pg::BattleEvent> events = session->copyEvents(std::get<pg::AcceptedCommand>(result).events);
	const pg::UnitDisplaced &displaced = requirePayload<pg::UnitDisplaced>(events);
	EXPECT_EQ(displaced.originalCell, target);
	EXPECT_EQ(displaced.finalCell, target);
	EXPECT_EQ(displaced.lockedDirectionX, 0);
	EXPECT_EQ(displaced.lockedDirectionZ, 0);
	EXPECT_EQ(displaced.requestedDistance, 2);
	EXPECT_EQ(displaced.appliedDistance, 0);
	const auto aggregate = std::find_if(events.begin(), events.end(), [](const pg::BattleEvent &event) {
		return std::holds_alternative<pg::UnitDisplaced>(event.payload);
	});
	const auto diagnostic = std::find_if(events.begin(), events.end(), [](const pg::BattleEvent &event) {
		const auto *skip = std::get_if<pg::EffectApplicationSkipped>(&event.payload);
		return skip != nullptr && skip->reason == pg::EffectSkipReason::NoDirectionalAxis;
	});
	ASSERT_NE(aggregate, events.end());
	ASSERT_NE(diagnostic, events.end());
	EXPECT_LT(std::distance(events.begin(), aggregate), std::distance(events.begin(), diagnostic));
	EXPECT_EQ(session->snapshot().units[1].cell, std::optional<pg::BoardCell>(target));
}

TEST(EffectResolverRuntime, DefersDefeatUntilEffectBoundaryContinuesAfterSourceDeathAndDetectsDraw)
{
	RuntimeData data("deferred-draw");
	data.ability("runtime-draw", ability(R"([
{"id":"self","type":"damage","applyTo":"sourceUnit","requiresLivingSource":true,"kind":"physical","base":100,"strengthRatioPermille":0,"magicPowerRatioPermille":0},
{"id":"target","type":"damage","applyTo":"primaryUnit","requiresLivingSource":false,"kind":"physical","base":100,"strengthRatioPermille":0,"magicPowerRatioPermille":0}
])"));
	pg::Registries registries;
	ASSERT_NO_THROW(registries.loadAll(data.path()));
	auto session = makeSession(registries, "runtime-draw");
	ASSERT_NE(session, nullptr);
	const pg::BoardCell enemy = *session->snapshot().units[1].cell;
	const pg::CommandResult result = session->submit(pg::CastAbilityCommand{pg::BattleUnitId{1}, "runtime-draw", enemy}, {pg::CommandController::Player});
	ASSERT_TRUE(std::holds_alternative<pg::AcceptedCommand>(result));
	const std::vector<pg::BattleEvent> events = session->copyEvents(std::get<pg::AcceptedCommand>(result).events);
	EXPECT_EQ(eventNames(events), (std::vector<std::string>{"resourceSpent", "abilityCast", "damage", "unitDefeated", "unitRemoved", "damage", "unitDefeated", "unitRemoved", "activationEnded", "battleEnded"}));
	EXPECT_EQ(session->outcome(), pg::BattleOutcome::Draw);
	EXPECT_EQ(session->snapshot().units[0].removalReason, pg::RemovalReason::Defeated);
	EXPECT_EQ(session->snapshot().units[1].removalReason, pg::RemovalReason::Defeated);
}

TEST(EffectResolverRuntime, NumericResolverFailureRollsBackThenCommitsOnlyTechnicalAbortTail)
{
	RuntimeData data("numeric-rollback");
	data.ability("runtime-overflow", ability(R"([{"id":"ward","type":"applyShield","applyTo":"sourceUnit","requiresLivingSource":true,"kind":"physical","amount":1,"duration":{"type":"infinite"}}])"));
	pg::Registries registries;
	ASSERT_NO_THROW(registries.loadAll(data.path()));
	auto session = makeSession(registries, "runtime-overflow");
	ASSERT_NE(session, nullptr);
	const pg::BattleSnapshot before = session->snapshot();
	const std::size_t batchesBefore = session->archivedBatches().size();
	session->primeShieldIdsForExhaustionTest(0U);
	const pg::BoardCell enemy = *before.units[1].cell;
	const pg::CommandResult result = session->submit(pg::CastAbilityCommand{pg::BattleUnitId{1}, "runtime-overflow", enemy}, {pg::CommandController::Player});
	ASSERT_TRUE(std::holds_alternative<pg::AbortedCommand>(result));
	const pg::AbortedCommand &aborted = std::get<pg::AbortedCommand>(result);
	EXPECT_EQ(aborted.reason, pg::BattleAbortReason::NumericInvariant);
	EXPECT_EQ(session->archivedBatches().size(), batchesBefore + 1U);
	EXPECT_EQ(session->archivedBatches().back().kind, pg::BattleBatchKind::TechnicalAbort);
	EXPECT_EQ(eventNames(session->copyEvents(aborted.events)), (std::vector<std::string>{"activationEnded", "battleAborted", "battleEnded"}));
	const pg::BattleSnapshot after = session->snapshot();
	EXPECT_EQ(after.units[0].health, before.units[0].health);
	EXPECT_EQ(after.units[0].actionPoints, before.units[0].actionPoints);
	EXPECT_EQ(after.units[0].movementPoints, before.units[0].movementPoints);
	EXPECT_TRUE(after.units[0].shields.empty());
	EXPECT_EQ(after.phase, pg::BattlePhase::Terminal);
	EXPECT_EQ(after.outcome, pg::BattleOutcome::Aborted);
}

TEST(EffectResolverRuntime, ExactResolvedBatchCapacityAlsoRollsBackIntoTechnicalAbort)
{
	RuntimeData data("resolved-batch-capacity");
	data.ability("runtime-capacity", ability(R"([{"id":"ward","type":"applyShield","applyTo":"sourceUnit","requiresLivingSource":true,"kind":"physical","amount":1,"duration":{"type":"infinite"}}])"));
	pg::Registries registries;
	ASSERT_NO_THROW(registries.loadAll(data.path()));
	auto session = makeSession(registries, "runtime-capacity");
	ASSERT_NE(session, nullptr);
	const pg::BattleSnapshot before = session->snapshot();
	const std::size_t batchesBefore = session->archivedBatches().size();
	// A three-event cast (cost, AbilityCast, ShieldApplied) still enters the transaction, but it
	// cannot coexist with the three-event terminal-abort reserve. The fully resolved staged count,
	// rather than a coarse per-effect estimate, decides this after resolution and before commit.
	session->primeSequenceCountersForExhaustionTest(100U, 100U, std::numeric_limits<std::uint64_t>::max() - 5U);
	const pg::BoardCell enemy = *before.units[1].cell;
	const pg::CommandResult result = session->submit(
		pg::CastAbilityCommand{pg::BattleUnitId{1}, "runtime-capacity", enemy}, {pg::CommandController::Player});
	ASSERT_TRUE(std::holds_alternative<pg::AbortedCommand>(result));
	const pg::AbortedCommand &aborted = std::get<pg::AbortedCommand>(result);
	EXPECT_EQ(aborted.reason, pg::BattleAbortReason::NumericInvariant);
	EXPECT_EQ(session->archivedBatches().size(), batchesBefore + 1U);
	EXPECT_EQ(session->archivedBatches().back().kind, pg::BattleBatchKind::TechnicalAbort);
	const pg::BattleSnapshot after = session->snapshot();
	EXPECT_EQ(after.units[0].health, before.units[0].health);
	EXPECT_EQ(after.units[0].actionPoints, before.units[0].actionPoints);
	EXPECT_EQ(after.units[0].movementPoints, before.units[0].movementPoints);
	EXPECT_TRUE(after.units[0].shields.empty());
	EXPECT_EQ(after.phase, pg::BattlePhase::Terminal);
	EXPECT_EQ(after.outcome, pg::BattleOutcome::Aborted);
}

TEST(EffectResolverRuntime, ScriptedHeadlessCombatCheckpointIsDeterministic)
{
	RuntimeData data("headless-checkpoint");
	data.ability("runtime-checkpoint", ability(R"([
{"id":"enemy-ward","type":"applyShield","applyTo":"primaryUnit","requiresLivingSource":true,"kind":"physical","amount":3,"duration":{"type":"infinite"}},
{"id":"physical-spill","type":"damage","applyTo":"primaryUnit","requiresLivingSource":true,"kind":"physical","base":15,"strengthRatioPermille":0,"magicPowerRatioPermille":0},
{"id":"magical-check","type":"damage","applyTo":"primaryUnit","requiresLivingSource":true,"kind":"magical","base":1,"strengthRatioPermille":0,"magicPowerRatioPermille":0},
{"id":"refill-clamp","type":"changeResource","applyTo":"sourceUnit","requiresLivingSource":true,"resource":"actionPoints","delta":100},
{"id":"bar-delay","type":"adjustTurnBar","applyTo":"sourceUnit","requiresLivingSource":true,"deltaSeconds":-1.0},
{"id":"pull","type":"displace","applyTo":"primaryUnit","requiresLivingSource":true,"direction":"towardSource","distance":1},
{"id":"anchor-teleport","type":"teleportSource","applyTo":"anchorCell","requiresLivingSource":true},
{"id":"finish","type":"damage","applyTo":"primaryUnit","requiresLivingSource":true,"kind":"physical","base":100,"strengthRatioPermille":0,"magicPowerRatioPermille":0}
])"));
	pg::Registries registries;
	ASSERT_NO_THROW(registries.loadAll(data.path()));
	const auto run = [&]() {
		auto session = makeSession(registries, "runtime-checkpoint");
		EXPECT_NE(session, nullptr);
		const pg::BoardCell enemy = *session->snapshot().units[1].cell;
		const pg::CommandResult result = session->submit(pg::CastAbilityCommand{pg::BattleUnitId{1}, "runtime-checkpoint", enemy}, {pg::CommandController::Player});
		EXPECT_TRUE(std::holds_alternative<pg::AcceptedCommand>(result));
		return std::pair{eventNames(session->copyEvents(std::get<pg::AcceptedCommand>(result).events)), session->gameplayProgressDigest()};
	};
	const auto first = run();
	const auto second = run();
	EXPECT_EQ(first, second);
	EXPECT_EQ(first.first, (std::vector<std::string>{"resourceSpent", "abilityCast", "shieldApplied", "shieldAbsorbed", "shieldBroken", "damage", "damage", "resourceChanged", "turnBarAdjusted", "unitMovementStep", "unitDisplaced", "unitMovementStep", "unitTeleported", "damage", "unitDefeated", "unitRemoved", "battleEnded"}));
	EXPECT_EQ(first.first.back(), "battleEnded");
}
