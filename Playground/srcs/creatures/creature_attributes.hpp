#pragma once

#include "battle/battle_time.hpp"
#include "battle/battle_types.hpp"
#include "core/definition_fields.hpp"
#include "core/game_rules.hpp"
#include "core/json.hpp"

#include <cstdint>
#include <string>

namespace pg
{
	// The nine authored numbers a creature fights with, and the only ones. There is no level, no
	// experience, no individual value, no nature and no random roll anywhere in this file: a fresh
	// creature of a species starts on exactly the numbers its species authored, and every later
	// difference between two creatures of the same species is a completed Feat node.
	//
	// Stamina is the turn-bar cost of one activation, so a lower stamina means a faster creature.
	struct CreatureAttributes
	{
		std::int64_t maxHealth = 1;
		std::int64_t strength = 0;
		std::int64_t magicPower = 0;
		std::int64_t armor = 0;
		std::int64_t resistance = 0;
		std::int64_t maxActionPoints = 1;
		std::int64_t maxMovementPoints = 1;
		BattleTime stamina = BattleTime::fromTicks(1000);
		std::int64_t range = 0;

		[[nodiscard]] bool operator==(const CreatureAttributes &p_other) const noexcept = default;
	};

	// The same bound the step-02 additive modifier and the step-03 stat reward already use, so a
	// reward can never grant a value the attribute schema would have rejected.
	inline constexpr std::int64_t MaximumCreatureAttribute = 1000000;

	// Reads the "attributes" object of a species file. p_rules supplies minimumStamina: a creature
	// faster than the rules allow would break the scheduler's tie-breaks, so it fails the load.
	[[nodiscard]] CreatureAttributes parseCreatureAttributes(JsonReader &p_reader, const GameRules &p_rules);

	// Re-checks every bound the parser checked. Reward replay calls it on its result, so a legal
	// board of legal rewards can still not produce an illegal creature. Throws a JsonError at
	// p_source, naming p_owner.
	void requireValidAttributes(
		const CreatureAttributes &p_attributes,
		const GameRules &p_rules,
		const DefinitionSource &p_source,
		const std::string &p_owner);

	// Checked addition in the stat's own unit: p_staminaDelta is used for Stamina and p_amount for
	// every other stat. Throws std::overflow_error rather than wrapping; the caller adds the file
	// and path context. It does not clamp to the attribute bounds - requireValidAttributes() does
	// that once the whole replay is done, so an intermediate sum is free to be out of range.
	void addToAttribute(
		CreatureAttributes &p_attributes,
		CreatureStat p_stat,
		std::int64_t p_amount,
		const BattleTime &p_staminaDelta);
}
