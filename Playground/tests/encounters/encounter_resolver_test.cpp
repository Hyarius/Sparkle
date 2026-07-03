#include "encounters/biome.hpp"
#include "encounters/encounter_resolver.hpp"
#include "encounters/encounter_table.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace
{
	[[nodiscard]] pg::WeightedEncounterTeam team(std::string p_name, int p_weight)
	{
		return {
			.displayName = std::move(p_name),
			.weight = p_weight,
			.team = {{.speciesId = "sprout", .aiId = "test", .completedNodeUuids = {}}}};
	}

	[[nodiscard]] pg::BiomeEncounterRule ruleFor(const pg::EncounterTable &p_table, double p_chance)
	{
		return {.trigger = "bush", .table = &p_table, .chancePerStep = p_chance};
	}
}

TEST(EncounterResolver, SuppressesSameCellAndHonorsZeroAndCertainChance)
{
	pg::EncounterTable table;
	table.tiers[0].weightedTeams.push_back(team("certain", 1));
	pg::EncounterResolver resolver(42);

	EXPECT_FALSE(resolver.tryRoll(ruleFor(table, 0.0), {0, 0, 0}, 0).has_value());
	EXPECT_FALSE(resolver.tryRoll(ruleFor(table, 1.0), {0, 0, 0}, 0).has_value());
	EXPECT_TRUE(resolver.tryRoll(ruleFor(table, 1.0), {1, 0, 0}, 0).has_value());
	EXPECT_FALSE(resolver.tryRoll(ruleFor(table, 1.0), {1, 0, 0}, 0).has_value());
}

TEST(EncounterResolver, EmptyBadgeTierFallsBackToHighestPopulatedLowerTier)
{
	pg::EncounterTable table;
	table.tiers[0].weightedTeams.push_back(team("starter", 1));
	pg::EncounterResolver resolver(7);

	const auto result = resolver.tryRoll(ruleFor(table, 1.0), {2, 0, 0}, 1);
	ASSERT_TRUE(result.has_value());
	EXPECT_EQ(result->displayName, "starter");
}

TEST(EncounterResolver, WeightedPickSequenceIsPinnedForWorldSeed)
{
	pg::EncounterTable table;
	table.tiers[0].weightedTeams = {team("common", 3), team("rare", 1)};
	pg::EncounterResolver resolver(12345);
	const pg::BiomeEncounterRule rule = ruleFor(table, 1.0);
	std::vector<std::string> picked;
	for (int x = 0; x < 12; ++x)
	{
		const auto result = resolver.tryRoll(rule, {x, 0, 0}, 0);
		ASSERT_TRUE(result.has_value());
		picked.push_back(result->displayName);
	}

	EXPECT_EQ(picked, (std::vector<std::string>{"common", "common", "common", "common", "common", "rare", "rare", "common", "common", "common", "rare", "rare"}));
}
