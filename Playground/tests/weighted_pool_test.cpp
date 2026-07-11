#include "world/weighted_id_parser.hpp"

#include "structures/container/spk_json_object.hpp"

#include <gtest/gtest.h>

#include <string>

namespace
{
	pg::JsonReader readerFor(const spk::JSON::Value &p_value)
	{
		return pg::JsonReader(p_value, "weighted-pool-test.json", "$.palette.test");
	}
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
