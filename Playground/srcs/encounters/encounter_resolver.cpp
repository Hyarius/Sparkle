#include "encounters/encounter_resolver.hpp"

#include <stdexcept>

namespace pg
{
	const EncounterTierDefinition &selectEncounterTier(
		const EncounterDefinition &p_definition,
		std::uint32_t p_progressionTier)
	{
		if (p_progressionTier > MaximumProgressionTier)
		{
			throw std::invalid_argument(
				"encounter '" + p_definition.id + "': progression tier " + std::to_string(p_progressionTier) +
				" is above the maximum of " + std::to_string(MaximumProgressionTier));
		}
		if (p_definition.tiers.empty())
		{
			throw std::invalid_argument("encounter '" + p_definition.id + "' authors no tier");
		}

		// Authored strictly increasing and starting at tier 0, so the first entry is always a legal
		// answer and the walk is a plain scan rather than a search with a fallback.
		const EncounterTierDefinition *selected = &p_definition.tiers.front();
		for (const EncounterTierDefinition &tier : p_definition.tiers)
		{
			if (tier.tier > p_progressionTier)
			{
				break;
			}
			selected = &tier;
		}
		return *selected;
	}

	const EncounterTeamDefinition &selectWeightedTeam(const EncounterTierDefinition &p_tier, BattleRng &p_rng)
	{
		std::uint64_t total = 0;
		for (const EncounterTeamDefinition &team : p_tier.teams)
		{
			if (team.weight == 0)
			{
				throw std::invalid_argument("tier " + std::to_string(p_tier.tier) + " holds a zero-weight team");
			}
			total += team.weight;
		}
		if (total == 0)
		{
			throw std::invalid_argument("tier " + std::to_string(p_tier.tier) + " holds no team");
		}

		std::uint64_t ticket = p_rng.uniformBelow(total);
		for (const EncounterTeamDefinition &team : p_tier.teams)
		{
			if (ticket < team.weight)
			{
				return team;
			}
			ticket -= team.weight;
		}

		// The ticket is below the sum of the weights it was just walked against, so this is not a
		// content error: it is the loop above being wrong.
		throw std::logic_error("weighted team selection walked past the end of its own total");
	}

	bool rollEncounterTrigger(const EncounterDefinition &p_definition, BattleRng &p_rng)
	{
		// Drawn even at 0 and at 1000: a Bush step has to cost the same number of bounded samples
		// whatever the biome authored, or the stream would fork on the content.
		const std::uint64_t roll = p_rng.uniformBelow(MaximumTriggerChancePermille);
		return roll < p_definition.triggerChancePermille;
	}

	ResolvedEncounter resolveEncounter(
		const EncounterDefinition &p_definition,
		std::uint32_t p_progressionTier,
		BattleRng &p_rng)
	{
		const EncounterTierDefinition &tier = selectEncounterTier(p_definition, p_progressionTier);
		const EncounterTeamDefinition &team = selectWeightedTeam(tier, p_rng);

		ResolvedEncounter result;
		result.encounterDefinitionId = p_definition.id;
		result.kind = p_definition.kind;
		result.allowsTaming = p_definition.allowsTaming;
		result.repeatable = p_definition.repeatable;
		result.resolvedTier = tier.tier;
		result.teamId = team.id;
		result.board = p_definition.board;
		result.enemyRoster = team.members;
		return result;
	}
}
