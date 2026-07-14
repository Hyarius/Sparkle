#pragma once

#include "core/creature_instance_id.hpp"
#include "core/game_rules.hpp"
#include "creatures/creature_state_derivation.hpp"
#include "creatures/creature_unit.hpp"

#include <array>
#include <cstddef>
#include <optional>
#include <vector>

namespace pg
{
	class Registries;

	// Six active slots and an ordered PC box, and the smallest set of operations that keeps both
	// consistent. Every operation names a creature by its instance id - never by species, never by
	// a pointer, never by a position that a previous operation may already have shifted - and every
	// operation is all-or-none: it performs every check that can fail before it touches a slot, so
	// a rejected move leaves the team and the storage order exactly as they were.
	class PlayerRoster
	{
	public:
		static constexpr std::size_t TeamCapacity = 6;
		static_assert(
			TeamCapacity == BattleGameRules::RequiredTeamCapacity,
			"the six slots are a serialized invariant, not a tunable: game-rules.json must agree with them");

		// An empty slot is an ordinary nullopt, not a dummy creature: nothing downstream has to
		// ask whether a creature is "the placeholder one".
		using Team = std::array<std::optional<CreatureUnit>, TeamCapacity>;

		// Where a creature ended up. Returned by value: a pointer into _storage would dangle at the
		// next insertion, and an index into it is only meaningful until the next removal.
		struct Placement
		{
			bool inTeam = false;
			std::size_t index = 0;

			[[nodiscard]] bool operator==(const Placement &p_other) const noexcept = default;
		};

	private:
		Team _team;
		std::vector<CreatureUnit> _storage;

		[[nodiscard]] std::optional<std::size_t> _teamSlotOf(CreatureInstanceId p_id) const noexcept;
		[[nodiscard]] std::optional<std::size_t> _storageIndexOf(CreatureInstanceId p_id) const noexcept;

	public:
		[[nodiscard]] const Team &team() const noexcept;
		[[nodiscard]] const std::vector<CreatureUnit> &storage() const noexcept;

		[[nodiscard]] const CreatureUnit *find(CreatureInstanceId p_id) const noexcept;
		[[nodiscard]] CreatureUnit *findMutable(CreatureInstanceId p_id) noexcept;
		[[nodiscard]] bool contains(CreatureInstanceId p_id) const noexcept;
		// Team members plus stored creatures.
		[[nodiscard]] std::size_t size() const noexcept;

		// Lowest empty team slot, otherwise the end of the storage box.
		Placement add(CreatureUnit p_unit);

		Placement moveTeamToStorage(CreatureInstanceId p_id);
		Placement moveStorageToTeam(CreatureInstanceId p_id, std::size_t p_slot);
		void swapTeamSlots(std::size_t p_first, std::size_t p_second);
		void swapTeamAndStorage(CreatureInstanceId p_teamMember, CreatureInstanceId p_stored);

		// Administrative and test use: the game itself releases a creature through step 18's
		// result commit, not through here. Returns the removed value.
		[[nodiscard]] CreatureUnit remove(CreatureInstanceId p_id);

		// Ids valid and unique across team and storage, species and board known, and every derived
		// cache equal to a fresh derivation - which is what proves the cache is a cache.
		void validate(const DerivationContext &p_context) const;
		void validate(const Registries &p_registries) const;

		[[nodiscard]] bool operator==(const PlayerRoster &p_other) const = default;
	};
}
