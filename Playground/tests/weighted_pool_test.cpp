#include "core/deterministic_random.hpp"
#include "core/weighted_pool.hpp"
#include "world/weighted_id_parser.hpp"

#include "structures/container/spk_json_object.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>

namespace
{
	pg::JsonReader readerFor(const spk::JSON::Value &p_value)
	{
		return pg::JsonReader(p_value, "weighted-pool-test.json", "$.palette.test");
	}
}

TEST(WeightedPool, PrecomputesTotalAndSelectsByWeight)
{
	pg::WeightedPool<std::string> pool;
	pool.add("common", 3.0);
	pool.add("rare", 1.0);

	EXPECT_DOUBLE_EQ(pool.totalWeight(), 4.0);
	EXPECT_EQ(pool.pick(0.0), "common");
	EXPECT_EQ(pool.pick(0.749), "common");
	EXPECT_EQ(pool.pick(0.75), "rare");
	EXPECT_EQ(pool.pickInclusive(0.75), "common");
	EXPECT_EQ(pool.pickTarget(3.5), "rare");
}

TEST(WeightedPool, RejectsInvalidWeightsAndEmptySelection)
{
	pg::WeightedPool<int> pool;
	EXPECT_THROW(pool.add(1, 0.0), std::invalid_argument);
	EXPECT_THROW(pool.add(1, std::numeric_limits<double>::infinity()), std::invalid_argument);
	EXPECT_THROW((void)pool.pick(0.5), std::logic_error);
}

TEST(WeightedIdParser, AcceptsEverySharedSchemaShape)
{
	const spk::JSON::Value value = spk::JSON::Value::fromString(R"([
		"plain",
		{"voxel": "weighted", "weight": 4},
		{"id": "chance", "chance": 2},
		{"mapped": 3},
		null
	])");
	const pg::JsonReader reader = readerFor(value);
	const std::vector<pg::WeightedId> result = pg::parseWeightedIds(value, reader, reader.path());

	ASSERT_EQ(result.size(), 5U);
	EXPECT_EQ(result[0].id, "plain");
	EXPECT_DOUBLE_EQ(result[0].weight, 1.0);
	EXPECT_EQ(result[1].id, "weighted");
	EXPECT_DOUBLE_EQ(result[1].weight, 4.0);
	EXPECT_EQ(result[2].id, "chance");
	EXPECT_DOUBLE_EQ(result[2].weight, 2.0);
	EXPECT_EQ(result[3].id, "mapped");
	EXPECT_DOUBLE_EQ(result[3].weight, 3.0);
	EXPECT_FALSE(result[4].id.has_value());
}

TEST(WeightedIdParser, RejectsAmbiguousOrInvalidWeights)
{
	const spk::JSON::Value ambiguous =
		spk::JSON::Value::fromString(R"({"id":"a","voxel":"b"})");
	const pg::JsonReader ambiguousReader = readerFor(ambiguous);
	EXPECT_THROW(
		(void)pg::parseWeightedIds(ambiguous, ambiguousReader, ambiguousReader.path()),
		pg::JsonError);

	const spk::JSON::Value invalid = spk::JSON::Value::fromString(R"({"a":0})");
	const pg::JsonReader invalidReader = readerFor(invalid);
	EXPECT_THROW((void)pg::parseWeightedIds(invalid, invalidReader, invalidReader.path()), pg::JsonError);
}

TEST(DeterministicRandom, PreservesExistingHashAlgorithms)
{
	EXPECT_EQ(pg::deterministic::fnv1a("coast-stair-length#0"), 0xb6dfcb319c295309ULL);
	EXPECT_EQ(pg::deterministic::fnv1a("1234::terrain/zones"), 0xd239281bd2e5d6b8ULL);
	EXPECT_EQ(pg::deterministic::avalanche(0x123456789abcdef0ULL), 0x9629f58e8ec5b906ULL);
}
