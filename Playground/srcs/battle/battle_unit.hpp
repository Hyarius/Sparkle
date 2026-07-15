#pragma once

#include "battle/battle_ids.hpp"
#include "battle/battle_time.hpp"
#include "battle/battle_types.hpp"
#include "board/board_cell.hpp"
#include "core/creature_instance_id.hpp"
#include "creatures/creature_attributes.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace pg
{
	// Why a unit left occupancy. Impressed (step 16 taming) is deliberately NOT a defeat: it must
	// never grant kill credit, and outcome code treats it as the creature walking off with the
	// player, not dying.
	enum class RemovalReason
	{
		None,
		Defeated,
		Impressed
	};

	[[nodiscard]] std::string_view toString(RemovalReason p_reason) noexcept;

	// Points the unit's next activation begins short of full. Step 07 consumes it at refill; step
	// 06 only ever carries the zero value, so nothing here spends it yet.
	struct NextActivationPenalty
	{
		int actionPoints = 0;
		int movementPoints = 0;

		[[nodiscard]] bool operator==(const NextActivationPenalty &) const noexcept = default;
	};

	// One value seed per actual participant, built by the caller from the shared step-04 derivation
	// service (a player's CreatureUnit, an enemy's EncounterSpawnDefinition projected through the
	// same deriveCreatureState). BattleSession never replays Feat rewards itself: it consumes the
	// already-derived attributes/loadout and only checks them.
	struct BattleParticipantSeed
	{
		BattleSide side = BattleSide::Player;
		std::uint32_t rosterOrder = 0;
		// A player carries its stable persistent creature id and no spawn-member id; an enemy is the
		// exact mirror. The projection and every later result snapshot preserve this asymmetry.
		std::optional<CreatureInstanceId> persistentCreatureId;
		std::optional<std::string> encounterSpawnMemberId;
		std::string speciesId;
		std::string formId;
		CreatureAttributes attributes;
		std::vector<std::string> abilityIds;
		std::vector<std::string> passiveStatusIds;
		std::optional<std::string> aiBehaviourId;
		// Copied in authored order from the exact enemy spawn member; empty for a player, whose
		// progress is addressed by stable creature id and snapshotted in step 17.
		std::vector<std::string> inheritedCompletedFeatNodeIds;
	};

	// One homogeneous encounter-local projection. There is deliberately no player/enemy/wild/AI
	// subclass: the difference between two units is entirely their field values, so scheduler,
	// targeting, AI, taming and outcome code all read the same type.
	//
	// It is a value the session owns and mutates through the command path; it is not a component and
	// never carries a rendering handle. No battle-local HP/AP/MP/status/position is ever written
	// back into the persistent CreatureUnit, and no BattleUnit owns or mutates Feat progress.
	class BattleUnit
	{
	private:
		BattleUnitId _id;
		BattleSide _side = BattleSide::Player;
		std::uint32_t _rosterOrder = 0;
		std::optional<CreatureInstanceId> _persistentCreatureId;
		std::optional<std::string> _encounterSpawnMemberId;
		std::string _speciesId;
		std::string _formId;
		std::optional<std::string> _aiBehaviourId;

		CreatureAttributes _baselineAttributes;	 // the derived state at battle entry
		CreatureAttributes _effectiveAttributes; // == baseline until step 10 recomputes from statuses
		std::vector<std::string> _abilityIds;
		std::vector<std::string> _passiveStatusIds;
		std::vector<std::string> _inheritedCompletedFeatNodeIds;

		int _health = 0;
		int _actionPoints = 0;
		int _movementPoints = 0;
		NextActivationPenalty _nextActivationPenalty;
		BattleTime _turnBarFill{};

		bool _placed = false;
		std::optional<BoardCell> _lastOccupiedCell;
		RemovalReason _removalReason = RemovalReason::None;

		// Step 10 adds status/shield instance containers here; step 16 may add a taming tracker.

	public:
		BattleUnit() = default;

		// Projects one seed into a fresh combat unit: current health at effective maximum, AP/MP,
		// turn-bar fill and penalties all zero. No level and no random stat scaling occurs anywhere.
		[[nodiscard]] static BattleUnit project(BattleUnitId p_id, const BattleParticipantSeed &p_seed);

		[[nodiscard]] BattleUnitId id() const noexcept
		{
			return _id;
		}
		[[nodiscard]] BattleSide side() const noexcept
		{
			return _side;
		}
		[[nodiscard]] std::uint32_t rosterOrder() const noexcept
		{
			return _rosterOrder;
		}
		[[nodiscard]] const std::optional<CreatureInstanceId> &persistentCreatureId() const noexcept
		{
			return _persistentCreatureId;
		}
		[[nodiscard]] const std::optional<std::string> &encounterSpawnMemberId() const noexcept
		{
			return _encounterSpawnMemberId;
		}
		[[nodiscard]] const std::string &speciesId() const noexcept
		{
			return _speciesId;
		}
		[[nodiscard]] const std::string &formId() const noexcept
		{
			return _formId;
		}
		[[nodiscard]] const std::optional<std::string> &aiBehaviourId() const noexcept
		{
			return _aiBehaviourId;
		}

		[[nodiscard]] const CreatureAttributes &baselineAttributes() const noexcept
		{
			return _baselineAttributes;
		}
		[[nodiscard]] const CreatureAttributes &effectiveAttributes() const noexcept
		{
			return _effectiveAttributes;
		}
		[[nodiscard]] const std::vector<std::string> &abilityIds() const noexcept
		{
			return _abilityIds;
		}
		[[nodiscard]] const std::vector<std::string> &passiveStatusIds() const noexcept
		{
			return _passiveStatusIds;
		}
		[[nodiscard]] const std::vector<std::string> &inheritedCompletedFeatNodeIds() const noexcept
		{
			return _inheritedCompletedFeatNodeIds;
		}

		[[nodiscard]] int health() const noexcept
		{
			return _health;
		}
		[[nodiscard]] int actionPoints() const noexcept
		{
			return _actionPoints;
		}
		[[nodiscard]] int movementPoints() const noexcept
		{
			return _movementPoints;
		}
		[[nodiscard]] const NextActivationPenalty &nextActivationPenalty() const noexcept
		{
			return _nextActivationPenalty;
		}
		[[nodiscard]] BattleTime turnBarFill() const noexcept
		{
			return _turnBarFill;
		}

		[[nodiscard]] bool placed() const noexcept
		{
			return _placed;
		}
		[[nodiscard]] const std::optional<BoardCell> &lastOccupiedCell() const noexcept
		{
			return _lastOccupiedCell;
		}
		[[nodiscard]] RemovalReason removalReason() const noexcept
		{
			return _removalReason;
		}

		[[nodiscard]] int maxHealth() const noexcept
		{
			return static_cast<int>(_effectiveAttributes.maxHealth);
		}
		[[nodiscard]] int maxActionPoints() const noexcept
		{
			return static_cast<int>(_effectiveAttributes.maxActionPoints);
		}
		[[nodiscard]] int maxMovementPoints() const noexcept
		{
			return static_cast<int>(_effectiveAttributes.maxMovementPoints);
		}
		[[nodiscard]] int range() const noexcept
		{
			return static_cast<int>(_effectiveAttributes.range);
		}

		// The one active-combatant predicate, and the only one every rule uses:
		//   placed and current health > 0 and removalReason == None.
		// An undeployed, defeated or impressed unit is not active.
		[[nodiscard]] bool isActiveCombatant() const noexcept;
		// Step 10 replaces this final-purpose seam with the active-status tag query. There are no
		// runtime statuses yet, so no Step 07 unit is stunned.
		[[nodiscard]] bool isStunned() const noexcept
		{
			return false;
		}

		// Mutation surface. These are reachable only through BattleContext, which is itself a private
		// member of BattleSession, so nothing outside the command path can call them.
		void markPlacedAt(const BoardCell &p_cell);
		void clearPlacement();
		void setRemovalReason(RemovalReason p_reason) noexcept
		{
			_removalReason = p_reason;
		}
		void setTurnBarFill(BattleTime p_fill) noexcept
		{
			_turnBarFill = p_fill;
		}
		void setActionPoints(int p_value) noexcept
		{
			_actionPoints = p_value;
		}
		void setMovementPoints(int p_value) noexcept
		{
			_movementPoints = p_value;
		}
		void setHealth(int p_value) noexcept
		{
			_health = p_value;
		}
		void addNextActivationPenalty(BattleResource p_resource, int p_amount) noexcept
		{
			if (p_resource == BattleResource::ActionPoints)
			{
				_nextActivationPenalty.actionPoints += p_amount;
			}
			else
			{
				_nextActivationPenalty.movementPoints += p_amount;
			}
		}
		void clearNextActivationPenalty() noexcept
		{
			_nextActivationPenalty = {};
		}
	};
}
