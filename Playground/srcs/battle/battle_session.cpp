#include "battle/battle_session.hpp"

#include "battle/battle_ids.hpp"
#include "battle/battle_outcome_rules.hpp"
#include "battle/battle_state_digest.hpp"
#include "battle/effects/effect_resolver.hpp"
#include "battle/placement/placement_resolver.hpp"
#include "battle/query/battle_query_service.hpp"
#include "battle/scheduler/stamina_scheduler.hpp"
#include "battle/status/status_hook_service.hpp"
#include "core/registries.hpp"
#include "creatures/creature_attributes.hpp"
#include "creatures/creature_species_definition.hpp"
#include "statuses/status_definition.hpp"

#include <algorithm>
#include <cstdlib>
#include <exception>
#include <limits>
#include <set>
#include <stdexcept>
#include <utility>
#include <variant>

namespace pg
{
	namespace
	{
		[[nodiscard]] bool controllerControlsPlayer(CommandController p_controller) noexcept
		{
			return p_controller == CommandController::Player || p_controller == CommandController::DebugAutoplay;
		}

		[[nodiscard]] std::optional<ActivationEndReason> endReason(CommandController p_controller, EndTurnRequestCause p_cause)
		{
			if (p_controller == CommandController::Player && p_cause == EndTurnRequestCause::Explicit)
			{
				return ActivationEndReason::Explicit;
			}
			if (p_controller != CommandController::EnemyAi && p_controller != CommandController::DebugAutoplay)
			{
				return std::nullopt;
			}
			switch (p_cause)
			{
			case EndTurnRequestCause::Explicit:
				return ActivationEndReason::Explicit;
			case EndTurnRequestCause::AiRepeatedStateGuard:
				return ActivationEndReason::AiRepeatedStateGuard;
			case EndTurnRequestCause::AiCommandCap:
				return ActivationEndReason::AiCommandCap;
			case EndTurnRequestCause::AiInvalidPlannedCommand:
				return ActivationEndReason::AiInvalidPlannedCommand;
			case EndTurnRequestCause::AiNoRuleProducedCommand:
				return ActivationEndReason::AiNoRuleProducedCommand;
			}
			return std::nullopt;
		}

		[[nodiscard]] std::optional<int> closestDistance(const BattleContext &p_context, BattleUnitId p_source, bool p_allies)
		{
			const std::optional<BoardCell> sourceCell = p_context.board().occupancy().cellOf(p_source);
			if (!sourceCell.has_value())
			{
				return std::nullopt;
			}
			const BattleSide side = p_context.unit(p_source).side();
			std::optional<int> closest;
			for (const BattleUnitId id : p_context.allUnitsInOrder())
			{
				if (id == p_source || (p_context.unit(id).side() == side) != p_allies || !p_context.unit(id).isActiveCombatant())
				{
					continue;
				}
				const std::optional<BoardCell> cell = p_context.board().occupancy().cellOf(id);
				if (!cell.has_value())
				{
					continue;
				}
				const int distance = std::abs(sourceCell->x - cell->x) + std::abs(sourceCell->z - cell->z);
				if (!closest.has_value() || distance < *closest)
				{
					closest = distance;
				}
			}
			return closest;
		}

		[[nodiscard]] bool kindMatchesSource(EncounterKind p_kind, BoardSourceKind p_source) noexcept
		{
			switch (p_kind)
			{
			case EncounterKind::Wild:
			case EncounterKind::Trainer:
				return p_source == BoardSourceKind::LiveWorld;
			case EncounterKind::Gym:
			case EncounterKind::Special:
				return p_source == BoardSourceKind::Handcrafted;
			case EncounterKind::Debug:
				return true;
			}
			return false;
		}

		// A RAII latch for the re-entry guard: a future synchronous event subscriber that tried to
		// submit while a batch is still being finalised gets SessionBusy instead of a corrupt log.
		class ResolutionGuard
		{
		private:
			bool &_flag;

		public:
			explicit ResolutionGuard(bool &p_flag) noexcept :
				_flag(p_flag)
			{
				_flag = true;
			}
			~ResolutionGuard()
			{
				_flag = false;
			}
			ResolutionGuard(const ResolutionGuard &) = delete;
			ResolutionGuard &operator=(const ResolutionGuard &) = delete;
		};
	}

	// ---- BattleConstructionResult (out of line, BattleSession now complete) ---------------------

	BattleConstructionResult::BattleConstructionResult(std::unique_ptr<BattleSession> p_session) :
		session(std::move(p_session))
	{
		if (session == nullptr)
		{
			throw std::invalid_argument("a successful battle construction result needs a session");
		}
	}

	BattleConstructionResult::BattleConstructionResult(BattleConstructionError p_error) :
		error(std::move(p_error))
	{
	}

	BattleConstructionResult::BattleConstructionResult(BattleConstructionResult &&) noexcept = default;
	BattleConstructionResult &BattleConstructionResult::operator=(BattleConstructionResult &&) noexcept = default;
	BattleConstructionResult::~BattleConstructionResult() = default;

	// ---- BattleSession --------------------------------------------------------------------------

	BattleSession::BattleSession(std::unique_ptr<BattleContext> p_context) noexcept :
		_context(std::move(p_context))
	{
	}

	BattleSession::~BattleSession() = default;

	BattleConstructionResult BattleSession::create(
		BattleConstructionRequest p_request,
		const Registries &p_registries,
		BoardData p_board)
	{
		const BattleDescriptor &descriptor = p_request.descriptor;

		const auto fail = [&](BattleConstructionErrorCode p_code,
							  std::string p_detail,
							  std::optional<BattleSide> p_side = std::nullopt,
							  std::optional<std::uint32_t> p_rosterOrder = std::nullopt,
							  std::string p_referenced = std::string()) {
			BattleConstructionError error;
			error.code = p_code;
			error.encounterDefinitionId = descriptor.encounterDefinitionId;
			error.side = p_side;
			error.rosterOrder = p_rosterOrder;
			error.referencedId = std::move(p_referenced);
			error.diagnosticDetail = std::move(p_detail);
			return BattleConstructionResult(std::move(error));
		};

		// 1. Descriptor: non-empty identities and a battle id that is exactly the seed fold.
		if (descriptor.stableEncounterIdentity.empty() || descriptor.encounterDefinitionId.empty() ||
			descriptor.resolvedTeamId.empty())
		{
			return fail(BattleConstructionErrorCode::InvalidDescriptor, "descriptor identity strings must be non-empty");
		}
		if (!(deriveBattleId(descriptor.combatSeed) == descriptor.battleId))
		{
			return fail(
				BattleConstructionErrorCode::InvalidDescriptor,
				"battleId must equal deriveBattleId(combatSeed): the session never re-derives combat RNG");
		}

		// 2. Board source: the descriptor copy must equal the board's own descriptor, and the encounter
		//    kind must pair with the source kind.
		if (!(descriptor.boardSource == p_board.sourceDescriptor()))
		{
			return fail(
				BattleConstructionErrorCode::BoardSourceMismatch,
				"descriptor board source does not equal the board's own source descriptor");
		}
		if (!kindMatchesSource(descriptor.encounterKind, p_board.sourceDescriptor().kind))
		{
			return fail(
				BattleConstructionErrorCode::BoardSourceMismatch,
				"encounter kind and board source kind are an illegal pair");
		}

		// 3. Participant counts, roster order and provenance shape.
		std::vector<const BattleParticipantSeed *> players;
		std::vector<const BattleParticipantSeed *> enemies;
		for (const BattleParticipantSeed &seed : p_request.participants)
		{
			(seed.side == BattleSide::Player ? players : enemies).push_back(&seed);
		}
		if (players.empty() || players.size() > 6)
		{
			return fail(
				BattleConstructionErrorCode::InvalidParticipantCount, "player side needs 1..6 units", BattleSide::Player);
		}
		if (enemies.empty() || enemies.size() > 6)
		{
			return fail(
				BattleConstructionErrorCode::InvalidParticipantCount, "enemy side needs 1..6 units", BattleSide::Enemy);
		}

		const auto validateSide = [&](BattleSide p_side, const std::vector<const BattleParticipantSeed *> &p_seeds)
			-> std::optional<BattleConstructionResult> {
			std::set<std::uint32_t> orders;
			std::set<std::string> identities;
			for (const BattleParticipantSeed *seed : p_seeds)
			{
				if (!orders.insert(seed->rosterOrder).second)
				{
					return fail(
						BattleConstructionErrorCode::InvalidRosterOrder, "duplicate roster order", p_side, seed->rosterOrder);
				}
				if (p_side == BattleSide::Player)
				{
					if (!seed->persistentCreatureId.has_value() || seed->encounterSpawnMemberId.has_value())
					{
						return fail(
							BattleConstructionErrorCode::InvalidRosterOrder,
							"a player seed carries a persistent creature id and no spawn-member id",
							p_side,
							seed->rosterOrder);
					}
					if (!identities.insert(std::string(seed->persistentCreatureId->string())).second)
					{
						return fail(
							BattleConstructionErrorCode::DuplicateParticipantIdentity,
							"two player seeds share one persistent creature id",
							p_side,
							seed->rosterOrder);
					}
				}
				else
				{
					if (seed->persistentCreatureId.has_value() || !seed->encounterSpawnMemberId.has_value() ||
						seed->encounterSpawnMemberId->empty())
					{
						return fail(
							BattleConstructionErrorCode::InvalidRosterOrder,
							"an enemy seed carries a non-empty spawn-member id and no persistent id",
							p_side,
							seed->rosterOrder);
					}
					if (!identities.insert(*seed->encounterSpawnMemberId).second)
					{
						return fail(
							BattleConstructionErrorCode::DuplicateParticipantIdentity,
							"two enemy seeds share one spawn-member id",
							p_side,
							seed->rosterOrder);
					}
				}
			}
			return std::nullopt;
		};
		if (std::optional<BattleConstructionResult> error = validateSide(BattleSide::Player, players))
		{
			return std::move(*error);
		}
		if (std::optional<BattleConstructionResult> error = validateSide(BattleSide::Enemy, enemies))
		{
			return std::move(*error);
		}

		// 4. Definition references and derived-loadout limits.
		for (const BattleParticipantSeed &seed : p_request.participants)
		{
			const CreatureSpeciesDefinition *species = p_registries.species().tryGet(seed.speciesId);
			if (species == nullptr)
			{
				return fail(
					BattleConstructionErrorCode::UnknownDefinitionReference,
					"unknown species",
					seed.side,
					seed.rosterOrder,
					seed.speciesId);
			}
			if (species->tryForm(seed.formId) == nullptr)
			{
				return fail(
					BattleConstructionErrorCode::UnknownDefinitionReference,
					"unknown form on species",
					seed.side,
					seed.rosterOrder,
					seed.formId);
			}
			if (seed.abilityIds.size() > p_registries.gameRules().battle.abilitySlotCapacity)
			{
				return fail(
					BattleConstructionErrorCode::InvalidDerivedLoadout,
					"more abilities than the ability-slot capacity",
					seed.side,
					seed.rosterOrder);
			}
			for (const std::string &abilityId : seed.abilityIds)
			{
				if (!p_registries.abilities().contains(abilityId))
				{
					return fail(
						BattleConstructionErrorCode::UnknownDefinitionReference,
						"unknown ability",
						seed.side,
						seed.rosterOrder,
						abilityId);
				}
			}
			for (const std::string &statusId : seed.passiveStatusIds)
			{
				const StatusDefinition *status = p_registries.statuses().tryGet(statusId);
				if (status == nullptr)
				{
					return fail(
						BattleConstructionErrorCode::UnknownDefinitionReference,
						"unknown passive status",
						seed.side,
						seed.rosterOrder,
						statusId);
				}
				if (isStunStatus(*status))
				{
					return fail(
						BattleConstructionErrorCode::InvalidDerivedLoadout,
						"a passive may not carry the reserved stun tag",
						seed.side,
						seed.rosterOrder,
						statusId);
				}
			}
			if (seed.side == BattleSide::Enemy)
			{
				if (!seed.aiBehaviourId.has_value() || !p_registries.aiBehaviours().contains(*seed.aiBehaviourId))
				{
					return fail(
						BattleConstructionErrorCode::UnknownDefinitionReference,
						"unknown or missing enemy AI behaviour",
						seed.side,
						seed.rosterOrder,
						seed.aiBehaviourId.value_or(std::string()));
				}
			}
			try
			{
				requireValidAttributes(
					seed.attributes, p_registries.gameRules(), DefinitionSource{}, seed.speciesId);
			} catch (const std::exception &)
			{
				return fail(
					BattleConstructionErrorCode::InvalidDerivedLoadout,
					"derived attributes are outside the game-rule bounds",
					seed.side,
					seed.rosterOrder);
			}
		}

		// 5. The board must be able to seat both sides.
		if (p_board.deployment().cells(BattleSide::Player).size() < players.size())
		{
			return fail(
				BattleConstructionErrorCode::InsufficientDeploymentCapacity,
				"the player deployment zone cannot seat the player roster",
				BattleSide::Player);
		}
		if (p_board.deployment().cells(BattleSide::Enemy).size() < enemies.size())
		{
			return fail(
				BattleConstructionErrorCode::InsufficientDeploymentCapacity,
				"the enemy deployment zone cannot seat the enemy roster",
				BattleSide::Enemy);
		}

		// 6. Allocate ids (players first in roster order, then enemies) and project units.
		std::sort(players.begin(), players.end(), [](const BattleParticipantSeed *p_a, const BattleParticipantSeed *p_b) {
			return p_a->rosterOrder < p_b->rosterOrder;
		});
		std::sort(enemies.begin(), enemies.end(), [](const BattleParticipantSeed *p_a, const BattleParticipantSeed *p_b) {
			return p_a->rosterOrder < p_b->rosterOrder;
		});

		BattleUnitIdAllocator allocator;
		std::map<BattleUnitId, BattleUnit> units;
		std::vector<BattleUnitId> playerOrder;
		std::vector<BattleUnitId> enemyOrder;
		for (const BattleParticipantSeed *seed : players)
		{
			const BattleUnitId id = allocator.allocate();
			units.emplace(id, BattleUnit::project(id, *seed));
			playerOrder.push_back(id);
		}
		for (const BattleParticipantSeed *seed : enemies)
		{
			const BattleUnitId id = allocator.allocate();
			units.emplace(id, BattleUnit::project(id, *seed));
			enemyOrder.push_back(id);
		}

		// 7. Build the context, then run the construction transaction against it.
		auto context = std::make_unique<BattleContext>(
			descriptor,
			p_registries,
			std::move(p_board),
			std::move(units),
			std::move(playerOrder),
			std::move(enemyOrder),
			BattleRng(descriptor.combatSeed));
		// Passive modifiers take effect as soon as the battle-local unit exists. Their Applied hooks
		// are deliberately delayed until both sides finish deployment.
		context->projectPassiveStatuses();

		const BattleSnapshot before = context->snapshot();
		std::vector<StagedEvent> staged;

		BattleStarted started;
		started.battleId = descriptor.battleId;
		started.stableEncounterIdentity = descriptor.stableEncounterIdentity;
		started.encounterDefinitionId = descriptor.encounterDefinitionId;
		started.encounterDisplayName = descriptor.encounterDisplayName;
		started.encounterKind = descriptor.encounterKind;
		started.boardSource = context->board().sourceDescriptor();
		started.participants = context->allUnitsInOrder();
		staged.push_back(StagedEvent{BattleEventCategory::Lifecycle, BattleEventOrigin{}, started});

		// Enemy auto-deployment through the shared board mutation path, System-issued.
		const EnemyPlacementPlan plan = planEnemyPlacements(
			context->board(),
			p_request.enemyPlacement,
			context->enemyOrder().size(),
			context->rng(),
			descriptor.encounterDefinitionId);
		if (!plan.ok())
		{
			return BattleConstructionResult(*plan.error);
		}
		for (std::size_t index = 0; index < context->enemyOrder().size(); ++index)
		{
			const BattleUnitId enemyId = context->enemyOrder()[index];
			const BoardCell cell = plan.cells[index];
			if (!context->placeUnitOccupancy(enemyId, cell))
			{
				return fail(
					BattleConstructionErrorCode::EnemyPlacementInvalid,
					"a planned enemy placement was rejected by the board",
					BattleSide::Enemy,
					index);
			}
			UnitDeploymentChanged deployment;
			deployment.unit = enemyId;
			deployment.previousCell = std::nullopt;
			deployment.newCell = cell;
			BattleEventOrigin origin;
			origin.sourceUnit = enemyId;
			staged.push_back(StagedEvent{BattleEventCategory::Gameplay, origin, deployment});
		}

		context->setSideConfirmed(BattleSide::Enemy, true);
		DeploymentConfirmed enemyConfirmed;
		enemyConfirmed.side = BattleSide::Enemy;
		enemyConfirmed.placedUnits = context->enemyOrder();
		staged.push_back(StagedEvent{BattleEventCategory::Lifecycle, BattleEventOrigin{}, enemyConfirmed});

		if (!context->hasOrdinaryCapacity(staged.size(), false))
		{
			return fail(
				BattleConstructionErrorCode::NumericOverflow, "the construction batch would exhaust the id space");
		}
		(void)context->commitOrdinaryBatch(BattleBatchKind::Construction, before, staged);

		return BattleConstructionResult(std::unique_ptr<BattleSession>(new BattleSession(std::move(context))));
	}

	CommandResult BattleSession::submit(const BattleCommand &p_command, CommandIssuer p_issuer)
	{
		if (_resolving)
		{
			return RejectedCommand{CommandRejection::SessionBusy};
		}
		ResolutionGuard guard(_resolving);

		if (_context->isTerminal())
		{
			return RejectedCommand{CommandRejection::BattleTerminal};
		}

		return std::visit(
			[&](const auto &p_specific) -> CommandResult {
				using Command = std::decay_t<decltype(p_specific)>;
				if constexpr (std::is_same_v<Command, PlaceUnitCommand>)
				{
					return _placeUnit(p_specific, p_issuer);
				}
				else if constexpr (std::is_same_v<Command, ConfirmDeploymentCommand>)
				{
					return _confirmDeployment(p_specific, p_issuer);
				}
				else if constexpr (std::is_same_v<Command, EndTurnCommand>)
				{
					return _endTurn(p_specific, p_issuer);
				}
				else if constexpr (std::is_same_v<Command, MoveCommand>)
				{
					return _move(p_specific, p_issuer);
				}
				else if constexpr (std::is_same_v<Command, CastAbilityCommand>)
				{
					return _cast(p_specific, p_issuer);
				}
				else
				{
					// Move/Cast have no activation runtime yet; their shapes are final but they
					// resolve to nothing in step 06.
					return RejectedCommand{CommandRejection::CommandUnavailable};
				}
			},
			p_command);
	}

	CommandResult BattleSession::_move(const MoveCommand &p_command, CommandIssuer p_issuer)
	{
		BattleContext &context = *_context;
		if (context.phase() != BattlePhase::Activation)
		{
			return RejectedCommand{CommandRejection::WrongPhase};
		}
		const BattleUnit *unit = context.tryUnit(p_command.unit);
		if (unit == nullptr)
		{
			return RejectedCommand{CommandRejection::UnknownUnit};
		}
		if (!context.activeUnit().has_value() || *context.activeUnit() != p_command.unit)
		{
			return RejectedCommand{CommandRejection::NotActiveUnit};
		}
		if ((unit->side() == BattleSide::Player && !controllerControlsPlayer(p_issuer.controller)) || (unit->side() == BattleSide::Enemy && p_issuer.controller != CommandController::EnemyAi))
		{
			return RejectedCommand{CommandRejection::WrongController};
		}
		auto planned = BattleQueryService(context).planMove(p_command.unit, p_command.destination);
		if (!planned)
		{
			return RejectedCommand{planned.error()};
		}
		if (!context.hasOrdinaryCapacity(planned->steps.size() * 2 + 1, true))
		{
			return _abort(BattleAbortReason::InternalInvariantViolation);
		}
		const BattleSnapshot before = context.snapshot();
		std::vector<StagedEvent> staged;
		BoardCell actualFinal = planned->origin;
		int appliedSteps = 0;
		int totalMovementSpent = 0;
		for (const PlannedPathStep &step : planned->steps)
		{
			BattleUnit &moving = context.unitMutable(p_command.unit);
			if (moving.movementPoints() < static_cast<int>(step.movementPointCost))
			{
				return _abort(BattleAbortReason::InternalInvariantViolation);
			}
			const int previous = moving.movementPoints();
			moving.setMovementPoints(previous - static_cast<int>(step.movementPointCost));
			ResourceSpent spent{p_command.unit, BattleResource::MovementPoints, static_cast<int>(step.movementPointCost), static_cast<int>(step.movementPointCost), previous, moving.movementPoints(), ResourceSpendReason::MovementCost};
			staged.push_back({BattleEventCategory::Gameplay, BattleEventOrigin{.sourceUnit = p_command.unit}, spent});
			if (!EffectResolver::applySpatialStep(
					context,
					staged,
					p_command.unit,
					step.from,
					step.to,
					static_cast<int>(step.movementPointCost),
					SpatialMoveCause::Voluntary,
					BattleEventOrigin{.sourceUnit = p_command.unit}))
			{
				return _abort(BattleAbortReason::InternalInvariantViolation);
			}
			++appliedSteps;
			totalMovementSpent += static_cast<int>(step.movementPointCost);
			const std::optional<BoardCell> current = context.board().occupancy().cellOf(p_command.unit);
			if (!context.unit(p_command.unit).isActiveCombatant() || !current.has_value() || *current != step.to)
			{
				actualFinal = current.value_or(step.to);
				break;
			}
			actualFinal = *current;
		}
		staged.push_back({BattleEventCategory::Gameplay, BattleEventOrigin{.sourceUnit = p_command.unit}, UnitMoved{p_command.unit, planned->origin, actualFinal, appliedSteps, totalMovementSpent, true}});
		if (appliedSteps > 0)
		{
			EffectResolver::finishSpatialMove(context, staged, p_command.unit, actualFinal);
			StatusHookService::dispatchStatusHook(context, staged, p_command.unit, StatusHook::AfterVoluntaryMove, p_command.unit);
		}
		context.setResolvedNonEndCommands(context.resolvedNonEndCommands() + 1);
		const CommittedBattleBatch batch = context.commitOrdinaryBatch(BattleBatchKind::Command, before, staged);
		_assertAcceptedInvariants();
		return AcceptedCommand{*batch.action, batch.id, batch.events};
	}

	CommandResult BattleSession::_cast(const CastAbilityCommand &p_command, CommandIssuer p_issuer)
	{
		BattleContext &context = *_context;
		if (context.phase() != BattlePhase::Activation)
		{
			return RejectedCommand{CommandRejection::WrongPhase};
		}
		const BattleUnit *unit = context.tryUnit(p_command.unit);
		if (!unit)
		{
			return RejectedCommand{CommandRejection::UnknownUnit};
		}
		if (!context.activeUnit().has_value() || *context.activeUnit() != p_command.unit)
		{
			return RejectedCommand{CommandRejection::NotActiveUnit};
		}
		if ((unit->side() == BattleSide::Player && !controllerControlsPlayer(p_issuer.controller)) || (unit->side() == BattleSide::Enemy && p_issuer.controller != CommandController::EnemyAi))
		{
			return RejectedCommand{CommandRejection::WrongController};
		}
		auto planned = BattleQueryService(context).planCast(p_command.unit, p_command.abilityId, p_command.anchor);
		if (!planned)
		{
			return RejectedCommand{planned.error()};
		}
		const AbilityDefinition *ability = context.registries().abilities().tryGet(p_command.abilityId);
		if (!ability || !EffectResolver::supportsAll(*ability))
		{
			return RejectedCommand{CommandRejection::EffectRuntimeUnavailable};
		}
		// The exact staged length is checked after resolution; this early check only guarantees that
		// the command can enter a rollback-safe transaction without consuming the abort reserve.
		if (!context.hasOrdinaryCapacity(0, true))
		{
			return _abort(BattleAbortReason::InternalInvariantViolation);
		}
		const BattleSnapshot before = context.snapshot();
		const BattleContext::CommandRollbackState rollback = context.captureCommandRollbackState();
		try
		{
			std::vector<StagedEvent> staged;
			BattleUnit &caster = context.unitMutable(p_command.unit);
			if (ability->cost.actionPoints > 0)
			{
				const int old = caster.actionPoints();
				caster.setActionPoints(old - ability->cost.actionPoints);
				staged.push_back({BattleEventCategory::Gameplay, BattleEventOrigin{.sourceUnit = p_command.unit, .abilityId = ability->id}, ResourceSpent{p_command.unit, BattleResource::ActionPoints, ability->cost.actionPoints, ability->cost.actionPoints, old, caster.actionPoints(), ResourceSpendReason::AbilityCost}});
			}
			if (ability->cost.movementPoints > 0)
			{
				const int old = caster.movementPoints();
				caster.setMovementPoints(old - ability->cost.movementPoints);
				staged.push_back({BattleEventCategory::Gameplay, BattleEventOrigin{.sourceUnit = p_command.unit, .abilityId = ability->id}, ResourceSpent{p_command.unit, BattleResource::MovementPoints, ability->cost.movementPoints, ability->cost.movementPoints, old, caster.movementPoints(), ResourceSpendReason::AbilityCost}});
			}
			AbilityCast cast;
			cast.source = p_command.unit;
			cast.abilityId = ability->id;
			cast.sourceCell = planned->sourceCellAtCapture;
			cast.anchorCell = planned->anchor;
			const std::int64_t dx = static_cast<std::int64_t>(planned->anchor.x) - planned->sourceCellAtCapture.x;
			const std::int64_t dz = static_cast<std::int64_t>(planned->anchor.z) - planned->sourceCellAtCapture.z;
			const std::int64_t distance = (dx < 0 ? -dx : dx) + (dz < 0 ? -dz : dz);
			if (distance > std::numeric_limits<int>::max())
			{
				throw std::overflow_error("cast target distance does not fit an event");
			}
			cast.targetDistance = static_cast<int>(distance);
			cast.affectedCells = planned->affectedCells;
			cast.affectedUnits = planned->affectedUnits;
			staged.push_back({BattleEventCategory::Gameplay, BattleEventOrigin{.sourceUnit = p_command.unit, .abilityId = ability->id}, cast});
			EffectResolver::resolveAbility(context, staged, *planned, *ability);
			if (context.resolvedNonEndCommands() == std::numeric_limits<std::size_t>::max())
			{
				throw std::overflow_error("resolved command counter overflow");
			}
			context.setResolvedNonEndCommands(context.resolvedNonEndCommands() + 1);

			const auto closeActivation = [&](ActivationEndReason p_reason) {
				if (context.unit(p_command.unit).isActiveCombatant())
				{
					if (const std::optional<BoardCell> cell = context.board().occupancy().cellOf(p_command.unit); cell.has_value())
					{
						StatusHookService::dispatchObjectTrigger(context, staged, BattleObjectTrigger::UnitActivationEndedOnCell, p_command.unit, *cell);
					}
					StatusHookService::dispatchStatusHook(context, staged, p_command.unit, StatusHook::ActivationEnd);
					context.expireCapturedOwnerActivationDurations(p_command.unit, staged);
				}
				const BattleUnit &active = context.unit(p_command.unit);
				ActivationEnded ended;
				ended.unit = p_command.unit;
				ended.turn = *context.turn();
				ended.finalCell = active.lastOccupiedCell();
				ended.finalActionPoints = active.actionPoints();
				ended.finalMovementPoints = active.movementPoints();
				ended.closestAllyDistance = closestDistance(context, p_command.unit, true);
				ended.closestEnemyDistance = closestDistance(context, p_command.unit, false);
				ended.reason = p_reason;
				context.unitMutable(p_command.unit).setTurnBarFill(BattleTime{});
				context.setActiveUnit(std::nullopt);
				context.setTurn(std::nullopt);
				context.setResolvedNonEndCommands(0);
				staged.push_back({BattleEventCategory::Lifecycle, BattleEventOrigin{.sourceUnit = p_command.unit}, ended});
			};

			// The resolver keeps all captured effects alive through a source defeat.  Only after its
			// after-cast seam does the session perform this one structural close.
			if (!context.unit(p_command.unit).isActiveCombatant())
			{
				closeActivation(ActivationEndReason::ActiveUnitDefeated);
			}
			const BattleOutcome outcome = evaluateBattleOutcome(context);
			if (outcome != BattleOutcome::Undecided)
			{
				context.setTerminal(outcome);
				BattleEnded ended;
				ended.outcome = outcome;
				ended.activePlayerCount = context.activeCount(BattleSide::Player);
				ended.activeEnemyCount = context.activeCount(BattleSide::Enemy);
				ended.notDefeatedPlayerCount = context.notDefeatedCount(BattleSide::Player);
				ended.notDefeatedEnemyCount = context.notDefeatedCount(BattleSide::Enemy);
				staged.push_back({BattleEventCategory::Lifecycle, {}, ended});
			}
			else if (context.activeUnit().has_value())
			{
				const bool commandCap = context.resolvedNonEndCommands() >= context.registries().gameRules().battle.maxCommandsPerActivation;
				const bool noLegalCommand = !BattleQueryService(context).hasAnyLegalNonEndCommand(*context.activeUnit());
				if (commandCap || noLegalCommand)
				{
					closeActivation(commandCap ? ActivationEndReason::CommandCap : ActivationEndReason::NoLegalCommands);
					context.setPhase(BattlePhase::AwaitingActivation);
				}
			}
			else
			{
				context.setPhase(BattlePhase::AwaitingActivation);
			}
			if (!context.hasOrdinaryCapacity(staged.size(), true))
			{
				throw std::overflow_error("resolved cast events exhaust the checked sequence space");
			}

			const CommittedBattleBatch batch = context.commitOrdinaryBatch(BattleBatchKind::Command, before, staged);
			_assertAcceptedInvariants();
			return AcceptedCommand{*batch.action, batch.id, batch.events};
		} catch (const std::overflow_error &)
		{
			// No staged event or counter is committed before this point.  Roll back gameplay state,
			// including shield allocation, then record the sole terminal TechnicalAbort batch.
			context.restoreCommandRollbackState(rollback);
			return _abort(BattleAbortReason::NumericInvariant);
		}
	}

	CommandResult BattleSession::_placeUnit(const PlaceUnitCommand &p_command, CommandIssuer p_issuer)
	{
		BattleContext &context = *_context;

		if (context.phase() != BattlePhase::Deployment)
		{
			return RejectedCommand{CommandRejection::WrongPhase};
		}
		const BattleUnit *unit = context.tryUnit(p_command.unit);
		if (unit == nullptr)
		{
			return RejectedCommand{CommandRejection::UnknownUnit};
		}
		// Enemy placement is System-only (transactional construction); public UI/AI never place enemies.
		if (unit->side() != BattleSide::Player || !controllerControlsPlayer(p_issuer.controller))
		{
			return RejectedCommand{CommandRejection::WrongController};
		}
		if (unit->removalReason() != RemovalReason::None)
		{
			return RejectedCommand{CommandRejection::UnitRemoved};
		}
		if (context.sideConfirmed(unit->side()))
		{
			return RejectedCommand{CommandRejection::UnitAlreadyConfirmed};
		}

		const std::optional<BoardCell> current = context.board().occupancy().cellOf(p_command.unit);
		if (current.has_value() && *current == p_command.destination)
		{
			return RejectedCommand{CommandRejection::NoStateChange};
		}
		if (!context.board().isStandable(p_command.destination))
		{
			return RejectedCommand{CommandRejection::DestinationNotStandable};
		}
		if (!context.board().deployment().contains(unit->side(), p_command.destination))
		{
			return RejectedCommand{CommandRejection::UnitOutsideDeploymentZone};
		}

		enum class Kind
		{
			NewPlace,
			Reposition,
			Swap
		};
		Kind kind = current.has_value() ? Kind::Reposition : Kind::NewPlace;
		BattleUnitId swapPartner;
		if (const std::optional<BattleUnitId> occupant = context.board().occupancy().unitAt(p_command.destination))
		{
			const BattleUnit &other = context.unit(*occupant);
			// Only an unconfirmed friendly (which its side is, since this side is unconfirmed) swaps; an
			// undeployed unit has no cell to give, so it cannot swap.
			if (other.side() != unit->side() || other.removalReason() != RemovalReason::None || !current.has_value())
			{
				return RejectedCommand{CommandRejection::DestinationOccupied};
			}
			kind = Kind::Swap;
			swapPartner = *occupant;
		}

		const std::size_t eventCount = kind == Kind::Swap ? 2 : 1;

		// Live terrain that moved under the board is a controlled abort; a handcrafted board is always
		// current. The capacity preflight keeps the terminal-abort reserve intact.
		if (!context.board().terrainIsCurrent())
		{
			return _abort(BattleAbortReason::RequiredTerrainChanged);
		}
		if (!context.hasOrdinaryCapacity(eventCount, true))
		{
			return _abort(BattleAbortReason::InternalInvariantViolation);
		}

		const BattleSnapshot before = context.snapshot();
		std::vector<StagedEvent> staged;
		const auto deploymentEvent = [](BattleUnitId p_who,
										std::optional<BoardCell> p_previous,
										BoardCell p_new,
										std::optional<BattleUnitId> p_partner) {
			UnitDeploymentChanged payload;
			payload.unit = p_who;
			payload.previousCell = p_previous;
			payload.newCell = p_new;
			payload.swapPartner = p_partner;
			BattleEventOrigin origin;
			origin.sourceUnit = p_who;
			return StagedEvent{BattleEventCategory::Gameplay, origin, payload};
		};

		if (kind == Kind::NewPlace)
		{
			if (!context.placeUnitOccupancy(p_command.unit, p_command.destination))
			{
				return _abort(BattleAbortReason::InternalInvariantViolation);
			}
			staged.push_back(deploymentEvent(p_command.unit, std::nullopt, p_command.destination, std::nullopt));
		}
		else if (kind == Kind::Reposition)
		{
			if (!context.moveUnitOccupancy(p_command.unit, p_command.destination))
			{
				return _abort(BattleAbortReason::InternalInvariantViolation);
			}
			staged.push_back(deploymentEvent(p_command.unit, current, p_command.destination, std::nullopt));
		}
		else
		{
			const BoardCell unitOld = *current;
			const BoardCell partnerOld = p_command.destination;
			if (!context.swapUnitsOccupancy(p_command.unit, swapPartner))
			{
				return _abort(BattleAbortReason::InternalInvariantViolation);
			}
			// The pair is emitted ordered by unit id; each unit's new cell is the other's former cell.
			StagedEvent moverEvent = deploymentEvent(p_command.unit, unitOld, partnerOld, swapPartner);
			StagedEvent partnerEvent = deploymentEvent(swapPartner, partnerOld, unitOld, p_command.unit);
			if (p_command.unit < swapPartner)
			{
				staged.push_back(std::move(moverEvent));
				staged.push_back(std::move(partnerEvent));
			}
			else
			{
				staged.push_back(std::move(partnerEvent));
				staged.push_back(std::move(moverEvent));
			}
		}

		const CommittedBattleBatch batch = context.commitOrdinaryBatch(BattleBatchKind::Command, before, staged);
		_assertAcceptedInvariants();
		return AcceptedCommand{batch.action.value(), batch.id, batch.events};
	}

	CommandResult BattleSession::_confirmDeployment(const ConfirmDeploymentCommand &p_command, CommandIssuer p_issuer)
	{
		BattleContext &context = *_context;

		// Only the player confirms through the gateway; the enemy is auto-confirmed at construction.
		if (p_command.side != BattleSide::Player || !controllerControlsPlayer(p_issuer.controller))
		{
			return RejectedCommand{CommandRejection::WrongController};
		}
		if (context.phase() != BattlePhase::Deployment)
		{
			return RejectedCommand{CommandRejection::WrongPhase};
		}
		if (context.sideConfirmed(p_command.side))
		{
			return RejectedCommand{CommandRejection::UnitAlreadyConfirmed};
		}
		for (const BattleUnitId id : context.sideOrder(p_command.side))
		{
			const std::optional<BoardCell> cell = context.board().occupancy().cellOf(id);
			if (!cell.has_value() || !context.board().deployment().contains(p_command.side, *cell))
			{
				return RejectedCommand{CommandRejection::DeploymentIncomplete};
			}
		}

		if (!context.board().terrainIsCurrent())
		{
			return _abort(BattleAbortReason::RequiredTerrainChanged);
		}
		if (!context.hasOrdinaryCapacity(2, true))
		{
			return _abort(BattleAbortReason::InternalInvariantViolation);
		}

		const BattleSnapshot before = context.snapshot();
		context.setSideConfirmed(p_command.side, true);
		context.setPhase(BattlePhase::AwaitingActivation);
		DeploymentConfirmed confirmed;
		confirmed.side = p_command.side;
		confirmed.placedUnits = context.sideOrder(p_command.side);
		DeploymentCompleted completed;
		completed.playerUnits = context.playerOrder();
		completed.enemyUnits = context.enemyOrder();
		std::vector<StagedEvent> staged{
			StagedEvent{BattleEventCategory::Lifecycle, BattleEventOrigin{}, confirmed},
			StagedEvent{BattleEventCategory::Lifecycle, BattleEventOrigin{}, completed}};
		for (const BattleUnitId id : context.allUnitsInOrder())
		{
			StatusHookService::dispatchStatusHook(context, staged, id, StatusHook::Applied);
		}

		const CommittedBattleBatch batch = context.commitOrdinaryBatch(BattleBatchKind::Command, before, staged);
		_assertAcceptedInvariants();
		return AcceptedCommand{batch.action.value(), batch.id, batch.events};
	}

	CommandResult BattleSession::_endTurn(const EndTurnCommand &p_command, CommandIssuer p_issuer)
	{
		BattleContext &context = *_context;
		if (context.phase() != BattlePhase::Activation)
		{
			return RejectedCommand{CommandRejection::WrongPhase};
		}
		const BattleUnit *unit = context.tryUnit(p_command.unit);
		if (unit == nullptr || !context.activeUnit().has_value() || *context.activeUnit() != p_command.unit)
		{
			return RejectedCommand{CommandRejection::CommandUnavailable};
		}
		if ((unit->side() == BattleSide::Player && !controllerControlsPlayer(p_issuer.controller)) ||
			(unit->side() == BattleSide::Enemy && p_issuer.controller != CommandController::EnemyAi))
		{
			return RejectedCommand{CommandRejection::WrongController};
		}
		const std::optional<ActivationEndReason> reason = endReason(p_issuer.controller, p_command.cause);
		if (!reason.has_value() || !unit->isActiveCombatant())
		{
			return RejectedCommand{CommandRejection::CommandUnavailable};
		}
		if (!context.hasOrdinaryCapacity(2, true))
		{
			return _abort(BattleAbortReason::InternalInvariantViolation);
		}

		const BattleSnapshot before = context.snapshot();
		std::vector<StagedEvent> staged;
		if (const std::optional<BoardCell> cell = context.board().occupancy().cellOf(p_command.unit); cell.has_value())
		{
			StatusHookService::dispatchObjectTrigger(context, staged, BattleObjectTrigger::UnitActivationEndedOnCell, p_command.unit, *cell);
		}
		StatusHookService::dispatchStatusHook(context, staged, p_command.unit, StatusHook::ActivationEnd);
		context.expireCapturedOwnerActivationDurations(p_command.unit, staged);
		ActivationEnded ended;
		ended.unit = p_command.unit;
		ended.turn = *context.turn();
		ended.finalCell = context.board().occupancy().cellOf(p_command.unit);
		ended.finalActionPoints = unit->actionPoints();
		ended.finalMovementPoints = unit->movementPoints();
		ended.closestAllyDistance = closestDistance(context, p_command.unit, true);
		ended.closestEnemyDistance = closestDistance(context, p_command.unit, false);
		ended.reason = *reason;
		context.unitMutable(p_command.unit).setTurnBarFill(BattleTime{});
		context.setActiveUnit(std::nullopt);
		context.setTurn(std::nullopt);
		context.setResolvedNonEndCommands(0);
		const BattleOutcome outcome = evaluateBattleOutcome(context);
		staged.push_back(StagedEvent{BattleEventCategory::Lifecycle, BattleEventOrigin{.sourceUnit = p_command.unit}, ended});
		if (outcome == BattleOutcome::Undecided)
		{
			context.setPhase(BattlePhase::AwaitingActivation);
		}
		else
		{
			context.setTerminal(outcome);
			BattleEnded battleEnded;
			battleEnded.outcome = outcome;
			battleEnded.activePlayerCount = context.activeCount(BattleSide::Player);
			battleEnded.activeEnemyCount = context.activeCount(BattleSide::Enemy);
			battleEnded.notDefeatedPlayerCount = context.notDefeatedCount(BattleSide::Player);
			battleEnded.notDefeatedEnemyCount = context.notDefeatedCount(BattleSide::Enemy);
			staged.push_back(StagedEvent{BattleEventCategory::Lifecycle, {}, battleEnded});
		}
		const CommittedBattleBatch batch = context.commitOrdinaryBatch(BattleBatchKind::Command, before, staged);
		_assertAcceptedInvariants();
		return AcceptedCommand{*batch.action, batch.id, batch.events};
	}

	SchedulerCallResult BattleSession::advanceUntilActivation()
	{
		if (_resolving)
		{
			return RejectedSchedulerAdvance{SchedulerRejection::SessionBusy};
		}
		if (_context->phase() != BattlePhase::AwaitingActivation)
		{
			return RejectedSchedulerAdvance{SchedulerRejection::WrongPhase};
		}
		ResolutionGuard guard(_resolving);
		return StaminaScheduler::advanceUntilActivation(*_context);
	}

	CommandResult BattleSession::_abort(BattleAbortReason p_reason)
	{
		const BattleSnapshot before = _context->snapshot();
		const CommittedBattleBatch batch = _context->commitAbortBatch(p_reason, before);
		return AbortedCommand{p_reason, batch.id, batch.events};
	}

	void BattleSession::_assertAcceptedInvariants() const
	{
		const BattleContext &context = *_context;
		if (context.playerOrder().size() + context.enemyOrder().size() != context.snapshot().units.size())
		{
			throw std::logic_error("battle invariant: ordered side lists and unit storage disagree");
		}
		if (!context.board().occupancy().invariantsHold())
		{
			throw std::logic_error("battle invariant: board occupancy maps are inconsistent");
		}
		for (const BattleUnitId id : context.allUnitsInOrder())
		{
			const BattleUnit &unit = context.unit(id);
			const bool onBoard = context.board().occupancy().cellOf(id).has_value();
			if (onBoard != unit.placed())
			{
				throw std::logic_error("battle invariant: placed flag disagrees with occupancy");
			}
		}
		const BattleSnapshot snapshot = context.snapshot();
		requireSnapshotInvariants(snapshot);
	}

	BattleSnapshot BattleSession::snapshot() const
	{
		return _context->snapshot();
	}

	std::vector<BattleEvent> BattleSession::copyEvents(EventRange p_range) const
	{
		return _context->events().copy(p_range);
	}

	std::vector<CommittedBattleBatch> BattleSession::takeCommittedBatches()
	{
		return _context->takeCommittedBatches();
	}

	const std::vector<CommittedBattleBatch> &BattleSession::archivedBatches() const noexcept
	{
		return _context->archivedBatches();
	}

	std::uint64_t BattleSession::gameplayProgressDigest() const
	{
		return pg::gameplayProgressDigest(*_context);
	}

	std::uint64_t BattleSession::authoritativeBattleStateDigest() const
	{
		return pg::authoritativeBattleStateDigest(*_context);
	}

	BattleId BattleSession::battleId() const noexcept
	{
		return _context->battleId();
	}

	BattlePhase BattleSession::phase() const noexcept
	{
		return _context->phase();
	}

	BattleOutcome BattleSession::outcome() const noexcept
	{
		return _context->outcome();
	}

	const std::optional<BattleTerminalRecord> &BattleSession::terminalRecord() const noexcept
	{
		return _context->terminalRecord();
	}

	const Registries &BattleSession::registries() const noexcept
	{
		return _context->registries();
	}

	const BoardData &BattleSession::board() const noexcept
	{
		return _context->board();
	}

	std::expected<PlaceUnitCommand, CommandRejection> BattleSession::planPlacement(
		BattleUnitId p_unit,
		BoardCell p_destination) const
	{
		const BattleContext &context = *_context;
		if (context.phase() != BattlePhase::Deployment)
		{
			return std::unexpected(CommandRejection::WrongPhase);
		}
		const BattleUnit *unit = context.tryUnit(p_unit);
		if (unit == nullptr)
		{
			return std::unexpected(CommandRejection::UnknownUnit);
		}
		if (unit->side() != BattleSide::Player)
		{
			return std::unexpected(CommandRejection::WrongController);
		}
		if (unit->removalReason() != RemovalReason::None)
		{
			return std::unexpected(CommandRejection::UnitRemoved);
		}
		if (context.sideConfirmed(unit->side()))
		{
			return std::unexpected(CommandRejection::UnitAlreadyConfirmed);
		}
		const std::optional<BoardCell> current = context.board().occupancy().cellOf(p_unit);
		if (current.has_value() && *current == p_destination)
		{
			return std::unexpected(CommandRejection::NoStateChange);
		}
		if (!context.board().isStandable(p_destination))
		{
			return std::unexpected(CommandRejection::DestinationNotStandable);
		}
		if (!context.board().deployment().contains(unit->side(), p_destination))
		{
			return std::unexpected(CommandRejection::UnitOutsideDeploymentZone);
		}
		if (const std::optional<BattleUnitId> occupant = context.board().occupancy().unitAt(p_destination))
		{
			const BattleUnit &other = context.unit(*occupant);
			if (other.side() != unit->side() || other.removalReason() != RemovalReason::None || !current.has_value())
			{
				return std::unexpected(CommandRejection::DestinationOccupied);
			}
		}
		if (!context.board().terrainIsCurrent())
		{
			return std::unexpected(CommandRejection::CommandUnavailable);
		}
		return PlaceUnitCommand{p_unit, p_destination};
	}

	std::expected<MovePlan, CommandRejection> BattleSession::planMove(BattleUnitId p_unit, BoardCell p_destination) const
	{
		return BattleQueryService(*_context).planMove(p_unit, p_destination);
	}

	std::expected<CastPlan, CommandRejection> BattleSession::planCast(BattleUnitId p_unit, std::string_view p_abilityId, BoardCell p_anchor) const
	{
		return BattleQueryService(*_context).planCast(p_unit, p_abilityId, p_anchor);
	}

	std::vector<MovePlan> BattleSession::legalMoves(BattleUnitId p_unit) const
	{
		return BattleQueryService(*_context).legalMoves(p_unit);
	}
	std::vector<AbilityAnchorPreview> BattleSession::abilityAnchors(BattleUnitId p_unit, std::string_view p_abilityId) const
	{
		return BattleQueryService(*_context).abilityAnchors(p_unit, p_abilityId);
	}

	void BattleSession::primeSequenceCountersForExhaustionTest(
		std::uint64_t p_nextAction,
		std::uint64_t p_nextBatch,
		std::uint64_t p_nextEvent)
	{
		_context->debugSetNextCounters(p_nextAction, p_nextBatch, p_nextEvent);
	}

	void BattleSession::primeShieldIdsForExhaustionTest(std::uint32_t p_next)
	{
		_context->debugSetNextShieldIdForExhaustionTest(p_next);
	}
}
