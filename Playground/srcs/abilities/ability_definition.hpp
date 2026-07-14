#pragma once

#include "abilities/effect_definition.hpp"
#include "abilities/targeting_definition.hpp"
#include "core/json.hpp"
#include "structures/math/spk_vector2.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace pg
{
	struct AbilityCost
	{
		int actionPoints = 0;
		int movementPoints = 0;
	};

	// An immutable authored command. It holds no current AP/MP, no board position and no
	// pointer to another definition: cross-references stay validated string ids, which the
	// runtime resolves through the registries.
	//
	// Zero-cost abilities are legal because a status or a passive may author one; the
	// activation command cap and the no-state-change guard of step 10 are what stop a loop.
	// Parsing cannot prove an effect changes state for every target.
	struct AbilityDefinition
	{
		std::string id;
		// Translation keys, not sentences: the text lives in resources/data/locales and is
		// resolved through pg::Translator at display time.
		std::string displayNameKey;
		std::string descriptionKey;
		spk::Vector2Int icon;
		AbilityCost cost;
		RangeDefinition range;
		AreaDefinition area;
		// Whether the chosen anchor cell is legal.
		TargetFilter anchorFilter;
		// Which occupants and cells of the already captured area are kept.
		TargetFilter affectedFilter;
		// Authored order is execution order and is preserved exactly.
		std::vector<EffectApplication> effects;
	};

	inline constexpr int AbilitySchemaVersion = 1;
	inline constexpr std::int64_t MaximumAbilityCost = 1000000;

	// Leaves the id empty: the registry loader assigns the validated filename stem.
	[[nodiscard]] AbilityDefinition parseAbilityDefinition(JsonReader &p_reader);
}
