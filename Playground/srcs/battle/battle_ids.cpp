#include "battle/battle_ids.hpp"

#include "core/deterministic_random.hpp"

namespace pg
{
	BattleId battleIdFromMixedSeed(std::uint64_t p_mixed) noexcept
	{
		// Fold the 64-bit mix into 32 bits, then remap the one invalid result. Remapping is
		// deliberate: a session whose seed folds to zero must still get a stable id, and it
		// simply shares its id with the seeds that fold to one.
		std::uint32_t value =
			static_cast<std::uint32_t>(p_mixed) ^ static_cast<std::uint32_t>(p_mixed >> 32U);
		if (value == 0U)
		{
			value = 1U;
		}
		return BattleId(value);
	}

	BattleId deriveBattleId(std::uint64_t p_combatSeed)
	{
		return battleIdFromMixedSeed(pg::deterministic::deriveSeed(p_combatSeed, "battle-id/v1"));
	}
}
