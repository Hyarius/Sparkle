#pragma once

#include "abilities/ability_definition.hpp"

#include <string>

namespace pg
{
	// Read-only, deliberately formula-free prototype text for the ability bar/detail panel.
	[[nodiscard]] std::string formatAbilitySummary(const AbilityDefinition &p_ability);
	[[nodiscard]] std::string formatAbilityCost(const AbilityDefinition &p_ability);
	[[nodiscard]] std::string formatAbilityRange(const AbilityDefinition &p_ability);
}
