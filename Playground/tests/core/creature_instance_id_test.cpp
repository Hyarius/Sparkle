#include <gtest/gtest.h>

#include "core/creature_instance_id.hpp"

#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace
{
	using pg::CreatureInstanceId;

	static_assert(std::is_trivially_copyable_v<CreatureInstanceId>);
}

TEST(CreatureInstanceIdTest, DefaultIsInvalidAndHasNoText)
{
	constexpr CreatureInstanceId value;
	EXPECT_FALSE(value.valid());
	EXPECT_EQ(value.serial(), 0U);
	EXPECT_TRUE(value.string().empty());
}

TEST(CreatureInstanceIdTest, FormatsSerialsCanonically)
{
	EXPECT_EQ(CreatureInstanceId::fromSerial(1U).string(), "creature-0000000000000001");
	EXPECT_EQ(CreatureInstanceId::fromSerial(0x2aU).string(), "creature-000000000000002a");
	EXPECT_EQ(CreatureInstanceId::fromSerial(0xdeadbeefU).string(), "creature-00000000deadbeef");
	EXPECT_EQ(
		CreatureInstanceId::fromSerial(std::numeric_limits<std::uint64_t>::max()).string(),
		"creature-ffffffffffffffff");
	EXPECT_EQ(CreatureInstanceId::fromSerial(1U).string().size(), CreatureInstanceId::TextLength);
}

TEST(CreatureInstanceIdTest, RoundTripsThroughItsCanonicalText)
{
	for (const std::uint64_t serial :
		 {std::uint64_t{1},
		  std::uint64_t{0x2a},
		  std::uint64_t{1000000},
		  std::numeric_limits<std::uint64_t>::max()})
	{
		const CreatureInstanceId original = CreatureInstanceId::fromSerial(serial);
		const CreatureInstanceId parsed = CreatureInstanceId::parse(original.string());

		EXPECT_EQ(parsed, original);
		EXPECT_EQ(parsed.serial(), serial);
		EXPECT_EQ(parsed.string(), original.string());
		EXPECT_TRUE(parsed.valid());
	}
}

TEST(CreatureInstanceIdTest, RejectsAZeroSerial)
{
	EXPECT_THROW((void)CreatureInstanceId::fromSerial(0U), std::invalid_argument);
	EXPECT_THROW((void)CreatureInstanceId::parse("creature-0000000000000000"), std::invalid_argument);
}

TEST(CreatureInstanceIdTest, RejectsEveryNonCanonicalText)
{
	const std::string cases[] = {
		"",
		"creature-",
		"creature-1",                        // missing zero padding
		"creature-00000000000000001",        // one digit too many
		"creature-000000000000002A",         // upper case hex
		"CREATURE-000000000000002a",         // upper case prefix
		"creature-00000000000000-1",         // a sign is not a hex digit
		"creature-+000000000000001",         // nor is a plus
		"creature-00000000000000 1",         // nor is whitespace
		" creature-0000000000000001",        // leading whitespace
		"creature-0000000000000001 ",        // trailing whitespace
		"creature-000000000000000g",         // invalid hex digit
		"creature_0000000000000001",         // wrong separator
		"beast-0000000000000001",            // wrong prefix
		"creature-10000000000000000",        // would overflow uint64
		"0000000000000001"};                 // no prefix at all

	for (const std::string &text : cases)
	{
		EXPECT_THROW((void)CreatureInstanceId::parse(text), std::invalid_argument) << "text '" << text << "'";
	}
}

TEST(CreatureInstanceIdTest, OrdersBySerial)
{
	EXPECT_LT(CreatureInstanceId::fromSerial(1U), CreatureInstanceId::fromSerial(2U));
	EXPECT_EQ(CreatureInstanceId::fromSerial(42U), CreatureInstanceId::fromSerial(42U));
	EXPECT_NE(CreatureInstanceId::fromSerial(42U), CreatureInstanceId::fromSerial(43U));
	// The invalid id sorts before every real one.
	EXPECT_LT(CreatureInstanceId{}, CreatureInstanceId::fromSerial(1U));
}
