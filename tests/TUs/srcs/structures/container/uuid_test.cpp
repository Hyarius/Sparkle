#include <gtest/gtest.h>

#include <sstream>
#include <unordered_set>

#include "structures/container/spk_uuid.hpp"

TEST(UuidTest, DefaultConstructedIsNull)
{
	spk::UUID uuid;

	EXPECT_TRUE(uuid.isNull());
	EXPECT_EQ(uuid, spk::UUID::null());
}

TEST(UuidTest, GenerateProducesNonNullValue)
{
	spk::UUID uuid = spk::UUID::generate();

	EXPECT_FALSE(uuid.isNull());
}

TEST(UuidTest, GenerateProducesDistinctValues)
{
	std::unordered_set<spk::UUID> seen;

	for (int i = 0; i < 1000; ++i)
	{
		seen.insert(spk::UUID::generate());
	}

	EXPECT_EQ(seen.size(), 1000u);
}

TEST(UuidTest, GenerateSetsVersion4AndVariantBits)
{
	spk::UUID uuid = spk::UUID::generate();
	const spk::UUID::Storage &bytes = uuid.bytes();

	EXPECT_EQ(bytes[6] & 0xF0u, 0x40u);
	EXPECT_EQ(bytes[8] & 0xC0u, 0x80u);
}

TEST(UuidTest, ToStringIsCanonicalFormat)
{
	spk::UUID uuid = spk::UUID::generate();
	const std::string text = uuid.toString();

	ASSERT_EQ(text.size(), 36u);
	EXPECT_EQ(text[8], '-');
	EXPECT_EQ(text[13], '-');
	EXPECT_EQ(text[18], '-');
	EXPECT_EQ(text[23], '-');
}

TEST(UuidTest, EqualityFollowsBytes)
{
	spk::UUID::Storage bytes{};
	bytes[0] = 0x12u;

	spk::UUID first(bytes);
	spk::UUID second(bytes);

	EXPECT_EQ(first, second);

	bytes[0] = 0x34u;
	spk::UUID third(bytes);

	EXPECT_NE(first, third);
}

TEST(UuidTest, StreamOperatorMatchesToString)
{
	spk::UUID uuid = spk::UUID::generate();

	std::ostringstream stream;
	stream << uuid;

	EXPECT_EQ(stream.str(), uuid.toString());
}

TEST(UuidTest, HashIsUsableInUnorderedSet)
{
	spk::UUID uuid = spk::UUID::generate();
	std::unordered_set<spk::UUID> set;

	set.insert(uuid);

	EXPECT_EQ(set.count(uuid), 1u);
}
