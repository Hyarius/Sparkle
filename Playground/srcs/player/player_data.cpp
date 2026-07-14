#include "player/player_data.hpp"

#include "core/registries.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace pg
{
	CreatureInstanceId PlayerData::allocateCreatureId()
	{
		if (nextCreatureSerial == 0)
		{
			// Zero is the invalid serial, and it is also how an exhausted counter parks itself.
			throw std::overflow_error("the creature serial counter is exhausted");
		}

		const CreatureInstanceId id = CreatureInstanceId::fromSerial(nextCreatureSerial);
		if (roster.contains(id))
		{
			throw std::invalid_argument(
				"creature serial " + std::to_string(nextCreatureSerial) + " is already in the roster");
		}

		nextCreatureSerial =
			nextCreatureSerial == std::numeric_limits<std::uint64_t>::max() ? 0 : nextCreatureSerial + 1;
		return id;
	}

	std::uint32_t PlayerData::encounterTier() const noexcept
	{
		return static_cast<std::uint32_t>(std::min<std::size_t>(clearedGymIds.size(), MaximumClearedGymTier));
	}

	PlayerRoster::Placement createAndAddCreature(
		PlayerData &p_player,
		std::string p_speciesId,
		std::span<const std::string> p_completedNodeIds,
		const DerivationContext &p_context)
	{
		// The whole transaction happens on a copy. Discarding it discards the reserved serial too,
		// which is exactly the boundary step 18 commits a battle result across.
		PlayerData working = p_player;

		const CreatureInstanceId id = working.allocateCreatureId();
		CreatureUnit unit = makeCreatureUnit(id, std::move(p_speciesId), p_completedNodeIds, p_context);
		const PlayerRoster::Placement placement = working.roster.add(std::move(unit));

		p_player = std::move(working);
		return placement;
	}

	PlayerRoster::Placement createAndAddCreature(
		PlayerData &p_player,
		std::string p_speciesId,
		std::span<const std::string> p_completedNodeIds,
		const Registries &p_registries)
	{
		return createAndAddCreature(
			p_player,
			std::move(p_speciesId),
			p_completedNodeIds,
			derivationContextOf(p_registries));
	}
}
