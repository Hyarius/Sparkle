#pragma once

#include "encounters/encounter_types.hpp"

#include <array>
#include <cstddef>
#include <string>
#include <string_view>
#include <optional>
#include <vector>

namespace pg
{
	class JsonReader;

	enum class EncounterTierId : std::size_t
	{
		NoBadge,
		OneBadge,
		TwoBadges,
		ThreeBadges,
		FourBadges,
		FiveBadges,
		SixBadges,
		SevenBadges,
		EightBadges,
		PostGame,
		Count
	};

	struct WeightedEncounterTeam
	{
		std::string displayName;
		int weight = 0;
		std::vector<EncounterTeamMember> team;
		std::optional<spk::Vector2Int> boardSize;
	};

	struct EncounterTier
	{
		std::vector<WeightedEncounterTeam> weightedTeams;
	};

	struct EncounterTable
	{
		std::string id;
		std::array<EncounterTier, static_cast<std::size_t>(EncounterTierId::Count)> tiers;

		[[nodiscard]] const EncounterTier *tierForBadges(int p_badgeCount) const noexcept;
	};

	[[nodiscard]] std::string_view encounterTierName(EncounterTierId p_tier) noexcept;
	[[nodiscard]] EncounterTable parseEncounterTable(JsonReader &p_reader);
}
