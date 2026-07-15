#include "creatures/creature_species_definition.hpp"

#include "core/content_id.hpp"

#include <algorithm>
#include <set>

namespace pg
{
	namespace
	{
		[[nodiscard]] PlaceholderVisual parsePresentation(JsonReader &p_reader)
		{
			p_reader.forbidUnknown({"tint", "scalePermille"});

			PlaceholderVisual result;
			const std::array<int, 4> tint = p_reader.require<std::array<int, 4>>("tint");
			for (std::size_t channel = 0; channel < tint.size(); ++channel)
			{
				if (tint[channel] < 0 || tint[channel] > 255)
				{
					throw JsonError(
						p_reader.file(),
						p_reader.pathFor("tint"),
						"colour channels are integers in [0, 255], got " + std::to_string(tint[channel]));
				}
				result.tint[channel] = static_cast<std::uint8_t>(tint[channel]);
			}

			result.scalePermille = static_cast<std::uint16_t>(
				requireIntegerInRange(p_reader, "scalePermille", MinimumFormScalePermille, MaximumFormScalePermille));
			return result;
		}

		[[nodiscard]] CreatureFormDefinition parseForm(JsonReader &p_reader)
		{
			p_reader.forbidUnknown({"id", "displayNameKey", "tier", "icon", "presentation"});

			CreatureFormDefinition result;
			result.id = p_reader.require<std::string>("id");
			requireContentId(result.id, p_reader.file(), p_reader.pathFor("id"), "form id");
			result.displayNameKey = requireDisplayNameKey(p_reader);
			result.tier = static_cast<std::uint32_t>(requireIntegerInRange(p_reader, "tier", 0, MaximumFormTier));
			result.icon = requireIcon(p_reader);

			JsonReader presentation = p_reader.child("presentation");
			result.presentation = parsePresentation(presentation);
			return result;
		}

		[[nodiscard]] std::vector<CreatureFormDefinition> parseForms(const JsonReader &p_reader)
		{
			std::vector<JsonReader> entries = p_reader.childArray("forms");
			if (entries.empty())
			{
				throw JsonError(p_reader.file(), p_reader.pathFor("forms"), "a species has at least one form");
			}

			std::vector<CreatureFormDefinition> result;
			result.reserve(entries.size());
			std::set<std::string> ids;
			for (JsonReader &entry : entries)
			{
				CreatureFormDefinition form = parseForm(entry);
				if (!ids.insert(form.id).second)
				{
					throw JsonError(entry.file(), entry.path(), "duplicate form id '" + form.id + "'");
				}
				result.push_back(std::move(form));
			}
			return result;
		}

		[[nodiscard]] std::vector<std::string> parseReferencedIds(
			const JsonReader &p_reader,
			const std::string &p_key,
			std::string_view p_kind)
		{
			const std::vector<std::string> authored = p_reader.require<std::vector<std::string>>(p_key);

			std::vector<std::string> result;
			result.reserve(authored.size());
			for (const std::string &id : authored)
			{
				requireContentId(id, p_reader.file(), p_reader.pathFor(p_key), p_kind);
				if (std::ranges::find(result, id) != result.end())
				{
					throw JsonError(p_reader.file(), p_reader.pathFor(p_key), "duplicate entry '" + id + "'");
				}
				result.push_back(id);
			}
			return result;
		}
	}

	const CreatureFormDefinition *CreatureSpeciesDefinition::tryForm(std::string_view p_formId) const noexcept
	{
		const auto entry = std::ranges::find(forms, p_formId, &CreatureFormDefinition::id);
		return entry == forms.end() ? nullptr : &*entry;
	}

	CreatureSpeciesDefinition parseCreatureSpeciesDefinition(
		JsonReader &p_reader,
		const GameRules &p_rules,
		const ConditionLimits &p_limits)
	{
		p_reader.forbidUnknown(
			{"version",
			 "displayNameKey",
			 "descriptionKey",
			 "attributes",
			 "defaultAbilities",
			 "defaultPassives",
			 "featBoard",
			 "defaultForm",
			 "forms",
			 "tamingProfile"});
		requireVersion(p_reader, CreatureSpeciesSchemaVersion);

		CreatureSpeciesDefinition result;
		result.displayNameKey = requireDisplayNameKey(p_reader);
		result.descriptionKey = requireDescriptionKey(p_reader);
		result.source = DefinitionSource{p_reader.file(), p_reader.path()};

		JsonReader attributes = p_reader.child("attributes");
		result.baseAttributes = parseCreatureAttributes(attributes, p_rules);

		result.defaultAbilityIds = parseReferencedIds(p_reader, "defaultAbilities", "ability id");
		if (result.defaultAbilityIds.empty())
		{
			throw JsonError(
				p_reader.file(),
				p_reader.pathFor("defaultAbilities"),
				"a species knows at least one ability before its first Feat node");
		}
		if (result.defaultAbilityIds.size() > p_rules.battle.abilitySlotCapacity)
		{
			throw JsonError(
				p_reader.file(),
				p_reader.pathFor("defaultAbilities"),
				"a species knows at most gameRules.battle.abilitySlotCapacity (" +
					std::to_string(p_rules.battle.abilitySlotCapacity) + ") abilities, got " +
					std::to_string(result.defaultAbilityIds.size()));
		}
		// Empty is ordinary: most species start with no passive at all.
		result.defaultPassiveStatusIds = parseReferencedIds(p_reader, "defaultPassives", "status id");

		result.featBoardId = p_reader.require<std::string>("featBoard");
		requireContentId(result.featBoardId, p_reader.file(), p_reader.pathFor("featBoard"), "feat board id");

		result.defaultFormId = p_reader.require<std::string>("defaultForm");
		requireContentId(result.defaultFormId, p_reader.file(), p_reader.pathFor("defaultForm"), "form id");
		result.forms = parseForms(p_reader);

		const CreatureFormDefinition *defaultForm = result.tryForm(result.defaultFormId);
		if (defaultForm == nullptr)
		{
			throw JsonError(
				p_reader.file(),
				p_reader.pathFor("defaultForm"),
				"defaultForm names '" + result.defaultFormId + "', which this species does not define");
		}

		// The chain starts at tier 0 and a Feat Board can only climb it, so exactly one form is at
		// the bottom and it is the one a fresh creature wears.
		const std::size_t baseForms = static_cast<std::size_t>(
			std::ranges::count(result.forms, std::uint32_t{0}, &CreatureFormDefinition::tier));
		if (baseForms != 1)
		{
			throw JsonError(
				p_reader.file(),
				p_reader.pathFor("forms"),
				"a species has exactly one tier-0 form, got " + std::to_string(baseForms));
		}
		if (defaultForm->tier != 0)
		{
			throw JsonError(
				p_reader.file(),
				p_reader.pathFor("defaultForm"),
				"the default form is the tier-0 form; '" + defaultForm->id + "' is tier " +
					std::to_string(defaultForm->tier));
		}

		// Omission means untameable. An explicit null is a value of the wrong type, and gets the
		// ordinary path-aware error rather than a second spelling of "absent".
		if (p_reader.contains("tamingProfile"))
		{
			JsonReader profile = p_reader.child("tamingProfile");
			result.tamingProfile = parseTamingProfile(profile, p_limits);
		}
		return result;
	}
}
