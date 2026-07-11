#include <gtest/gtest.h>

#include "structures/math/spk_deterministic_random.hpp"

TEST(DeterministicRandom, PreservesStableHashAlgorithms)
{
	EXPECT_EQ(spk::deterministic::fnv1a("coast-stair-length#0"), 0xb6dfcb319c295309ULL);
	EXPECT_EQ(spk::deterministic::deriveSeed(1234, "terrain/zones"), 0xd239281bd2e5d6b8ULL);
	EXPECT_EQ(spk::deterministic::avalanche(0x123456789abcdef0ULL), 0x9629f58e8ec5b906ULL);
}

TEST(DeterministicRandom, IncrementalHasherMatchesCompatibilityFunctions)
{
	spk::deterministic::StableHasher64 hasher;
	hasher.mix("cell");
	hasher.mix(-12);

	std::uint64_t expected = spk::deterministic::FnvOffset;
	spk::deterministic::mix(expected, "cell");
	spk::deterministic::mix(expected, -12);
	EXPECT_EQ(hasher.value(), expected);
	EXPECT_GE(spk::deterministic::unitInterval(expected), 0.0);
	EXPECT_LT(spk::deterministic::unitInterval(expected), 1.0);
}
