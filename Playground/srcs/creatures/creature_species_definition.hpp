#pragma once

#include "core/definition_fields.hpp"
#include "core/game_rules.hpp"
#include "core/json.hpp"
#include "creatures/creature_attributes.hpp"
#include "taming/taming_profile_definition.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace pg
{
	// What step 14 needs to draw a creature, and nothing more: a coloured cube. The species layer
	// is headless, so it holds four channels and a scale rather than an spk::Color, a mesh, a
	// texture, an animation or an audio cue. Final asset metadata replaces this, it does not
	// extend it.
	struct PlaceholderVisual
	{
		std::array<std::uint8_t, 4> tint{255, 255, 255, 255};
		std::uint16_t scalePermille = 1000;

		[[nodiscard]] bool operator==(const PlaceholderVisual &p_other) const noexcept = default;
	};

	// A form is species-local: "base" belongs to the species that authored it, and two species may
	// each have one. Tier orders the evolution chain and is what stops a Feat Board from walking a
	// creature back down it.
	struct CreatureFormDefinition
	{
		std::string id;
		// A translation key, not a sentence (see resources/data/locales).
		std::string displayNameKey;
		std::uint32_t tier = 0;
		PlaceholderVisual presentation;
	};

	// The immutable half of a creature. It holds a baseline, not a state: no current form, no
	// unlocked loadout, no current HP, no progression, no level and no scaling curve. A creature's
	// difference from this baseline is entirely the Feat nodes it has completed.
	struct CreatureSpeciesDefinition
	{
		std::string id;
		// Translation keys, not sentences (see resources/data/locales).
		std::string displayNameKey;
		std::string descriptionKey;
		CreatureAttributes baseAttributes;
		std::vector<std::string> defaultAbilityIds;
		std::vector<std::string> defaultPassiveStatusIds;
		std::string featBoardId;
		// Absent means untameable: there is no "tameable: false" flag to disagree with it.
		std::optional<TamingProfileDefinition> tamingProfile;
		std::string defaultFormId;
		// Authored order, preserved: presentation and tests read it.
		std::vector<CreatureFormDefinition> forms;
		DefinitionSource source;

		[[nodiscard]] const CreatureFormDefinition *tryForm(std::string_view p_formId) const noexcept;
	};

	inline constexpr int CreatureSpeciesSchemaVersion = 1;
	inline constexpr std::uint32_t MaximumFormTier = 100;
	inline constexpr int MinimumFormScalePermille = 250;
	inline constexpr int MaximumFormScalePermille = 3000;

	// Leaves the id empty: the registry loader assigns the validated filename stem. Everything a
	// species can prove alone is proved here - schema, attribute bounds, form ids, tiers, the
	// taming tree's shape. Its ability, status, Feat Board and form-versus-board references are
	// resolved in core/combat_definition_validation.hpp, once those registries exist.
	[[nodiscard]] CreatureSpeciesDefinition parseCreatureSpeciesDefinition(
		JsonReader &p_reader,
		const GameRules &p_rules,
		const ConditionLimits &p_limits);
}
