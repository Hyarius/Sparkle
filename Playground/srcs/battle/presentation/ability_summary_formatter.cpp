#include "battle/presentation/ability_summary_formatter.hpp"

#include <sstream>
#include <type_traits>

namespace pg
{
	std::string formatAbilityCost(const AbilityDefinition &p_ability)
	{
		std::string result;
		if (p_ability.cost.actionPoints != 0)
		{
			result += std::to_string(p_ability.cost.actionPoints) + " AP";
		}
		if (p_ability.cost.movementPoints != 0)
		{
			if (!result.empty())
			{
				result += " ";
			}
			result += std::to_string(p_ability.cost.movementPoints) + " MP";
		}
		return result.empty() ? "Free" : result;
	}

	std::string formatAbilityRange(const AbilityDefinition &p_ability)
	{
		std::string result = "Range " + std::to_string(p_ability.range.minimum) + "-" + std::to_string(p_ability.range.maximum);
		if (p_ability.area.radius > 0)
		{
			result += " | radius " + std::to_string(p_ability.area.radius);
		}
		if (p_ability.range.requiresLineOfSight)
		{
			result += " | LoS";
		}
		return result;
	}

	std::string formatAbilitySummary(const AbilityDefinition &p_ability)
	{
		std::ostringstream result;
		result << p_ability.displayNameKey << " - " << formatAbilityCost(p_ability) << '\n'
			   << formatAbilityRange(p_ability);
		if (!p_ability.effects.empty())
		{
			result << '\n';
			bool first = true;
			for (const EffectApplication &effect : p_ability.effects)
			{
				if (!first)
				{
					result << "; ";
				}
				first = false;
				std::visit([&result](const auto &payload) {
					using Payload = std::decay_t<decltype(payload)>;
					if constexpr (std::is_same_v<Payload, DamageEffectSpec>)
					{
						result << (payload.kind == DamageKind::Physical ? "Physical damage" : "Magic damage");
					}
					else if constexpr (std::is_same_v<Payload, HealEffectSpec>)
					{
						result << "Heal";
					}
					else if constexpr (std::is_same_v<Payload, ApplyShieldEffectSpec>)
					{
						result << "Shield";
					}
					else if constexpr (std::is_same_v<Payload, ApplyStatusEffectSpec>)
					{
						result << "Apply " << payload.status;
					}
					else if constexpr (std::is_same_v<Payload, RemoveStatusEffectSpec>)
					{
						result << "Remove " << payload.status;
					}
					else if constexpr (std::is_same_v<Payload, ChangeResourceEffectSpec>)
					{
						result << "Change resource";
					}
					else if constexpr (std::is_same_v<Payload, DisplaceEffectSpec>)
					{
						result << "Displace";
					}
					else if constexpr (std::is_same_v<Payload, PlaceObjectEffectSpec>)
					{
						result << "Place " << payload.object;
					}
					else
					{
						result << "Effect";
					}
				},
						   effect.payload);
			}
		}
		return result.str();
	}
}
