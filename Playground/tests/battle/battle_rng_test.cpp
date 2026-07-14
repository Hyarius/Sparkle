#include <gtest/gtest.h>

#include "battle/battle_rng.hpp"
#include "core/deterministic_random.hpp"

#include <array>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
	// SplitMix64 reference vectors. These are the algorithm's published outputs, not a
	// capture of our own code: if an edit changes them, every archived battle replays
	// differently, which is exactly what this test exists to prevent.
	constexpr std::array<std::uint64_t, 5> GoldenSeedZero{
		0xe220a8397b1dcdafULL,
		0x6e789e6aa1b965f4ULL,
		0x06c45d188009454fULL,
		0xf88bb8a8724c81ecULL,
		0x1b39896a51a8749bULL};

	constexpr std::array<std::uint64_t, 5> GoldenSeedSequential{
		0x157a3807a48faa9dULL,
		0xd573529b34a1d093ULL,
		0x2f90b72e996dccbeULL,
		0xa2d419334c4667ecULL,
		0x01404ce914938008ULL};

	constexpr std::uint64_t SequentialSeed = 0x0123456789abcdefULL;
}

TEST(BattleRngTest, ProducesTheGoldenSplitMix64Sequence)
{
	pg::BattleRng zeroSeeded(0ULL);
	for (std::size_t index = 0; index < GoldenSeedZero.size(); ++index)
	{
		EXPECT_EQ(zeroSeeded.nextU64(), GoldenSeedZero[index]) << "draw " << index << " of seed 0";
	}
	EXPECT_EQ(zeroSeeded.drawCount(), GoldenSeedZero.size());

	pg::BattleRng sequentialSeeded(SequentialSeed);
	for (std::size_t index = 0; index < GoldenSeedSequential.size(); ++index)
	{
		EXPECT_EQ(sequentialSeeded.nextU64(), GoldenSeedSequential[index]) << "draw " << index;
	}
	EXPECT_EQ(sequentialSeeded.drawCount(), GoldenSeedSequential.size());
}

TEST(BattleRngTest, SameSeedGivesTheSameStreamAndDrawCount)
{
	pg::BattleRng first(SequentialSeed);
	pg::BattleRng second(SequentialSeed);

	for (int index = 0; index < 32; ++index)
	{
		EXPECT_EQ(first.nextU64(), second.nextU64());
	}
	EXPECT_EQ(first.drawCount(), second.drawCount());
	EXPECT_EQ(first.state(), second.state());
}

TEST(BattleRngTest, DifferentSeedsDiverge)
{
	pg::BattleRng first(SequentialSeed);
	pg::BattleRng second(SequentialSeed + 1U);

	bool diverged = false;
	for (int index = 0; index < 8 && !diverged; ++index)
	{
		diverged = (first.nextU64() != second.nextU64());
	}
	EXPECT_TRUE(diverged);
}

TEST(BattleRngTest, DrawCountCountsEveryActualDrawIncludingRejections)
{
	pg::BattleRng generator(SequentialSeed);
	EXPECT_EQ(generator.drawCount(), 0U);

	(void)generator.nextU64();
	EXPECT_EQ(generator.drawCount(), 1U);

	// uniformBelow consumes at least one draw and one more per rejection; the count is a
	// stream cursor, not a count of accepted results.
	const std::size_t before = generator.drawCount();
	(void)generator.uniformBelow(6U);
	EXPECT_GE(generator.drawCount(), before + 1U);
}

TEST(BattleRngTest, UniformBelowOneIsAlwaysZero)
{
	pg::BattleRng generator(SequentialSeed);
	for (int index = 0; index < 16; ++index)
	{
		EXPECT_EQ(generator.uniformBelow(1U), 0U);
	}
}

TEST(BattleRngTest, UniformBelowZeroFailsWithoutAdvancingTheGenerator)
{
	pg::BattleRng generator(SequentialSeed);
	const std::uint64_t stateBefore = generator.state();

	EXPECT_THROW((void)generator.uniformBelow(0U), std::invalid_argument);
	EXPECT_EQ(generator.drawCount(), 0U);
	EXPECT_EQ(generator.state(), stateBefore);

	// And the stream still starts where it would have.
	EXPECT_EQ(generator.nextU64(), GoldenSeedSequential[0]);
}

TEST(BattleRngTest, BoundedDrawsStayInsideTheirDomain)
{
	pg::BattleRng generator(SequentialSeed);
	constexpr std::uint64_t bound = 6U;

	for (int index = 0; index < 256; ++index)
	{
		const std::uint64_t value = generator.uniformBelow(bound);
		ASSERT_LT(value, bound);
	}
}

TEST(BattleRngTest, BoundedDrawsAreReproducibleAcrossIndependentGenerators)
{
	pg::BattleRng first(0ULL);
	pg::BattleRng second(0ULL);

	std::vector<std::uint64_t> firstValues;
	std::vector<std::uint64_t> secondValues;
	for (int index = 0; index < 16; ++index)
	{
		firstValues.push_back(first.uniformBelow(100U));
		secondValues.push_back(second.uniformBelow(100U));
	}
	EXPECT_EQ(firstValues, secondValues);
	EXPECT_EQ(first.drawCount(), second.drawCount());
}

TEST(BattleRngTest, RejectionSamplingDiscardsTheBiasedPrefix)
{
	// With this bound only one value of the 2^64 domain is rejected, but the arithmetic that
	// finds it is the same arithmetic every bound uses: 2^64 mod bound values are dropped.
	constexpr std::uint64_t bound = 3U;
	const std::uint64_t rejectedPrefix = (0xffffffffffffffffULL - bound + 1U) % bound;
	EXPECT_EQ(rejectedPrefix, 1U);

	pg::BattleRng generator(SequentialSeed);
	const std::uint64_t value = generator.uniformBelow(bound);
	EXPECT_LT(value, bound);
	// The first golden draw is far above the rejected prefix, so exactly one draw happened.
	EXPECT_EQ(generator.drawCount(), 1U);
	EXPECT_EQ(value, GoldenSeedSequential[0] % bound);
}

TEST(BattleRngTest, ChildSeedsSeparateEncounterResolutionFromCombat)
{
	constexpr std::uint64_t worldSeed = 0x7a11c0ffeeULL;
	const std::string identity = "route-3/cell/12:34/ordinal/2";

	// Step 12 owns this derivation; the shape is pinned here so a later step cannot quietly
	// re-derive a second "battle/" seed or fold the two domains together.
	const std::uint64_t encounterResolutionSeed =
		pg::deterministic::deriveSeed(worldSeed, identity + "/encounter-resolution");
	const std::uint64_t combatSeed = pg::deterministic::deriveSeed(worldSeed, identity + "/combat");

	EXPECT_EQ(encounterResolutionSeed, 0xd46c5cdfd56d1619ULL);
	EXPECT_EQ(combatSeed, 0xe962b6c86ff38c8fULL);
	EXPECT_NE(encounterResolutionSeed, combatSeed);

	// Identical across independent derivations, and sensitive to every input.
	EXPECT_EQ(combatSeed, pg::deterministic::deriveSeed(worldSeed, identity + "/combat"));
	EXPECT_NE(combatSeed, pg::deterministic::deriveSeed(worldSeed + 1U, identity + "/combat"));
	EXPECT_NE(combatSeed, pg::deterministic::deriveSeed(worldSeed, "route-3/cell/12:35/ordinal/2/combat"));

	// The session seeds its generator with the combat seed exactly as received: no second
	// derivation, so its first draw is a pure function of the seed.
	pg::BattleRng sessionRng(combatSeed);
	pg::BattleRng replayRng(combatSeed);
	EXPECT_EQ(sessionRng.nextU64(), replayRng.nextU64());
}
