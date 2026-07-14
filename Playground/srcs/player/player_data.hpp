#pragma once

#include "core/creature_instance_id.hpp"
#include "encounters/encounter_definition.hpp"
#include "player/player_roster.hpp"

#include "structures/math/spk_vector3.hpp"

#include <cstdint>
#include <set>
#include <span>
#include <string>

namespace pg
{
	class Registries;

	// The persistent player value. It is a value, not a service: nothing subscribes to it, nothing
	// looks it up globally, and a caller that may fail works on a copy and publishes it only on
	// success - which is what makes "a failed operation reserved no id" true by construction rather
	// than by a rollback path that has to be remembered.
	//
	// Step 12 adds the authoritative mode, world seed and world plan; step 18 adds the save.
	struct PlayerData
	{
		PlayerRoster roster;
		// Serials are dealt out in order, so a new game's starters are always creature-...01 to
		// creature-...0N. A recruit is never identified by std::random_device.
		std::uint64_t nextCreatureSerial = 1;
		// Step 12/18 use it to give a repeated encounter a stable identity, and therefore a stable
		// battle seed.
		std::uint64_t encounterSerial = 0;

		// Set from the generated spawn once the scene commits it: the new-game config deliberately
		// hard-codes no world cell.
		spk::Vector3Int playerCell{};
		spk::Vector3Int lastHealPoint{};

		std::set<std::string> clearedTrainerIds;
		std::set<std::string> clearedGymIds;
		std::set<std::string> clearedSpecialEncounterIds;

		// Formats the next serial, checks it is non-zero, unused and not the end of the range, and
		// reserves it by advancing the counter on this value. Throws std::overflow_error or
		// std::invalid_argument without advancing anything.
		[[nodiscard]] CreatureInstanceId allocateCreatureId();

		// The cleared-gym count, clamped to MaximumClearedGymTier. There is no separate badge count
		// that could drift from this one.
		[[nodiscard]] std::uint32_t encounterTier() const noexcept;

		[[nodiscard]] bool operator==(const PlayerData &p_other) const = default;
	};

	// Allocates the id, builds and validates the creature, and only then publishes the working
	// value into p_player. A throw anywhere in between discards the working copy - and with it the
	// reserved serial - so a failed recruit leaves no gap in the id sequence.
	PlayerRoster::Placement createAndAddCreature(
		PlayerData &p_player,
		std::string p_speciesId,
		std::span<const std::string> p_completedNodeIds,
		const DerivationContext &p_context);

	PlayerRoster::Placement createAndAddCreature(
		PlayerData &p_player,
		std::string p_speciesId,
		std::span<const std::string> p_completedNodeIds,
		const Registries &p_registries);
}
