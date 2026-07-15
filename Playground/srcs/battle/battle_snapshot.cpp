#include "battle/battle_snapshot.hpp"

#include <set>
#include <stdexcept>
#include <string>

namespace pg
{
	void requireSnapshotInvariants(const BattleSnapshot &p_snapshot)
	{
		const auto fail = [](const std::string &p_why) {
			throw std::logic_error("battle snapshot invariant: " + p_why);
		};

		// Abort reason is present exactly when the outcome is Aborted.
		if ((p_snapshot.outcome == BattleOutcome::Aborted) != p_snapshot.abortReason.has_value())
		{
			fail("abortReason is present exactly when outcome == Aborted");
		}

		std::set<std::uint32_t> seenIds;
		for (const BattleUnitSnapshot &unit : p_snapshot.units)
		{
			if (!unit.id.valid())
			{
				fail("a unit snapshot carries the invalid zero id");
			}
			if (!seenIds.insert(unit.id.value()).second)
			{
				fail("duplicate unit id " + std::to_string(unit.id.value()));
			}

			// cell present exactly while placed.
			if (unit.placed != unit.cell.has_value())
			{
				fail("cell is present exactly while placed for unit " + std::to_string(unit.id.value()));
			}
			// lastOccupiedCell equals cell while placed.
			if (unit.placed && unit.lastOccupiedCell != unit.cell)
			{
				fail("lastOccupiedCell must equal the current cell while placed");
			}

			// Player-versus-enemy provenance nullability.
			if (unit.side == BattleSide::Player)
			{
				if (!unit.persistentCreatureId.has_value())
				{
					fail("a player unit must carry a persistent creature id");
				}
				if (unit.encounterSpawnMemberId.has_value())
				{
					fail("a player unit must not carry an encounter spawn-member id");
				}
			}
			else
			{
				if (unit.persistentCreatureId.has_value())
				{
					fail("an enemy unit must not carry a persistent creature id");
				}
				if (!unit.encounterSpawnMemberId.has_value() || unit.encounterSpawnMemberId->empty())
				{
					fail("an enemy unit must carry a non-empty encounter spawn-member id");
				}
			}
		}
	}
}
