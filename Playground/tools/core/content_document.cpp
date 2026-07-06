#include "tools/core/content_document.hpp"

#include "abilities/ability.hpp"
#include "abilities/effect_describe.hpp"
#include "core/json.hpp"
#include "creatures/apply_progress.hpp"
#include "creatures/creature_species.hpp"
#include "creatures/creature_unit.hpp"
#include "encounters/encounter_table.hpp"
#include "feats/uuid.hpp"
#include "statuses/status.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace
{
	[[nodiscard]] bool validId(const std::string &p_id)
	{
		if (p_id.empty() || p_id.front() == '-' || p_id.back() == '-')
		{
			return false;
		}
		bool dash = false;
		for (const unsigned char character : p_id)
		{
			const bool currentDash = character == '-';
			if ((!std::islower(character) && !std::isdigit(character) && !currentDash) || (dash && currentDash))
			{
				return false;
			}
			dash = currentDash;
		}
		return true;
	}

	[[nodiscard]] nlohmann::json emptyTiers()
	{
		nlohmann::json tiers = nlohmann::json::object();
		for (std::size_t index = 0; index < static_cast<std::size_t>(pg::EncounterTierId::Count); ++index)
		{
			tiers[std::string(pg::encounterTierName(static_cast<pg::EncounterTierId>(index)))] = {{"weightedTeams", nlohmann::json::array()}};
		}
		return tiers;
	}
}

namespace pg::tools
{
	void ContentDocument::load(const std::filesystem::path &p_file, Domain p_domain)
	{
		_domain = p_domain;
		_file = p_file;
		_sourceFile = p_file;
		_id = p_file.stem().string();
		_json = JsonLoader::parseFile(p_file);
		_dirty = false;
	}

	void ContentDocument::create(
		const std::filesystem::path &p_dataDirectory,
		std::string p_id,
		Domain p_domain,
		const Registries &p_registries)
	{
		if (!validId(p_id))
		{
			throw std::invalid_argument("Invalid content id");
		}
		_domain = p_domain;
		_id = std::move(p_id);
		_file = p_dataDirectory / directoryName(_domain) / (_id + ".json");
		_sourceFile.clear();
		if (_domain == Domain::Encounter)
		{
			_json = {{"version", 1}, {"tiers", emptyTiers()}};
		}
		else if (_domain == Domain::Ability)
		{
			_json = {
				{"version", 1}, {"displayName", "New Ability"}, {"icon", {0, 0}}, {"cost", {{"ap", 1}, {"mp", 0}}}, {"range", {{"shape", "circle"}, {"min", 1}, {"max", 1}, {"requiresLineOfSight", true}}}, {"areaOfEffect", {{"shape", "square"}, {"value", 0}}}, {"targetProfile", "enemy"}, {"targetAnimation", "TakeDamage"}, {"effects", nlohmann::json::array({effectDefaults("damage")})}};
		}
		else if (_domain == Domain::Status)
		{
			_json = {
				{"version", 1}, {"displayName", "New Status"}, {"icon", {0, 0}}, {"tags", nlohmann::json::array({"status"})}, {"hookPoint", "turnStart"}, {"effects", nlohmann::json::array({effectDefaults("damage")})}};
		}
		else
		{
			const auto abilities = p_registries.abilities().ids();
			const auto boards = p_registries.featBoards().ids();
			_json = {
				{"version", 1}, {"displayName", "New Species"}, {"attributes", {{"health", 10}, {"ap", 5}, {"mp", 3}, {"attack", 1}, {"armor", 0}, {"armorPenetration", 0}, {"magic", 1}, {"resistance", 0}, {"resistancePenetration", 0}, {"bonusRange", 0}, {"stamina", 3.0}, {"staminaRate", 1.0}, {"bonusHealing", 0.0}, {"lifeSteal", 0.0}, {"omnivampirism", 0.0}, {"timeEffectResistance", 0.0}}}, {"defaultAbilities", abilities.empty() ? nlohmann::json::array() : nlohmann::json::array({abilities.front()})}, {"featBoard", boards.empty() ? "" : boards.front()}, {"defaultFormId", "base"}, {"forms", {{"base", {{"displayName", "Base"}, {"tier", 0}, {"model", "placeholder-cube"}, {"animationSet", "basic-creature"}, {"avatar", {0, 0}}}}}}, {"tamingProfile", {{"conditions", nlohmann::json::array()}}}};
		}
		_dirty = true;
	}

	void ContentDocument::save(const JsonWriter &p_writer)
	{
		p_writer.write(_file, _json);
		if (!_sourceFile.empty() && _sourceFile != _file)
		{
			std::error_code error;
			std::filesystem::remove(_sourceFile, error);
			if (error)
			{
				throw std::runtime_error("Unable to remove renamed content: " + error.message());
			}
		}
		_sourceFile = _file;
		_dirty = false;
	}

	ContentDocument::Domain ContentDocument::domain() const noexcept
	{
		return _domain;
	}
	const std::filesystem::path &ContentDocument::file() const noexcept
	{
		return _file;
	}
	const std::string &ContentDocument::id() const noexcept
	{
		return _id;
	}
	const nlohmann::json &ContentDocument::json() const noexcept
	{
		return _json;
	}
	nlohmann::json &ContentDocument::data() noexcept
	{
		return _json;
	}
	nlohmann::json &ContentDocument::editJson() noexcept
	{
		_dirty = true;
		return _json;
	}
	bool ContentDocument::dirty() const noexcept
	{
		return _dirty;
	}

	void ContentDocument::rename(const std::string &p_id)
	{
		if (p_id == _id)
		{
			return;
		}
		if (!validId(p_id))
		{
			throw std::invalid_argument("Ids use lower-case letters, digits, and single hyphens");
		}
		_id = p_id;
		_file = _file.parent_path() / (_id + ".json");
		_dirty = true;
	}

	std::string ContentDocument::directoryName(Domain p_domain)
	{
		switch (p_domain)
		{
		case Domain::Encounter:
			return "encounter-tables";
		case Domain::Ability:
			return "abilities";
		case Domain::Status:
			return "statuses";
		case Domain::Species:
			return "creatures";
		}
		return {};
	}

	std::vector<std::string> ContentDocument::effectTypes()
	{
		return {"damage", "heal", "applyStatus", "removeStatus", "consumeStatus", "cleanse", "revive", "applyShield", "resourceChange", "stealResource", "displacement", "swapPosition", "swapPositionWithCaster", "teleportSelf", "adjustTurnBarTime", "adjustTurnBarDuration", "placeObject", "removeObject"};
	}

	nlohmann::json ContentDocument::effectDefaults(const std::string &p_type)
	{
		if (p_type == "damage")
		{
			return {{"type", p_type}, {"baseDamage", 1}, {"damageKind", "physical"}, {"attackRatio", 0.0}, {"magicRatio", 0.0}};
		}
		if (p_type == "heal")
		{
			return {{"type", p_type}, {"baseHealing", 1}, {"attackRatio", 0.0}, {"magicRatio", 0.0}};
		}
		if (p_type == "applyStatus")
		{
			return {{"type", p_type}, {"status", "burn"}, {"stackCount", 1}, {"duration", {{"type", "turnBased"}, {"turns", 1}}}};
		}
		if (p_type == "removeStatus" || p_type == "consumeStatus")
		{
			return {{"type", p_type}, {"status", "burn"}, {"stackCount", 1}};
		}
		if (p_type == "cleanse" || p_type == "removeObject")
		{
			return {{"type", p_type}, {"tags", nlohmann::json::array({"harmful"})}};
		}
		if (p_type == "revive")
		{
			return {{"type", p_type}, {"healthRestored", 1}};
		}
		if (p_type == "applyShield")
		{
			return {{"type", p_type}, {"kind", "physical"}, {"amount", 1}, {"durationTurns", 1}};
		}
		if (p_type == "resourceChange")
		{
			return {{"type", p_type}, {"resource", "ap"}, {"value", 1}};
		}
		if (p_type == "stealResource")
		{
			return {{"type", p_type}, {"resource", "health"}, {"value", 1}};
		}
		if (p_type == "displacement")
		{
			return {{"type", p_type}, {"orientation", "awayFromCaster"}, {"force", 1}};
		}
		if (p_type == "swapPosition" || p_type == "swapPositionWithCaster" || p_type == "teleportSelf")
		{
			return {{"type", p_type}};
		}
		if (p_type == "adjustTurnBarTime" || p_type == "adjustTurnBarDuration")
		{
			return {{"type", p_type}, {"delta", 0.5}};
		}
		if (p_type == "placeObject")
		{
			return {{"type", p_type}, {"object", "object"}, {"duration", {{"type", "turnBased"}, {"turns", 1}}}};
		}
		throw std::invalid_argument("Unknown effect type '" + p_type + "'");
	}

	std::vector<std::string> ContentDocument::validate(const Registries &p_registries) const
	{
		std::vector<std::string> errors;
		try
		{
			nlohmann::json copy = _json;
			JsonReader reader(copy, _file);
			if (_domain == Domain::Ability)
			{
				(void)parseAbility(reader, p_registries.statuses());
			}
			else if (_domain == Domain::Status)
			{
				Status status = parseStatus(reader);
				resolveStatusReferences(status, p_registries.statuses(), _file);
			}
			else if (_domain == Domain::Encounter)
			{
				(void)parseEncounterTable(reader);
				for (const auto &[tierName, tier] : _json.at("tiers").items())
				{
					(void)tierName;
					for (const auto &team : tier.at("weightedTeams"))
					{
						for (const auto &slot : team.at("team"))
						{
							if (slot.is_null())
							{
								continue;
							}
							const std::string speciesId = slot.value("species", "");
							const CreatureSpecies *species = p_registries.creatures().tryGet(speciesId);
							if (species == nullptr)
							{
								throw std::runtime_error("unknown species '" + speciesId + "'");
							}
							if (p_registries.ai().tryGet(slot.value("ai", "")) == nullptr)
							{
								throw std::runtime_error("unknown AI id");
							}
							for (const auto &uuid : slot.at("completedNodes"))
							{
								const auto parsed = uuidFromString(uuid.get<std::string>());
								if (!parsed.has_value() || species->featBoard == nullptr || species->featBoard->tryNode(*parsed) == nullptr)
								{
									throw std::runtime_error("completed node does not belong to species board");
								}
							}
						}
					}
				}
			}
			else
			{
				(void)parseCreatureSpecies(reader, p_registries.abilities(), p_registries.featBoards());
			}
		} catch (const std::exception &exception)
		{
			errors.push_back(exception.what());
		}
		return errors;
	}

	std::string ContentDocument::rulesPreview(const Registries &p_registries) const
	{
		if (_domain != Domain::Ability && _domain != Domain::Status)
		{
			return {};
		}
		try
		{
			nlohmann::json copy = _json;
			JsonReader reader(copy, _file);
			std::ostringstream result;
			if (_domain == Domain::Ability)
			{
				const Ability parsed = parseAbility(reader, p_registries.statuses());
				for (const auto &effect : parsed.effects)
				{
					result << describe(*effect) << '\n';
				}
			}
			else
			{
				const Status parsed = parseStatus(reader);
				for (const auto &effect : parsed.effects)
				{
					result << describe(*effect) << '\n';
				}
			}
			return result.str();
		} catch (const std::exception &exception)
		{
			return std::string("Preview unavailable: ") + exception.what();
		}
	}

	std::string ContentDocument::completedNodesSummary(
		const std::string &p_speciesId,
		const nlohmann::json &p_completedNodes,
		const Registries &p_registries)
	{
		const CreatureSpecies *species = p_registries.creatures().tryGet(p_speciesId);
		if (species == nullptr)
		{
			return "Unknown species";
		}
		CreatureUnit unit(*species);
		if (species->featBoard != nullptr)
		{
			nlohmann::json progress = nlohmann::json::object();
			for (const auto &uuid : p_completedNodes)
			{
				progress[uuid.get<std::string>()] = {{"completionCount", 1}};
			}
			(void)unit.featBoardProgress.fromJson(progress, *species->featBoard);
			applyProgress(unit);
		}
		return unit.currentFormId + " | HP " + std::to_string(unit.attributes.health) + " AP " + std::to_string(unit.attributes.ap) +
			   " MP " + std::to_string(unit.attributes.mp) + " | " + std::to_string(unit.abilities.size()) + " abilities, " +
			   std::to_string(unit.permanentPassives.size()) + " passives";
	}
}
