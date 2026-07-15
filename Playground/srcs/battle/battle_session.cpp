#include "battle/battle_session.hpp"

#include "battle/battle_ids.hpp"
#include "battle/battle_state_digest.hpp"
#include "battle/placement/placement_resolver.hpp"
#include "core/registries.hpp"
#include "creatures/creature_attributes.hpp"
#include "creatures/creature_species_definition.hpp"
#include "statuses/status_definition.hpp"

#include <algorithm>
#include <exception>
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
			explicit ResolutionGuard(bool &p_flag) noexcept : _flag(p_flag) { _flag = true; }
			~ResolutionGuard() { _flag = false; }
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

	BattleConstructionResult::BattleConstructionResult(BattleConstructionError p_error) : error(std::move(p_error)) {}

	BattleConstructionResult::BattleConstructionResult(BattleConstructionResult &&) noexcept = default;
	BattleConstructionResult &BattleConstructionResult::operator=(BattleConstructionResult &&) noexcept = default;
	BattleConstructionResult::~BattleConstructionResult() = default;

	// ---- BattleSession --------------------------------------------------------------------------

	BattleSession::BattleSession(std::unique_ptr<BattleContext> p_context) noexcept : _context(std::move(p_context)) {}

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
			}
			catch (const std::exception &)
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
				else
				{
					// Move/Cast/EndTurn have no activation runtime yet; their shapes are final but they
					// resolve to nothing in step 06.
					return RejectedCommand{CommandRejection::CommandUnavailable};
				}
			},
			p_command);
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
		if (!context.hasOrdinaryCapacity(1, true))
		{
			return _abort(BattleAbortReason::InternalInvariantViolation);
		}

		const BattleSnapshot before = context.snapshot();
		context.setSideConfirmed(p_command.side, true);
		// Step 07 appends the phase transition (and DeploymentCompleted) once both sides are confirmed;
		// step 06 leaves the phase in Deployment.
		DeploymentConfirmed confirmed;
		confirmed.side = p_command.side;
		confirmed.placedUnits = context.sideOrder(p_command.side);
		std::vector<StagedEvent> staged{StagedEvent{BattleEventCategory::Lifecycle, BattleEventOrigin{}, confirmed}};

		const CommittedBattleBatch batch = context.commitOrdinaryBatch(BattleBatchKind::Command, before, staged);
		_assertAcceptedInvariants();
		return AcceptedCommand{batch.action.value(), batch.id, batch.events};
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

	void BattleSession::primeSequenceCountersForExhaustionTest(
		std::uint64_t p_nextAction,
		std::uint64_t p_nextBatch,
		std::uint64_t p_nextEvent)
	{
		_context->debugSetNextCounters(p_nextAction, p_nextBatch, p_nextEvent);
	}
}
