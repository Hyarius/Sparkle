#include "battle/battle_unit.hpp"

namespace pg
{
	std::string_view toString(RemovalReason p_reason) noexcept
	{
		switch (p_reason)
		{
		case RemovalReason::None:
			return "none";
		case RemovalReason::Defeated:
			return "defeated";
		case RemovalReason::Impressed:
			return "impressed";
		}
		return "none";
	}

	BattleUnit BattleUnit::project(BattleUnitId p_id, const BattleParticipantSeed &p_seed)
	{
		BattleUnit unit;
		unit._id = p_id;
		unit._side = p_seed.side;
		unit._rosterOrder = p_seed.rosterOrder;
		unit._persistentCreatureId = p_seed.persistentCreatureId;
		unit._encounterSpawnMemberId = p_seed.encounterSpawnMemberId;
		unit._speciesId = p_seed.speciesId;
		unit._formId = p_seed.formId;
		unit._aiBehaviourId = p_seed.aiBehaviourId;

		unit._baselineAttributes = p_seed.attributes;
		unit._effectiveAttributes = p_seed.attributes;
		unit._abilityIds = p_seed.abilityIds;
		unit._passiveStatusIds = p_seed.passiveStatusIds;
		unit._inheritedCompletedFeatNodeIds = p_seed.inheritedCompletedFeatNodeIds;

		// Current health begins at effective maximum. AP/MP are authoritatively refilled at
		// activation start in step 07, so they begin at zero here, as do the turn-bar and penalties.
		unit._health = static_cast<int>(p_seed.attributes.maxHealth);
		unit._actionPoints = 0;
		unit._movementPoints = 0;
		unit._nextActivationPenalty = NextActivationPenalty{};
		unit._turnBarFill = BattleTime{};

		unit._placed = false;
		unit._lastOccupiedCell = std::nullopt;
		unit._removalReason = RemovalReason::None;
		return unit;
	}

	bool BattleUnit::isActiveCombatant() const noexcept
	{
		return _placed && _health > 0 && _removalReason == RemovalReason::None;
	}

	void BattleUnit::markPlacedAt(const BoardCell &p_cell)
	{
		_placed = true;
		_lastOccupiedCell = p_cell;
	}

	void BattleUnit::clearPlacement()
	{
		// The support cell is preserved as lastOccupiedCell for events and result presentation; only
		// the live "is on the board" flag drops.
		_placed = false;
	}
}
