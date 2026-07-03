#include "creatures/creature_species.hpp"

#include "abilities/ability.hpp"
#include "core/json.hpp"
#include "core/registry.hpp"

#include <stdexcept>

namespace
{
	[[nodiscard]] std::string nonEmpty(pg::JsonReader &p_reader, const std::string &p_key)
	{
		std::string value = p_reader.require<std::string>(p_key);
		if (value.empty())
		{
			throw pg::JsonError(p_reader.file(), p_reader.pathFor(p_key), "value must not be empty");
		}
		return value;
	}

	[[nodiscard]] spk::Vector2Int parseAvatar(
		const nlohmann::json &p_json,
		const std::filesystem::path &p_file,
		const std::string &p_path)
	{
		if (!p_json.is_array() || p_json.size() != 2 || !p_json[0].is_number_integer() || !p_json[1].is_number_integer())
		{
			throw pg::JsonError(p_file, p_path, "expected an array of two integers");
		}
		const spk::Vector2Int result{p_json[0].get<int>(), p_json[1].get<int>()};
		if (result.x < 0 || result.y < 0)
		{
			throw pg::JsonError(p_file, p_path, "avatar coordinates must be non-negative");
		}
		return result;
	}
}

namespace pg
{
	const CreatureForm &CreatureSpecies::form(const std::string &p_id) const
	{
		const auto found = forms.find(p_id);
		if (found == forms.end())
		{
			throw std::out_of_range("unknown creature form '" + p_id + "' for species '" + id + "'");
		}
		return found->second;
	}

	CreatureSpecies parseCreatureSpecies(JsonReader &p_reader, const Registry<Ability> &p_abilities)
	{
		// tamingProfile is deliberately accepted but not interpreted until step 19.
		p_reader.forbidUnknown({"version", "displayName", "attributes", "defaultAbilities", "featBoard", "defaultFormId", "forms", "tamingProfile"});
		if (p_reader.require<int>("version") != 1)
		{
			throw JsonError(p_reader.file(), p_reader.pathFor("version"), "unsupported creature species version");
		}

		CreatureSpecies result;
		result.displayName = nonEmpty(p_reader, "displayName");
		JsonReader attributes = p_reader.child("attributes");
		result.attributes = parseAttributes(attributes);
		result.featBoardId = nonEmpty(p_reader, "featBoard");
		result.defaultFormId = nonEmpty(p_reader, "defaultFormId");

		const nlohmann::json abilityIds = p_reader.require<nlohmann::json>("defaultAbilities");
		if (!abilityIds.is_array())
		{
			throw JsonError(p_reader.file(), p_reader.pathFor("defaultAbilities"), "expected an array");
		}
		for (std::size_t index = 0; index < abilityIds.size(); ++index)
		{
			const std::string path = p_reader.pathFor("defaultAbilities") + "[" + std::to_string(index) + "]";
			if (!abilityIds[index].is_string() || abilityIds[index].get<std::string>().empty())
			{
				throw JsonError(p_reader.file(), path, "expected a non-empty ability id");
			}
			const std::string id = abilityIds[index].get<std::string>();
			const Ability *ability = p_abilities.tryGet(id);
			if (ability == nullptr)
			{
				throw JsonError(p_reader.file(), path, "unknown ability id '" + id + "'");
			}
			result.defaultAbilities.push_back(ability);
		}

		const nlohmann::json forms = p_reader.require<nlohmann::json>("forms");
		if (!forms.is_object() || forms.empty())
		{
			throw JsonError(p_reader.file(), p_reader.pathFor("forms"), "expected a non-empty object");
		}
		for (const auto &[id, json] : forms.items())
		{
			const std::string path = p_reader.pathFor("forms") + "." + id;
			if (id.empty() || !json.is_object())
			{
				throw JsonError(p_reader.file(), path, "expected a named form object");
			}
			JsonReader formReader(json, p_reader.file(), path);
			formReader.forbidUnknown({"displayName", "tier", "model", "animationSet", "avatar"});
			CreatureForm form{
				.displayName = nonEmpty(formReader, "displayName"),
				.tier = formReader.require<int>("tier"),
				.modelId = nonEmpty(formReader, "model"),
				.animationSetId = nonEmpty(formReader, "animationSet"),
				.avatar = parseAvatar(
					formReader.require<nlohmann::json>("avatar"), p_reader.file(), formReader.pathFor("avatar"))};
			if (form.tier < 0)
			{
				throw JsonError(p_reader.file(), formReader.pathFor("tier"), "tier must be non-negative");
			}
			result.forms.emplace(id, std::move(form));
		}
		if (!result.forms.contains(result.defaultFormId))
		{
			throw JsonError(p_reader.file(), p_reader.pathFor("defaultFormId"), "unknown default form id '" + result.defaultFormId + "'");
		}
		return result;
	}
}
