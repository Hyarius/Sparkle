#include "encounters/encounter_table.hpp"

#include "core/json.hpp"

#include <algorithm>
#include <array>
#include <limits>

namespace
{
	constexpr std::array<std::string_view, static_cast<std::size_t>(pg::EncounterTierId::Count)> TierNames = {
		"noBadge",
		"oneBadge",
		"twoBadges",
		"threeBadges",
		"fourBadges",
		"fiveBadges",
		"sixBadges",
		"sevenBadges",
		"eightBadges",
		"postGame"};

	[[nodiscard]] std::string requireNonEmptyString(pg::JsonReader &p_reader, const std::string &p_key)
	{
		std::string result = p_reader.require<std::string>(p_key);
		if (result.empty())
		{
			throw pg::JsonError(p_reader.file(), p_reader.pathFor(p_key), "value must not be empty");
		}
		return result;
	}

	[[nodiscard]] pg::EncounterTeamMember parseMember(pg::JsonReader &p_reader)
	{
		p_reader.forbidUnknown({"species", "ai", "completedNodes"});
		pg::EncounterTeamMember result{
			.speciesId = requireNonEmptyString(p_reader, "species"),
			.aiId = requireNonEmptyString(p_reader, "ai"),
			.completedNodeUuids = p_reader.require<std::vector<std::string>>("completedNodes")};
		if (std::ranges::any_of(result.completedNodeUuids, &std::string::empty))
		{
			throw pg::JsonError(p_reader.file(), p_reader.pathFor("completedNodes"), "node ids must not be empty");
		}
		return result;
	}

	[[nodiscard]] pg::WeightedEncounterTeam parseWeightedTeam(pg::JsonReader &p_reader)
	{
		p_reader.forbidUnknown({"displayName", "weight", "team", "boardSize"});
		pg::WeightedEncounterTeam result;
		result.displayName = requireNonEmptyString(p_reader, "displayName");
		result.weight = p_reader.require<int>("weight");
		if (result.weight <= 0)
		{
			throw pg::JsonError(p_reader.file(), p_reader.pathFor("weight"), "weight must be positive");
		}

		if (!p_reader.contains("team"))
		{
			throw pg::JsonError(p_reader.file(), p_reader.pathFor("team"), "missing required field");
		}
		const nlohmann::json &team = p_reader.value().at("team");
		if (!team.is_array())
		{
			throw pg::JsonError(p_reader.file(), p_reader.pathFor("team"), "expected an array");
		}
		if (team.size() != 6)
		{
			throw pg::JsonError(p_reader.file(), p_reader.pathFor("team"), "team must contain exactly 6 slots");
		}
		for (std::size_t index = 0; index < team.size(); ++index)
		{
			if (team[index].is_null())
			{
				continue;
			}
			const std::string path = p_reader.pathFor("team") + "[" + std::to_string(index) + "]";
			if (!team[index].is_object())
			{
				throw pg::JsonError(p_reader.file(), path, "expected an object or null");
			}
			pg::JsonReader memberReader(team[index], p_reader.file(), path);
			result.team.push_back(parseMember(memberReader));
		}
		if (result.team.empty())
		{
			throw pg::JsonError(p_reader.file(), p_reader.pathFor("team"), "team must contain at least one member");
		}
		if (p_reader.contains("boardSize"))
		{
			const nlohmann::json size = p_reader.require<nlohmann::json>("boardSize");
			if (!size.is_array() || size.size() != 2 || !size[0].is_number_integer() ||
				!size[1].is_number_integer() || size[0].get<int>() <= 0 || size[1].get<int>() <= 0)
			{
				throw pg::JsonError(
					p_reader.file(), p_reader.pathFor("boardSize"),
					"expected an array of two positive integers");
			}
			result.boardSize = spk::Vector2Int{size[0].get<int>(), size[1].get<int>()};
		}
		return result;
	}
}

namespace pg
{
	const EncounterTier *EncounterTable::tierForBadges(int p_badgeCount) const noexcept
	{
		const int requested = std::clamp(p_badgeCount, 0, static_cast<int>(EncounterTierId::PostGame));
		for (int index = requested; index >= 0; --index)
		{
			const EncounterTier &candidate = tiers[static_cast<std::size_t>(index)];
			if (!candidate.weightedTeams.empty())
			{
				return &candidate;
			}
		}
		return nullptr;
	}

	std::string_view encounterTierName(EncounterTierId p_tier) noexcept
	{
		const std::size_t index = static_cast<std::size_t>(p_tier);
		return index < TierNames.size() ? TierNames[index] : std::string_view{};
	}

	EncounterTable parseEncounterTable(JsonReader &p_reader)
	{
		p_reader.forbidUnknown({"version", "tiers"});
		if (p_reader.require<int>("version") != 1)
		{
			throw JsonError(p_reader.file(), p_reader.pathFor("version"), "unsupported encounter table version");
		}

		JsonReader tiersReader = p_reader.child("tiers");
		tiersReader.forbidUnknown({"noBadge", "oneBadge", "twoBadges", "threeBadges", "fourBadges", "fiveBadges", "sixBadges", "sevenBadges", "eightBadges", "postGame"});

		EncounterTable result;
		for (std::size_t index = 0; index < TierNames.size(); ++index)
		{
			const std::string name(TierNames[index]);
			JsonReader tierReader = tiersReader.child(name);
			tierReader.forbidUnknown({"weightedTeams"});
			for (JsonReader teamReader : tierReader.childArray("weightedTeams"))
			{
				result.tiers[index].weightedTeams.push_back(parseWeightedTeam(teamReader));
			}
		}
		return result;
	}
}
