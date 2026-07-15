#include "battle/status/effective_stats.hpp"

#include "battle/battle_unit.hpp"
#include "core/registries.hpp"
#include "statuses/status_definition.hpp"

#include <limits>
#include <stdexcept>

namespace pg
{
	namespace
	{
		[[nodiscard]] std::int64_t checkedAdd(std::int64_t p_value, std::int64_t p_delta)
		{
			if ((p_delta > 0 && p_value > std::numeric_limits<std::int64_t>::max() - p_delta) ||
				(p_delta < 0 && p_value < std::numeric_limits<std::int64_t>::min() - p_delta))
			{
				throw std::overflow_error("effective-stat addition overflowed");
			}
			return p_value + p_delta;
		}

		[[nodiscard]] std::int64_t checkedMultiplyPermille(std::int64_t p_value, std::int64_t p_factor)
		{
			// Attribute values are clamped non-negative only after both phases, so use a conservative
			// checked product rather than relying on implementation-defined signed overflow.
			if (p_value != 0 && (p_value > std::numeric_limits<std::int64_t>::max() / p_factor ||
								 p_value < std::numeric_limits<std::int64_t>::min() / p_factor))
			{
				throw std::overflow_error("effective-stat multiplication overflowed");
			}
			return (p_value * p_factor) / 1000;
		}

		[[nodiscard]] std::int64_t &numericAttribute(CreatureAttributes &p_attributes, CreatureStat p_stat)
		{
			switch (p_stat)
			{
			case CreatureStat::MaxHealth:
				return p_attributes.maxHealth;
			case CreatureStat::Strength:
				return p_attributes.strength;
			case CreatureStat::MagicPower:
				return p_attributes.magicPower;
			case CreatureStat::Armor:
				return p_attributes.armor;
			case CreatureStat::Resistance:
				return p_attributes.resistance;
			case CreatureStat::MaxActionPoints:
				return p_attributes.maxActionPoints;
			case CreatureStat::MaxMovementPoints:
				return p_attributes.maxMovementPoints;
			case CreatureStat::Range:
				return p_attributes.range;
			case CreatureStat::Stamina:
				break;
			}
			throw std::logic_error("stamina is represented by BattleTime, not a numeric attribute reference");
		}

		[[nodiscard]] std::int64_t modifierValue(const StatModifierSpec &p_modifier, std::uint32_t p_stacks)
		{
			if (!p_modifier.perStack)
			{
				return p_modifier.value;
			}
			if (p_stacks != 0 && (p_modifier.value > std::numeric_limits<std::int64_t>::max() / static_cast<std::int64_t>(p_stacks) ||
								  p_modifier.value < std::numeric_limits<std::int64_t>::min() / static_cast<std::int64_t>(p_stacks)))
			{
				throw std::overflow_error("per-stack effective-stat modifier overflowed");
			}
			return p_modifier.value * static_cast<std::int64_t>(p_stacks);
		}

		void apply(const StatModifierSpec &p_modifier, std::uint32_t p_stacks, bool p_additive, CreatureAttributes &p_result)
		{
			if ((p_modifier.operation == StatModifierOperation::Add) != p_additive)
			{
				return;
			}
			if (p_modifier.stat == CreatureStat::Stamina)
			{
				std::int64_t value = p_result.stamina.ticks();
				if (p_additive)
				{
					value = checkedAdd(value, modifierValue(p_modifier, p_stacks));
				}
				else
				{
					const std::uint32_t count = p_modifier.perStack ? p_stacks : 1U;
					for (std::uint32_t i = 0; i < count; ++i)
					{
						value = checkedMultiplyPermille(value, p_modifier.value);
					}
				}
				p_result.stamina = BattleTime::fromTicks(value);
				return;
			}

			std::int64_t &attribute = numericAttribute(p_result, p_modifier.stat);
			if (p_additive)
			{
				attribute = checkedAdd(attribute, modifierValue(p_modifier, p_stacks));
			}
			else
			{
				const std::uint32_t count = p_modifier.perStack ? p_stacks : 1U;
				for (std::uint32_t i = 0; i < count; ++i)
				{
					attribute = checkedMultiplyPermille(attribute, p_modifier.value);
				}
			}
		}

		void applyPass(const BattleUnit &p_unit, const Registries &p_registries, bool p_additive, CreatureAttributes &p_result)
		{
			const auto applyInstances = [&](const std::vector<BattleStatusInstance> &p_instances) {
				for (const BattleStatusInstance &instance : p_instances)
				{
					const StatusDefinition *definition = p_registries.statuses().tryGet(instance.definitionId);
					if (definition == nullptr)
					{
						throw std::logic_error("battle status references an unavailable definition '" + instance.definitionId + "'");
					}
					for (const StatModifierSpec &modifier : definition->modifiers)
					{
						apply(modifier, instance.stacks, p_additive, p_result);
					}
				}
			};
			applyInstances(p_unit.passiveStatuses());
			applyInstances(p_unit.transientStatuses());
		}
	}

	CreatureAttributes EffectiveStatsEvaluator::evaluate(const BattleUnit &p_unit, const Registries &p_registries)
	{
		CreatureAttributes result = p_unit.baselineAttributes();
		applyPass(p_unit, p_registries, true, result);
		applyPass(p_unit, p_registries, false, result);

		result.maxHealth = std::max<std::int64_t>(1, result.maxHealth);
		result.maxActionPoints = std::max<std::int64_t>(1, result.maxActionPoints);
		result.maxMovementPoints = std::max<std::int64_t>(1, result.maxMovementPoints);
		result.stamina = std::max(result.stamina, p_registries.gameRules().battle.minimumStamina);
		result.strength = std::max<std::int64_t>(0, result.strength);
		result.magicPower = std::max<std::int64_t>(0, result.magicPower);
		result.armor = std::max<std::int64_t>(0, result.armor);
		result.resistance = std::max<std::int64_t>(0, result.resistance);
		result.range = std::max<std::int64_t>(0, result.range);
		return result;
	}
}
