#pragma once

#include "battle/battle_ids.hpp"
#include "battle/battle_phase.hpp"
#include "battle/battle_sequence_ids.hpp"
#include "battle/battle_time.hpp"
#include "battle/battle_types.hpp"
#include "battle/battle_unit.hpp" // RemovalReason
#include "battle/effects/duration_state.hpp"
#include "board/board_cell.hpp"
#include "core/creature_instance_id.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace pg
{
	struct BattleShieldSnapshot
	{
		BattleShieldId id;
		DamageKind kind = DamageKind::Physical;
		std::int64_t remainingAmount = 0;
		DurationSnapshot duration;
		std::optional<BattleUnitId> source;
		std::optional<std::string> abilityId;
		std::optional<std::string> effectId;
		[[nodiscard]] bool operator==(const BattleShieldSnapshot &) const = default;
	};
	// A deterministic value copy of one unit's observable state. It carries no pointer into the
	// live BattleUnit, so it stays valid after later commands and after the session is destroyed.
	struct BattleUnitSnapshot
	{
		BattleUnitId id;
		BattleSide side = BattleSide::Player;
		std::uint32_t rosterOrder = 0;
		std::optional<CreatureInstanceId> persistentCreatureId;
		std::optional<std::string> encounterSpawnMemberId;
		std::vector<std::string> inheritedCompletedFeatNodeIds;
		std::string speciesId;
		std::string formId;
		int health = 0;
		int maxHealth = 0;
		int actionPoints = 0;
		int maxActionPoints = 0;
		int movementPoints = 0;
		int maxMovementPoints = 0;
		BattleTime stamina;
		BattleTime turnBarFill;
		int range = 0;
		bool placed = false;
		std::optional<BoardCell> cell; // present exactly while placed
		std::optional<BoardCell> lastOccupiedCell;
		RemovalReason removalReason = RemovalReason::None;
		std::vector<BattleShieldSnapshot> shields;

		[[nodiscard]] bool operator==(const BattleUnitSnapshot &) const = default;
	};

	// The whole battle, copied. Units are Player roster order then Enemy roster order. Step 10 adds
	// status/shield/object snapshots.
	struct BattleSnapshot
	{
		BattleId battleId;
		BoardSourceDescriptor boardSource;
		BattlePhase phase = BattlePhase::Deployment;
		BattleOutcome outcome = BattleOutcome::Undecided;
		std::optional<BattleAbortReason> abortReason;
		BattleTime elapsed;
		std::optional<TurnIndex> turn;
		std::uint64_t nextTurn = 1;
		std::optional<BattleUnitId> activeUnit;
		std::size_t resolvedNonEndCommands = 0;
		std::vector<BattleUnitSnapshot> units;

		[[nodiscard]] bool operator==(const BattleSnapshot &) const = default;
	};

	// Validates the copied-state invariants section 15 requires: no duplicate id, cell present
	// exactly while placed, lastOccupiedCell agreement, abort-reason/outcome agreement, and
	// player-versus-enemy provenance nullability. Throws std::logic_error on a violation; tests and
	// debug builds call it after producing a snapshot.
	void requireSnapshotInvariants(const BattleSnapshot &p_snapshot);
}
