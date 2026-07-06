#include "encounters/encounter_resolver.hpp"

#include "encounters/biome.hpp"
#include "encounters/encounter_table.hpp"

#include <cstdint>
#include <limits>
#include <stdexcept>

namespace pg
{
	EncounterResolver::EncounterResolver(int p_worldSeed)
	{
		std::seed_seq seed{
			static_cast<std::uint32_t>(p_worldSeed),
			static_cast<std::uint32_t>(StreamId)};
		_random.seed(seed);
	}

	std::optional<ResolvedEncounter> EncounterResolver::tryRoll(
		const BiomeEncounterRule &p_rule,
		const spk::Vector3Int &p_cell,
		int p_badgeCount)
	{
		if (_lastCheckedCell.has_value() && *_lastCheckedCell == p_cell)
		{
			return std::nullopt;
		}
		_lastCheckedCell = p_cell;
		if (p_rule.table == nullptr || p_rule.chancePerStep <= 0.0)
		{
			return std::nullopt;
		}
		if (p_rule.chancePerStep < 1.0 &&
			std::generate_canonical<double, std::numeric_limits<double>::digits>(_random) >= p_rule.chancePerStep)
		{
			return std::nullopt;
		}

		const EncounterTier *tier = p_rule.table->tierForBadges(p_badgeCount);
		if (tier == nullptr)
		{
			return std::nullopt;
		}
		int totalWeight = 0;
		for (const WeightedEncounterTeam &candidate : tier->weightedTeams)
		{
			if (candidate.weight > std::numeric_limits<int>::max() - totalWeight)
			{
				throw std::overflow_error("encounter team weights exceed integer range");
			}
			totalWeight += candidate.weight;
		}
		std::uniform_int_distribution<int> pick(1, totalWeight);
		int value = pick(_random);
		for (const WeightedEncounterTeam &candidate : tier->weightedTeams)
		{
			value -= candidate.weight;
			if (value <= 0)
			{
				return ResolvedEncounter{candidate.displayName, candidate.team, candidate.boardSize};
			}
		}
		throw std::logic_error("encounter weighted pick failed");
	}

	void EncounterResolver::observeCell(const spk::Vector3Int &p_cell) noexcept
	{
		_lastCheckedCell = p_cell;
	}
}
