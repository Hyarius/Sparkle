#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <string_view>

#include "structures/math/spk_deterministic_random.hpp"

namespace
{
	constexpr std::uint64_t structuredUInt32Zero()
	{
		spk::deterministic::StableHasher64 hasher;
		hasher.mix(std::uint32_t{0});
		return hasher.value();
	}

	constexpr std::uint64_t structuredInt32Minimum()
	{
		spk::deterministic::StableHasher64 hasher;
		hasher.mix(std::numeric_limits<std::int32_t>::min());
		return hasher.value();
	}
}

static_assert(spk::deterministic::fnv1a64::hash("") == 0xcbf29ce484222325ULL);
static_assert(spk::deterministic::fnv1a64::hash("foobar") == 0x85944171f73967e8ULL);
static_assert(structuredUInt32Zero() == 0xc76abe129076aff1ULL);
static_assert(structuredInt32Minimum() == 0xe196009b1a7efa36ULL);
static_assert(spk::deterministic::splitMix64Finalize(1) == 0x5692161d100b05e5ULL);
static_assert(spk::deterministic::toUnitInterval(0) == 0.0);

TEST(DeterministicRandomTest, Fnv1a64MatchesKnownByteVectors)
{
	EXPECT_EQ(spk::deterministic::fnv1a64::hash(""), 0xcbf29ce484222325ULL);
	EXPECT_EQ(spk::deterministic::fnv1a64::hash("a"), 0xaf63dc4c8601ec8cULL);
	EXPECT_EQ(spk::deterministic::fnv1a64::hash("foobar"), 0x85944171f73967e8ULL);
	EXPECT_EQ(spk::deterministic::fnv1a64::hash("hello"), 0xa430d84680aabd0bULL);
	EXPECT_EQ(spk::deterministic::fnv1a64::hash(std::string_view{"\0\xff", 2}), 0x0831c907b4ea2b60ULL);
}

TEST(DeterministicRandomTest, StructuredMixUsesTaggedLittleEndianValues)
{
	spk::deterministic::StableHasher64 uint32Zero;
	uint32Zero.mix(std::uint32_t{0});
	EXPECT_EQ(uint32Zero.value(), 0xc76abe129076aff1ULL);

	spk::deterministic::StableHasher64 uint32Maximum;
	uint32Maximum.mix(std::numeric_limits<std::uint32_t>::max());
	EXPECT_EQ(uint32Maximum.value(), 0x420c6a0f4e5dfb4dULL);

	spk::deterministic::StableHasher64 negativeInt32;
	negativeInt32.mix(std::int32_t{-12});
	EXPECT_EQ(negativeInt32.value(), 0x451a9116e77bb209ULL);

	spk::deterministic::StableHasher64 int64Minimum;
	int64Minimum.mix(std::numeric_limits<std::int64_t>::min());
	EXPECT_EQ(int64Minimum.value(), 0x3bb627664a666fe8ULL);

	spk::deterministic::StableHasher64 uint64Maximum;
	uint64Maximum.mix(std::numeric_limits<std::uint64_t>::max());
	EXPECT_EQ(uint64Maximum.value(), 0xdd9545281a91a253ULL);

	spk::deterministic::StableHasher64 fourNullBytes;
	fourNullBytes.mix(std::string_view{"\0\0\0\0", 4});
	EXPECT_EQ(fourNullBytes.value(), 0x1796aea3ab37b1a8ULL);
	EXPECT_NE(fourNullBytes.value(), uint32Zero.value());
}

TEST(DeterministicRandomTest, StructuredMixPreservesValueBoundaries)
{
	spk::deterministic::StableHasher64 abThenC;
	abThenC.mix("ab");
	abThenC.mix("c");
	EXPECT_EQ(abThenC.value(), 0x66351a77b9ec3432ULL);

	spk::deterministic::StableHasher64 aThenBc;
	aThenBc.mix("a");
	aThenBc.mix("bc");
	EXPECT_EQ(aThenBc.value(), 0x52b8f01ec614876aULL);
	EXPECT_NE(abThenC.value(), aThenBc.value());

	spk::deterministic::StableHasher64 emptyThenAbc;
	emptyThenAbc.mix("");
	emptyThenAbc.mix("abc");
	EXPECT_EQ(emptyThenAbc.value(), 0x3bcf28ad5dccae5cULL);

	spk::deterministic::StableHasher64 abcThenEmpty;
	abcThenEmpty.mix("abc");
	abcThenEmpty.mix("");
	EXPECT_EQ(abcThenEmpty.value(), 0x07449920e4a0d200ULL);
	EXPECT_NE(emptyThenAbc.value(), abcThenEmpty.value());
}

TEST(DeterministicRandomTest, DeriveSeedUsesItsVersionedDomain)
{
	EXPECT_EQ(spk::deterministic::deriveSeed(0, ""), 0x13ac663e1623cc5eULL);
	EXPECT_EQ(spk::deterministic::deriveSeed(1234, "terrain/zones"), 0xbaa45cb1b64f8403ULL);
	EXPECT_EQ(
		spk::deterministic::deriveSeed(std::numeric_limits<std::uint64_t>::max(), "a:b"),
		0xa860153a0b1dd927ULL);
	EXPECT_EQ(spk::deterministic::deriveSeed(1234, std::string_view{"a\0b", 3}), 0x611db2f91b82042bULL);
	EXPECT_EQ(
		spk::deterministic::deriveSeed(1234, "terrain/zones"),
		spk::deterministic::deriveSeed(1234, "terrain/zones"));
	EXPECT_NE(spk::deterministic::deriveSeed(1234, "terrain/zones"), spk::deterministic::deriveSeed(1235, "terrain/zones"));
	EXPECT_NE(spk::deterministic::deriveSeed(1234, "terrain/zones"), spk::deterministic::deriveSeed(1234, "terrain/biomes"));

	spk::deterministic::StableHasher64 ordinaryHasher;
	ordinaryHasher.mix("spk.derive-seed.v1");
	ordinaryHasher.mix(std::uint64_t{0});
	ordinaryHasher.mix("");
	EXPECT_NE(spk::deterministic::deriveSeed(0, ""), ordinaryHasher.value());
}

TEST(DeterministicRandomTest, SplitMix64FinalizerMatchesGoldenValues)
{
	EXPECT_EQ(spk::deterministic::splitMix64Finalize(0), 0ULL);
	EXPECT_EQ(spk::deterministic::splitMix64Finalize(1), 0x5692161d100b05e5ULL);
	EXPECT_EQ(
		spk::deterministic::splitMix64Finalize(std::numeric_limits<std::uint64_t>::max()),
		0xb4d055fcf2cbbd7bULL);
	EXPECT_EQ(spk::deterministic::splitMix64Finalize(0x123456789abcdef0ULL), 0x9629f58e8ec5b906ULL);
}

TEST(DeterministicRandomTest, ToUnitIntervalUsesOnlyTheUpper53Bits)
{
	EXPECT_EQ(spk::deterministic::toUnitInterval(0), 0.0);
	EXPECT_EQ(
		spk::deterministic::toUnitInterval(std::numeric_limits<std::uint64_t>::max()),
		0x1.fffffffffffffp-1);
	EXPECT_LT(spk::deterministic::toUnitInterval(std::numeric_limits<std::uint64_t>::max()), 1.0);

	const std::uint64_t retainedBits = 0x123456789abcdef0ULL;
	EXPECT_EQ(spk::deterministic::toUnitInterval(retainedBits), spk::deterministic::toUnitInterval(retainedBits | 0x7ffULL));
	EXPECT_NE(spk::deterministic::toUnitInterval(retainedBits), spk::deterministic::toUnitInterval(retainedBits + (1ULL << 11U)));
}
