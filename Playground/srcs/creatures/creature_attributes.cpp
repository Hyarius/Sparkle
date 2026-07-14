#include "creatures/creature_attributes.hpp"

#include <limits>
#include <stdexcept>

namespace pg
{
	namespace
	{
		[[nodiscard]] std::int64_t requireAttribute(
			JsonReader &p_reader,
			const std::string &p_key,
			std::int64_t p_minimum)
		{
			return requireIntegerInRange(p_reader, p_key, p_minimum, MaximumCreatureAttribute);
		}

		void requireBound(
			bool p_holds,
			const DefinitionSource &p_source,
			const std::string &p_owner,
			const std::string &p_message)
		{
			if (!p_holds)
			{
				throw JsonError(p_source.file, p_source.jsonPath, "'" + p_owner + "' " + p_message);
			}
		}

		// Guarded before the fact, like BattleTime's: this repository is built with compilers that
		// do not share one overflow builtin.
		[[nodiscard]] std::int64_t addChecked(std::int64_t p_value, std::int64_t p_amount)
		{
			constexpr std::int64_t Minimum = std::numeric_limits<std::int64_t>::min();
			constexpr std::int64_t Maximum = std::numeric_limits<std::int64_t>::max();

			if ((p_amount > 0 && p_value > Maximum - p_amount) || (p_amount < 0 && p_value < Minimum - p_amount))
			{
				throw std::overflow_error("creature attribute addition overflowed");
			}
			return p_value + p_amount;
		}
	}

	CreatureAttributes parseCreatureAttributes(JsonReader &p_reader, const GameRules &p_rules)
	{
		p_reader.forbidUnknown(
			{"maxHealth",
			 "strength",
			 "magicPower",
			 "armor",
			 "resistance",
			 "maxActionPoints",
			 "maxMovementPoints",
			 "staminaSeconds",
			 "range"});

		CreatureAttributes result;
		// A creature with no health, no action point or no movement point could never take its
		// first turn, so these three are positive rather than merely non-negative.
		result.maxHealth = requireAttribute(p_reader, "maxHealth", 1);
		result.strength = requireAttribute(p_reader, "strength", 0);
		result.magicPower = requireAttribute(p_reader, "magicPower", 0);
		result.armor = requireAttribute(p_reader, "armor", 0);
		result.resistance = requireAttribute(p_reader, "resistance", 0);
		result.maxActionPoints = requireAttribute(p_reader, "maxActionPoints", 1);
		result.maxMovementPoints = requireAttribute(p_reader, "maxMovementPoints", 1);
		result.stamina = requireBattleSeconds(p_reader, "staminaSeconds", BattleTimeSign::Positive);
		result.range = requireAttribute(p_reader, "range", 0);

		if (result.stamina < p_rules.battle.minimumStamina)
		{
			throw JsonError(
				p_reader.file(),
				p_reader.pathFor("staminaSeconds"),
				"stamina is at least gameRules.battle.minimumStamina (" +
					std::to_string(p_rules.battle.minimumStamina.ticks()) + " ticks), got " +
					std::to_string(result.stamina.ticks()));
		}
		return result;
	}

	void requireValidAttributes(
		const CreatureAttributes &p_attributes,
		const GameRules &p_rules,
		const DefinitionSource &p_source,
		const std::string &p_owner)
	{
		const auto inRange = [&p_source, &p_owner](std::int64_t p_value, std::int64_t p_minimum, const std::string &p_name) {
			requireBound(
				p_value >= p_minimum && p_value <= MaximumCreatureAttribute,
				p_source,
				p_owner,
				"derives " + p_name + " = " + std::to_string(p_value) + ", outside [" + std::to_string(p_minimum) + ", " +
					std::to_string(MaximumCreatureAttribute) + "]");
		};

		inRange(p_attributes.maxHealth, 1, "maxHealth");
		inRange(p_attributes.strength, 0, "strength");
		inRange(p_attributes.magicPower, 0, "magicPower");
		inRange(p_attributes.armor, 0, "armor");
		inRange(p_attributes.resistance, 0, "resistance");
		inRange(p_attributes.maxActionPoints, 1, "maxActionPoints");
		inRange(p_attributes.maxMovementPoints, 1, "maxMovementPoints");
		inRange(p_attributes.range, 0, "range");

		requireBound(
			p_attributes.stamina >= p_rules.battle.minimumStamina,
			p_source,
			p_owner,
			"derives a stamina of " + std::to_string(p_attributes.stamina.ticks()) + " ticks, below the rules' minimum of " +
				std::to_string(p_rules.battle.minimumStamina.ticks()));
	}

	void addToAttribute(
		CreatureAttributes &p_attributes,
		CreatureStat p_stat,
		std::int64_t p_amount,
		const BattleTime &p_staminaDelta)
	{
		switch (p_stat)
		{
		case CreatureStat::MaxHealth:
			p_attributes.maxHealth = addChecked(p_attributes.maxHealth, p_amount);
			return;
		case CreatureStat::Strength:
			p_attributes.strength = addChecked(p_attributes.strength, p_amount);
			return;
		case CreatureStat::MagicPower:
			p_attributes.magicPower = addChecked(p_attributes.magicPower, p_amount);
			return;
		case CreatureStat::Armor:
			p_attributes.armor = addChecked(p_attributes.armor, p_amount);
			return;
		case CreatureStat::Resistance:
			p_attributes.resistance = addChecked(p_attributes.resistance, p_amount);
			return;
		case CreatureStat::MaxActionPoints:
			p_attributes.maxActionPoints = addChecked(p_attributes.maxActionPoints, p_amount);
			return;
		case CreatureStat::MaxMovementPoints:
			p_attributes.maxMovementPoints = addChecked(p_attributes.maxMovementPoints, p_amount);
			return;
		case CreatureStat::Stamina:
			// BattleTime's own operator+ is the checked one; it throws std::overflow_error too.
			p_attributes.stamina = p_attributes.stamina + p_staminaDelta;
			return;
		case CreatureStat::Range:
			p_attributes.range = addChecked(p_attributes.range, p_amount);
			return;
		}
	}
}
