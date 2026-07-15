#pragma once

#include "battle/battle_command.hpp" // ActivationEndReason
#include "battle/battle_ids.hpp"
#include "battle/battle_phase.hpp"
#include "battle/battle_sequence_ids.hpp"
#include "battle/battle_time.hpp"
#include "battle/battle_types.hpp"
#include "battle/battle_unit.hpp" // RemovalReason
#include "battle/effects/duration_state.hpp"
#include "board/board_cell.hpp" // BoardCell, BoardSourceDescriptor

#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace pg
{
	// One append-only event vocabulary, declared in full now. Later steps fill the runtime payloads
	// that step 06 does not yet emit; they must never grow a parallel callback-specific event class.
	// Every field is a copied value: no BattleUnit*, no AbilityDefinition*, no component/entity/
	// widget, no reference_wrapper, no span into a temporary command plan, and no callback. An event
	// copied out of the log therefore stays valid after its source temporaries die.

	enum class BattleEventCategory
	{
		Gameplay,
		Lifecycle,
		Taming,
		Diagnostic
	};

	// The closed per-transition cause of a spatial move (step 08+ fills these in).
	enum class SpatialMoveCause
	{
		Voluntary,
		Displacement,
		Teleport,
		Swap
	};

	// Presentation direction of a displacement, distinct from the effect's authored push/pull.
	enum class DisplacementDirection
	{
		Toward,
		Away
	};

	enum class ResourceSpendReason
	{
		AbilityCost,
		MovementCost
	};

	enum class ShieldRemovalReason
	{
		Depleted,
		TimelineExpired,
		OwnerActivationsExpired,
		OwnerRemoved
	};

	enum class StatusRemovalReason
	{
		ExplicitEffect,
		Cleanse,
		TimelineExpired,
		OwnerActivationsExpired,
		UnitRemoved
	};

	enum class BattleObjectRemovalReason
	{
		TimelineExpired,
		OwnerActivationsExpired,
		TriggerExhausted,
		OwnerRemoved,
		ExplicitEffect
	};

	// Which units/ability/effect/status/object a transition is about. Every field optional so one
	// origin shape serves every payload; a deployment event carries only sourceUnit, a damage event
	// carries source, target, ability, effect.
	struct BattleEventOrigin
	{
		std::optional<BattleUnitId> sourceUnit;
		std::optional<BattleUnitId> targetUnit;
		std::optional<std::string> abilityId;
		std::optional<std::string> effectId;
		std::optional<std::string> statusId;
		std::optional<BattleObjectId> objectId;

		[[nodiscard]] bool operator==(const BattleEventOrigin &) const = default;
	};

	struct BattleEventHeader
	{
		BattleId battleId;
		BattleEventSequence sequence;
		BattleBatchId batchId;
		BattleTime battleTime;
		std::optional<TurnIndex> turn;
		std::optional<BattleActionId> action;
		BattleEventCategory category = BattleEventCategory::Lifecycle;
		BattleEventOrigin origin;

		[[nodiscard]] bool operator==(const BattleEventHeader &) const = default;
	};

	// ---- Lifecycle / deployment ------------------------------------------------------------------

	struct BattleStarted
	{
		BattleId battleId;
		std::string stableEncounterIdentity;
		std::string encounterDefinitionId;
		std::string encounterDisplayName;
		EncounterKind encounterKind = EncounterKind::Wild;
		BoardSourceDescriptor boardSource;
		std::vector<BattleUnitId> participants; // Player roster order then Enemy roster order

		[[nodiscard]] bool operator==(const BattleStarted &) const = default;
	};

	struct UnitDeploymentChanged
	{
		BattleUnitId unit;
		std::optional<BoardCell> previousCell;
		BoardCell newCell;
		std::optional<BattleUnitId> swapPartner;

		[[nodiscard]] bool operator==(const UnitDeploymentChanged &) const = default;
	};

	struct DeploymentConfirmed
	{
		BattleSide side = BattleSide::Player;
		std::vector<BattleUnitId> placedUnits; // roster order

		[[nodiscard]] bool operator==(const DeploymentConfirmed &) const = default;
	};

	struct DeploymentCompleted
	{
		std::vector<BattleUnitId> playerUnits;
		std::vector<BattleUnitId> enemyUnits;

		[[nodiscard]] bool operator==(const DeploymentCompleted &) const = default;
	};

	// ---- Timeline / activation (step 07) ---------------------------------------------------------

	struct BattleTimeAdvanced
	{
		BattleTime previous;
		BattleTime delta;
		BattleTime current;

		[[nodiscard]] bool operator==(const BattleTimeAdvanced &) const = default;
	};

	struct ActivationStarted
	{
		BattleUnitId unit;
		TurnIndex turn;
		int actionPoints = 0;
		int movementPoints = 0;
		std::optional<int> closestAllyDistance;
		std::optional<int> closestEnemyDistance;

		[[nodiscard]] bool operator==(const ActivationStarted &) const = default;
	};

	struct ActivationEnded
	{
		BattleUnitId unit;
		TurnIndex turn;
		std::optional<BoardCell> finalCell;
		std::optional<int> finalActionPoints;
		std::optional<int> finalMovementPoints;
		std::optional<int> closestAllyDistance;
		std::optional<int> closestEnemyDistance;
		ActivationEndReason reason = ActivationEndReason::Explicit;

		[[nodiscard]] bool operator==(const ActivationEnded &) const = default;
	};

	// ---- Resources (step 07-09) ------------------------------------------------------------------

	struct ResourceSpent
	{
		BattleUnitId unit;
		BattleResource resource = BattleResource::ActionPoints;
		int requested = 0;
		int applied = 0;
		int before = 0;
		int after = 0;
		ResourceSpendReason reason = ResourceSpendReason::AbilityCost;

		[[nodiscard]] bool operator==(const ResourceSpent &) const = default;
	};

	struct ResourceChanged
	{
		BattleUnitId unit;
		BattleResource resource = BattleResource::ActionPoints;
		int requestedDelta = 0;
		int appliedDelta = 0;
		int before = 0;
		int after = 0;
		ResourceChangeReason reason = ResourceChangeReason::Effect;

		[[nodiscard]] bool operator==(const ResourceChanged &) const = default;
	};

	struct NextActivationPenaltyApplied
	{
		BattleUnitId unit;
		BattleResource resource = BattleResource::ActionPoints;
		int requested = 0;
		int applied = 0;
		int accumulatedBefore = 0;
		int accumulatedAfter = 0;

		[[nodiscard]] bool operator==(const NextActivationPenaltyApplied &) const = default;
	};

	struct TurnBarAdjusted
	{
		BattleUnitId unit;
		BattleTime requestedDelta;
		BattleTime appliedDelta;
		BattleTime before;
		BattleTime after;
		[[nodiscard]] bool operator==(const TurnBarAdjusted &) const = default;
	};

	enum class EffectSkipReason
	{
		MissingScopeValue,
		SourceNotLiving,
		TargetNotLiving,
		TargetNoLongerPlaced,
		DestinationInvalid,
		DestinationBlocked,
		NoDirectionalAxis,
		NoAppliedChange
	};
	struct EffectApplicationSkipped
	{
		std::string effectId;
		std::optional<BattleUnitId> targetUnit;
		std::optional<BoardCell> targetCell;
		EffectSkipReason reason = EffectSkipReason::NoAppliedChange;
		[[nodiscard]] bool operator==(const EffectApplicationSkipped &) const = default;
	};

	// ---- Movement (step 08) ----------------------------------------------------------------------

	struct UnitMovementStep
	{
		BattleUnitId unit;
		BoardCell from;
		BoardCell to;
		int enteredCellCost = 0;
		SpatialMoveCause cause = SpatialMoveCause::Voluntary;

		[[nodiscard]] bool operator==(const UnitMovementStep &) const = default;
	};

	struct UnitMoved
	{
		BattleUnitId unit;
		BoardCell startCell;
		BoardCell endCell;
		int enteredCellCount = 0;
		int totalMovementSpent = 0;
		bool voluntary = true;

		[[nodiscard]] bool operator==(const UnitMoved &) const = default;
	};

	struct UnitDisplaced
	{
		std::optional<BattleUnitId> source;
		BattleUnitId target;
		DisplacementDirection direction = DisplacementDirection::Away;
		BoardCell originalCell;
		BoardCell finalCell;
		// One of (+/-1, 0) or (0, +/-1); (0, 0) records the no-axis application.
		int lockedDirectionX = 0;
		int lockedDirectionZ = 0;
		int requestedDistance = 0;
		int appliedDistance = 0;

		[[nodiscard]] bool operator==(const UnitDisplaced &) const = default;
	};

	struct UnitTeleported
	{
		BattleUnitId unit;
		BoardCell from;
		BoardCell to;
		int manhattanDistance = 0;

		[[nodiscard]] bool operator==(const UnitTeleported &) const = default;
	};

	struct UnitsSwapped
	{
		BattleUnitId first;
		BattleUnitId second;
		BoardCell firstBefore;
		BoardCell firstAfter;
		BoardCell secondBefore;
		BoardCell secondAfter;

		[[nodiscard]] bool operator==(const UnitsSwapped &) const = default;
	};

	// ---- Ability / combat (step 08-10) -----------------------------------------------------------

	struct AbilityCast
	{
		BattleUnitId source;
		std::string abilityId;
		BoardCell sourceCell;
		BoardCell anchorCell;
		int targetDistance = 0; // source-to-anchor X/Z Manhattan
		std::vector<BoardCell> affectedCells;
		std::vector<BattleUnitId> affectedUnits;

		[[nodiscard]] bool operator==(const AbilityCast &) const = default;
	};

	struct Damage
	{
		std::optional<BattleUnitId> source;
		BattleUnitId target;
		std::optional<std::string> abilityId;
		std::optional<std::string> effectId;
		DamageKind kind = DamageKind::Physical;
		int computedDamage = 0;
		int appliedToShield = 0;
		int appliedToHealth = 0;
		bool targetHadAnyShield = false;
		bool targetHadMatchingShield = false;
		int healthBefore = 0;
		int healthAfter = 0;

		[[nodiscard]] bool operator==(const Damage &) const = default;
	};

	struct Healing
	{
		std::optional<BattleUnitId> source;
		BattleUnitId target;
		std::optional<std::string> abilityId;
		std::optional<std::string> effectId;
		int requestedHealing = 0;
		int appliedHealing = 0;
		int healthBefore = 0;
		int healthAfter = 0;

		[[nodiscard]] bool operator==(const Healing &) const = default;
	};

	struct ShieldApplied
	{
		std::optional<BattleUnitId> source;
		BattleUnitId target;
		BattleShieldId shieldId;
		DamageKind kind = DamageKind::Physical;
		int requestedAmount = 0;
		int appliedAmount = 0;
		DurationSnapshot duration;
		std::optional<std::string> sourceAbilityId;
		std::optional<std::string> sourceEffectId;

		[[nodiscard]] bool operator==(const ShieldApplied &) const = default;
	};

	struct ShieldAbsorbed
	{
		std::optional<BattleUnitId> source;
		BattleUnitId target;
		BattleShieldId shieldId;
		DamageKind kind = DamageKind::Physical;
		int requestedAmount = 0;
		int appliedAmount = 0;
		std::optional<BattleUnitId> shieldAppliedBy;
		std::optional<std::string> shieldSourceAbilityId;
		std::optional<std::string> shieldSourceEffectId;

		[[nodiscard]] bool operator==(const ShieldAbsorbed &) const = default;
	};

	struct ShieldBroken
	{
		std::optional<BattleUnitId> source;
		BattleUnitId target;
		BattleShieldId shieldId;
		DamageKind kind = DamageKind::Physical;
		std::optional<BattleUnitId> shieldAppliedBy;
		std::optional<std::string> shieldSourceAbilityId;
		std::optional<std::string> shieldSourceEffectId;

		[[nodiscard]] bool operator==(const ShieldBroken &) const = default;
	};

	struct ShieldRemoved
	{
		BattleUnitId target;
		BattleShieldId shieldId;
		DamageKind kind = DamageKind::Physical;
		ShieldRemovalReason reason = ShieldRemovalReason::TimelineExpired;
		int remainingAmount = 0;
		std::optional<BattleUnitId> shieldAppliedBy;
		std::optional<std::string> shieldSourceAbilityId;
		std::optional<std::string> shieldSourceEffectId;

		[[nodiscard]] bool operator==(const ShieldRemoved &) const = default;
	};

	// ---- Status (step 10) ------------------------------------------------------------------------

	struct StatusApplied
	{
		std::optional<BattleUnitId> source;
		BattleUnitId target;
		std::string statusId;
		int requestedStacks = 0;
		int appliedStacks = 0;
		int resultingStacks = 0;
		BattleTime resultingDuration;
		std::vector<std::string> tags;
		BattleStatusInstanceId instanceId;
		DurationSnapshot duration;
		BattleStatusOrigin origin = BattleStatusOrigin::TransientEffect;

		[[nodiscard]] bool operator==(const StatusApplied &) const = default;
	};

	struct StatusRemoved
	{
		std::optional<BattleUnitId> source;			 // immediate cleanser / removing-effect
		std::optional<BattleUnitId> originalApplier; // separate field, may differ from source
		BattleUnitId target;
		std::string statusId;
		int requestedStacks = 0;
		int removedStacks = 0;
		int remainingStacks = 0;
		StatusRemovalReason reason = StatusRemovalReason::TimelineExpired;
		std::vector<std::string> tags;
		BattleStatusInstanceId instanceId;

		[[nodiscard]] bool operator==(const StatusRemoved &) const = default;
	};

	struct EffectiveStatChanged
	{
		BattleUnitId unit;
		CreatureStat stat = CreatureStat::MaxHealth;
		std::int64_t before = 0;
		std::int64_t after = 0;

		[[nodiscard]] bool operator==(const EffectiveStatChanged &) const = default;
	};

	// ---- Battle objects (step 10) ----------------------------------------------------------------

	struct BattleObjectPlaced
	{
		BattleObjectId object;
		std::string definitionId;
		std::optional<BattleUnitId> creator;
		BoardCell cell;

		[[nodiscard]] bool operator==(const BattleObjectPlaced &) const = default;
	};

	struct BattleObjectRemoved
	{
		BattleObjectId object;
		std::string definitionId;
		BoardCell cell;
		BattleObjectRemovalReason reason = BattleObjectRemovalReason::ExplicitEffect;

		[[nodiscard]] bool operator==(const BattleObjectRemoved &) const = default;
	};

	struct BattleObjectTriggered
	{
		BattleObjectId object;
		std::string definitionId;
		std::optional<BattleUnitId> triggerUnit;
		BoardCell cell;
		std::string triggerId;
		int triggerCount = 0;

		[[nodiscard]] bool operator==(const BattleObjectTriggered &) const = default;
	};

	// ---- Removal / defeat / end (step 06 emits the terminal ones) --------------------------------

	struct UnitRemoved
	{
		BattleUnitId unit;
		std::optional<BoardCell> previousCell;
		RemovalReason reason = RemovalReason::None;

		[[nodiscard]] bool operator==(const UnitRemoved &) const = default;
	};

	struct UnitDefeated
	{
		BattleUnitId unit;
		std::optional<BattleUnitId> killer;
		std::optional<std::string> abilityId;
		std::optional<std::string> effectId;
		BoardCell previousCell;

		[[nodiscard]] bool operator==(const UnitDefeated &) const = default;
	};

	struct BattleEnded
	{
		BattleOutcome outcome = BattleOutcome::Undecided;
		int activePlayerCount = 0;
		int activeEnemyCount = 0;
		int notDefeatedPlayerCount = 0;
		int notDefeatedEnemyCount = 0;

		[[nodiscard]] bool operator==(const BattleEnded &) const = default;
	};

	struct BattleAborted
	{
		BattleAbortReason reason = BattleAbortReason::InternalInvariantViolation;

		[[nodiscard]] bool operator==(const BattleAborted &) const = default;
	};

	using BattleEventPayload = std::variant<
		BattleStarted,
		UnitDeploymentChanged,
		DeploymentConfirmed,
		DeploymentCompleted,
		BattleTimeAdvanced,
		ActivationStarted,
		ActivationEnded,
		ResourceSpent,
		ResourceChanged,
		NextActivationPenaltyApplied,
		TurnBarAdjusted,
		UnitMovementStep,
		UnitMoved,
		UnitDisplaced,
		UnitTeleported,
		UnitsSwapped,
		AbilityCast,
		Damage,
		Healing,
		ShieldApplied,
		ShieldAbsorbed,
		ShieldBroken,
		ShieldRemoved,
		StatusApplied,
		StatusRemoved,
		EffectiveStatChanged,
		BattleObjectPlaced,
		BattleObjectRemoved,
		BattleObjectTriggered,
		EffectApplicationSkipped,
		UnitRemoved,
		UnitDefeated,
		BattleEnded,
		BattleAborted>;

	// The sole header-plus-payload wrapper; there is no per-event subclass.
	struct BattleEvent
	{
		BattleEventHeader header;
		BattleEventPayload payload;

		[[nodiscard]] bool operator==(const BattleEvent &) const = default;
	};

	// A stable short name for the payload alternative, for golden traces and diagnostics. Never
	// parsed back into a type.
	[[nodiscard]] std::string_view battleEventName(const BattleEventPayload &p_payload) noexcept;
	[[nodiscard]] std::string_view toString(BattleEventCategory p_category) noexcept;
}
