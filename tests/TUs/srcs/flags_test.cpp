#include <cstdint>
#include <sstream>
#include <string>
#include <type_traits>

#include <gtest/gtest.h>

#include "spk_flags.hpp"

namespace
{
	enum class TestFlag8 : std::uint8_t
	{
		None = 0,
		A = 0b00000001,
		B = 0b00000010,
		C = 0b00000100,
		D = 0b00001000,
		AB = A | B,
		AC = A | C,
		All = A | B | C | D,
		InvA = 0b11111110
	};

	enum class TestFlag16 : std::uint16_t
	{
		None = 0,
		A = 1 << 0,
		B = 1 << 1,
		C = 1 << 2,
		D = 1 << 3,
		All = A | B | C | D
	};

	enum class TestFlag32 : std::uint32_t
	{
		None = 0,
		A = 1u << 0,
		B = 1u << 1,
		C = 1u << 2,
		D = 1u << 3,
		High = 1u << 31,
		AllLow = A | B | C | D
	};
}

TEST(spk_flags, defaultConstructor_ShouldBeEmpty)
{
	spk::Flags<TestFlag32> flags;

	EXPECT_TRUE(flags.none());
	EXPECT_FALSE(flags.any());
	EXPECT_FALSE(static_cast<bool>(flags));
	EXPECT_EQ(flags.raw(), 0u);
}

TEST(spk_flags, enumConstructor_ShouldInitializeWithFlagMask)
{
	spk::Flags<TestFlag32> flags(TestFlag32::B);

	EXPECT_TRUE(flags.any());
	EXPECT_TRUE(flags.has(TestFlag32::B));
	EXPECT_FALSE(flags.has(TestFlag32::A));
	EXPECT_EQ(flags.raw(), static_cast<std::uint32_t>(TestFlag32::B));
}

TEST(spk_flags, rawConstructor_ShouldInitializeWithRawMask)
{
	spk::Flags<TestFlag32> flags(0b1011u);

	EXPECT_EQ(flags.raw(), 0b1011u);
	EXPECT_TRUE(flags.has(TestFlag32::A));
	EXPECT_TRUE(flags.has(TestFlag32::B));
	EXPECT_FALSE(flags.has(TestFlag32::C));
	EXPECT_TRUE(flags.has(TestFlag32::D));
}

TEST(spk_flags, initializerListConstructor_ShouldCombineAllValues)
{
	spk::Flags<TestFlag32> flags({TestFlag32::A, TestFlag32::C, TestFlag32::D});

	EXPECT_EQ(flags.raw(), (1u << 0) | (1u << 2) | (1u << 3));
	EXPECT_TRUE(flags.has(TestFlag32::A));
	EXPECT_FALSE(flags.has(TestFlag32::B));
	EXPECT_TRUE(flags.has(TestFlag32::C));
	EXPECT_TRUE(flags.has(TestFlag32::D));
}

TEST(spk_flags, clear_ShouldRemoveAllFlags)
{
	spk::Flags<TestFlag32> flags({TestFlag32::A, TestFlag32::B, TestFlag32::C});

	flags.clear();

	EXPECT_TRUE(flags.none());
	EXPECT_EQ(flags.raw(), 0u);
}

TEST(spk_flags, setEnum_ShouldAddFlag)
{
	spk::Flags<TestFlag32> flags;

	flags.set(TestFlag32::C);

	EXPECT_TRUE(flags.has(TestFlag32::C));
	EXPECT_EQ(flags.raw(), static_cast<std::uint32_t>(TestFlag32::C));
}

TEST(spk_flags, setFlags_ShouldAddAllBitsFromOtherFlags)
{
	spk::Flags<TestFlag32> flags(TestFlag32::A);
	spk::Flags<TestFlag32> other({TestFlag32::B, TestFlag32::D});

	flags.set(other);

	EXPECT_TRUE(flags.has(TestFlag32::A));
	EXPECT_TRUE(flags.has(TestFlag32::B));
	EXPECT_FALSE(flags.has(TestFlag32::C));
	EXPECT_TRUE(flags.has(TestFlag32::D));
}

TEST(spk_flags, unsetEnum_ShouldRemoveFlag)
{
	spk::Flags<TestFlag32> flags({TestFlag32::A, TestFlag32::B, TestFlag32::C});

	flags.unset(TestFlag32::B);

	EXPECT_TRUE(flags.has(TestFlag32::A));
	EXPECT_FALSE(flags.has(TestFlag32::B));
	EXPECT_TRUE(flags.has(TestFlag32::C));
}

TEST(spk_flags, unsetFlags_ShouldRemoveAllBitsFromOtherFlags)
{
	spk::Flags<TestFlag32> flags({TestFlag32::A, TestFlag32::B, TestFlag32::C, TestFlag32::D});
	spk::Flags<TestFlag32> other({TestFlag32::B, TestFlag32::D});

	flags.unset(other);

	EXPECT_TRUE(flags.has(TestFlag32::A));
	EXPECT_FALSE(flags.has(TestFlag32::B));
	EXPECT_TRUE(flags.has(TestFlag32::C));
	EXPECT_FALSE(flags.has(TestFlag32::D));
}

TEST(spk_flags, resetEnum_ShouldBehaveLikeUnset)
{
	spk::Flags<TestFlag32> flags({TestFlag32::A, TestFlag32::C});

	flags.reset(TestFlag32::A);

	EXPECT_FALSE(flags.has(TestFlag32::A));
	EXPECT_TRUE(flags.has(TestFlag32::C));
}

TEST(spk_flags, resetFlags_ShouldBehaveLikeUnset)
{
	spk::Flags<TestFlag32> flags({TestFlag32::A, TestFlag32::B, TestFlag32::D});
	spk::Flags<TestFlag32> other({TestFlag32::A, TestFlag32::D});

	flags.reset(other);

	EXPECT_FALSE(flags.has(TestFlag32::A));
	EXPECT_TRUE(flags.has(TestFlag32::B));
	EXPECT_FALSE(flags.has(TestFlag32::D));
}

TEST(spk_flags, toggleEnum_ShouldFlipPresenceOfFlag)
{
	spk::Flags<TestFlag32> flags({TestFlag32::A, TestFlag32::C});

	flags.toggle(TestFlag32::A);
	flags.toggle(TestFlag32::B);

	EXPECT_FALSE(flags.has(TestFlag32::A));
	EXPECT_TRUE(flags.has(TestFlag32::B));
	EXPECT_TRUE(flags.has(TestFlag32::C));
}

TEST(spk_flags, toggleFlags_ShouldFlipAllBitsFromOtherFlags)
{
	spk::Flags<TestFlag32> flags({TestFlag32::A, TestFlag32::B});
	spk::Flags<TestFlag32> other({TestFlag32::B, TestFlag32::C});

	flags.toggle(other);

	EXPECT_TRUE(flags.has(TestFlag32::A));
	EXPECT_FALSE(flags.has(TestFlag32::B));
	EXPECT_TRUE(flags.has(TestFlag32::C));
}

TEST(spk_flags, hasEnum_ShouldRequireAllBitsOfGivenMask)
{
	spk::Flags<TestFlag8> flags({TestFlag8::A, TestFlag8::B, TestFlag8::D});

	EXPECT_TRUE(flags.has(TestFlag8::A));
	EXPECT_TRUE(flags.has(TestFlag8::AB));
	EXPECT_FALSE(flags.has(TestFlag8::AC));
	EXPECT_FALSE(flags.has(TestFlag8::C));
}

TEST(spk_flags, hasFlags_ShouldRequireAllBitsOfOtherFlags)
{
	spk::Flags<TestFlag32> flags({TestFlag32::A, TestFlag32::B, TestFlag32::D});

	EXPECT_TRUE(flags.has(spk::Flags<TestFlag32>({TestFlag32::A, TestFlag32::B})));
	EXPECT_FALSE(flags.has(spk::Flags<TestFlag32>({TestFlag32::A, TestFlag32::C})));
}

TEST(spk_flags, testAnyEnum_ShouldReturnTrueWhenAtLeastOneBitMatches)
{
	spk::Flags<TestFlag8> flags({TestFlag8::A, TestFlag8::D});

	EXPECT_TRUE(flags.testAny(TestFlag8::A));
	EXPECT_TRUE(flags.testAny(TestFlag8::AB));
	EXPECT_FALSE(flags.testAny(TestFlag8::B));
	EXPECT_FALSE(flags.testAny(TestFlag8::C));
}

TEST(spk_flags, testAnyFlags_ShouldReturnTrueWhenAtLeastOneBitMatches)
{
	spk::Flags<TestFlag32> flags({TestFlag32::A, TestFlag32::D});

	EXPECT_TRUE(flags.testAny(spk::Flags<TestFlag32>({TestFlag32::A, TestFlag32::B})));
	EXPECT_TRUE(flags.testAny(spk::Flags<TestFlag32>({TestFlag32::C, TestFlag32::D})));
	EXPECT_FALSE(flags.testAny(spk::Flags<TestFlag32>({TestFlag32::B, TestFlag32::C})));
}

TEST(spk_flags, boolOperator_ShouldReflectAnyState)
{
	spk::Flags<TestFlag32> emptyFlags;
	spk::Flags<TestFlag32> filledFlags(TestFlag32::C);

	EXPECT_FALSE(static_cast<bool>(emptyFlags));
	EXPECT_TRUE(static_cast<bool>(filledFlags));
}

TEST(spk_flags, orEqualsEnum_ShouldAddSingleFlag)
{
	spk::Flags<TestFlag32> flags(TestFlag32::A);

	flags |= TestFlag32::C;

	EXPECT_TRUE(flags.has(TestFlag32::A));
	EXPECT_TRUE(flags.has(TestFlag32::C));
}

TEST(spk_flags, andEqualsEnum_ShouldKeepOnlyBitsFromGivenMask)
{
	spk::Flags<TestFlag8> flags({TestFlag8::A, TestFlag8::B, TestFlag8::D});

	flags &= TestFlag8::AB;

	EXPECT_TRUE(flags.has(TestFlag8::A));
	EXPECT_TRUE(flags.has(TestFlag8::B));
	EXPECT_FALSE(flags.has(TestFlag8::D));
	EXPECT_EQ(flags.raw(), static_cast<std::uint8_t>(TestFlag8::AB));
}

TEST(spk_flags, xorEqualsEnum_ShouldToggleSingleFlag)
{
	spk::Flags<TestFlag32> flags({TestFlag32::A, TestFlag32::B});

	flags ^= TestFlag32::B;

	EXPECT_TRUE(flags.has(TestFlag32::A));
	EXPECT_FALSE(flags.has(TestFlag32::B));
}

TEST(spk_flags, orEqualsFlags_ShouldCombineMasks)
{
	spk::Flags<TestFlag32> left(TestFlag32::A);
	spk::Flags<TestFlag32> right({TestFlag32::B, TestFlag32::D});

	left |= right;

	EXPECT_TRUE(left.has(TestFlag32::A));
	EXPECT_TRUE(left.has(TestFlag32::B));
	EXPECT_TRUE(left.has(TestFlag32::D));
}

TEST(spk_flags, andEqualsFlags_ShouldKeepIntersection)
{
	spk::Flags<TestFlag32> left({TestFlag32::A, TestFlag32::B, TestFlag32::D});
	spk::Flags<TestFlag32> right({TestFlag32::B, TestFlag32::C, TestFlag32::D});

	left &= right;

	EXPECT_FALSE(left.has(TestFlag32::A));
	EXPECT_TRUE(left.has(TestFlag32::B));
	EXPECT_FALSE(left.has(TestFlag32::C));
	EXPECT_TRUE(left.has(TestFlag32::D));
}

TEST(spk_flags, xorEqualsFlags_ShouldToggleBitsFromOtherMask)
{
	spk::Flags<TestFlag32> left({TestFlag32::A, TestFlag32::B});
	spk::Flags<TestFlag32> right({TestFlag32::B, TestFlag32::C});

	left ^= right;

	EXPECT_TRUE(left.has(TestFlag32::A));
	EXPECT_FALSE(left.has(TestFlag32::B));
	EXPECT_TRUE(left.has(TestFlag32::C));
}

TEST(spk_flags, binaryOrBetweenFlags_ShouldReturnCombinedCopy)
{
	const spk::Flags<TestFlag32> left(TestFlag32::A);
	const spk::Flags<TestFlag32> right({TestFlag32::B, TestFlag32::D});

	const spk::Flags<TestFlag32> result = left | right;

	EXPECT_TRUE(result.has(TestFlag32::A));
	EXPECT_TRUE(result.has(TestFlag32::B));
	EXPECT_TRUE(result.has(TestFlag32::D));

	EXPECT_TRUE(left.has(TestFlag32::A));
	EXPECT_FALSE(left.has(TestFlag32::B));
}

TEST(spk_flags, binaryAndBetweenFlags_ShouldReturnIntersectionCopy)
{
	const spk::Flags<TestFlag32> left({TestFlag32::A, TestFlag32::B, TestFlag32::D});
	const spk::Flags<TestFlag32> right({TestFlag32::B, TestFlag32::C, TestFlag32::D});

	const spk::Flags<TestFlag32> result = left & right;

	EXPECT_FALSE(result.has(TestFlag32::A));
	EXPECT_TRUE(result.has(TestFlag32::B));
	EXPECT_FALSE(result.has(TestFlag32::C));
	EXPECT_TRUE(result.has(TestFlag32::D));
}

TEST(spk_flags, binaryXorBetweenFlags_ShouldReturnToggledCopy)
{
	const spk::Flags<TestFlag32> left({TestFlag32::A, TestFlag32::B});
	const spk::Flags<TestFlag32> right({TestFlag32::B, TestFlag32::C});

	const spk::Flags<TestFlag32> result = left ^ right;

	EXPECT_TRUE(result.has(TestFlag32::A));
	EXPECT_FALSE(result.has(TestFlag32::B));
	EXPECT_TRUE(result.has(TestFlag32::C));
}

TEST(spk_flags, binaryNot_ShouldInvertStoredBits)
{
	const spk::Flags<TestFlag8, std::uint8_t> flags(TestFlag8::A);

	const spk::Flags<TestFlag8, std::uint8_t> result = ~flags;

	EXPECT_EQ(
		result.raw(),
		static_cast<std::uint8_t>(TestFlag8::InvA));
}

TEST(spk_flags, mixedOrOperators_ShouldSupportFlagAndFlagsBothWays)
{
	const spk::Flags<TestFlag32> flags(TestFlag32::A);

	const spk::Flags<TestFlag32> leftResult = flags | TestFlag32::C;
	const spk::Flags<TestFlag32> rightResult = TestFlag32::C | flags;

	EXPECT_TRUE(leftResult.has(TestFlag32::A));
	EXPECT_TRUE(leftResult.has(TestFlag32::C));
	EXPECT_TRUE(rightResult.has(TestFlag32::A));
	EXPECT_TRUE(rightResult.has(TestFlag32::C));
}

TEST(spk_flags, mixedAndOperators_ShouldSupportFlagAndFlagsBothWays)
{
	const spk::Flags<TestFlag8> flags({TestFlag8::A, TestFlag8::B, TestFlag8::D});

	const spk::Flags<TestFlag8> leftResult = flags & TestFlag8::AB;
	const spk::Flags<TestFlag8> rightResult = TestFlag8::AB & flags;

	EXPECT_EQ(leftResult.raw(), static_cast<std::uint8_t>(TestFlag8::AB));
	EXPECT_EQ(rightResult.raw(), static_cast<std::uint8_t>(TestFlag8::AB));
}

TEST(spk_flags, mixedXorOperators_ShouldSupportFlagAndFlagsBothWays)
{
	const spk::Flags<TestFlag32> flags({TestFlag32::A, TestFlag32::B});

	const spk::Flags<TestFlag32> leftResult = flags ^ TestFlag32::B;
	const spk::Flags<TestFlag32> rightResult = TestFlag32::B ^ flags;

	EXPECT_TRUE(leftResult.has(TestFlag32::A));
	EXPECT_FALSE(leftResult.has(TestFlag32::B));

	EXPECT_TRUE(rightResult.has(TestFlag32::A));
	EXPECT_FALSE(rightResult.has(TestFlag32::B));
}

TEST(spk_flags, equalityBetweenFlags_ShouldCompareRawBits)
{
	const spk::Flags<TestFlag32> left({TestFlag32::A, TestFlag32::C});
	const spk::Flags<TestFlag32> same({TestFlag32::A, TestFlag32::C});
	const spk::Flags<TestFlag32> different({TestFlag32::A, TestFlag32::B});

	EXPECT_TRUE(left == same);
	EXPECT_FALSE(left != same);
	EXPECT_FALSE(left == different);
	EXPECT_TRUE(left != different);
}

TEST(spk_flags, equalityBetweenFlagsAndEnum_ShouldCompareAgainstSingleMask)
{
	const spk::Flags<TestFlag32> onlyA(TestFlag32::A);
	const spk::Flags<TestFlag32> aAndB({TestFlag32::A, TestFlag32::B});

	EXPECT_TRUE(onlyA == TestFlag32::A);
	EXPECT_TRUE(TestFlag32::A == onlyA);

	EXPECT_FALSE(aAndB == TestFlag32::A);
	EXPECT_FALSE(TestFlag32::A == aAndB);

	EXPECT_TRUE(aAndB != TestFlag32::A);
	EXPECT_TRUE(TestFlag32::A != aAndB);
}

TEST(spk_flags, aliases_ShouldUseRequestedStorageType)
{
	static_assert(std::is_same_v<typename spk::Flags8<TestFlag8>::MaskType, std::uint8_t>);
	static_assert(std::is_same_v<typename spk::Flags16<TestFlag16>::MaskType, std::uint16_t>);
	static_assert(std::is_same_v<typename spk::Flags32<TestFlag32>::MaskType, std::uint32_t>);

	EXPECT_TRUE((std::is_same_v<typename spk::Flags8<TestFlag8>::MaskType, std::uint8_t>));
	EXPECT_TRUE((std::is_same_v<typename spk::Flags16<TestFlag16>::MaskType, std::uint16_t>));
	EXPECT_TRUE((std::is_same_v<typename spk::Flags32<TestFlag32>::MaskType, std::uint32_t>));
}

TEST(spk_flags, shouldSupportHighBitWithinStorage)
{
	spk::Flags<TestFlag32> flags;

	flags.set(TestFlag32::High);

	EXPECT_TRUE(flags.has(TestFlag32::High));
	EXPECT_EQ(flags.raw(), 0x80000000u);
}

TEST(spk_flags, ostreamOperator_ShouldPrintHexValue)
{
	const spk::Flags<TestFlag32> flags({TestFlag32::A, TestFlag32::B});

	std::ostringstream outputStream;
	outputStream << flags;

	EXPECT_EQ(outputStream.str(), "0x3");
}

TEST(spk_flags, ostreamOperator_ShouldRestoreStreamFormattingFlags)
{
	const spk::Flags<TestFlag32> flags({TestFlag32::A, TestFlag32::B});

	std::ostringstream outputStream;
	outputStream << flags << ' ' << 42;

	EXPECT_EQ(outputStream.str(), "0x3 42");
}

TEST(spk_flags, wostreamOperator_ShouldPrintHexValue)
{
	const spk::Flags<TestFlag32> flags({TestFlag32::A, TestFlag32::C});

	std::wostringstream outputStream;
	outputStream << flags;

	EXPECT_EQ(outputStream.str(), L"0x5");
}

TEST(spk_flags, wostreamOperator_ShouldRestoreStreamFormattingFlags)
{
	const spk::Flags<TestFlag32> flags({TestFlag32::A, TestFlag32::C});

	std::wostringstream outputStream;
	outputStream << flags << L' ' << 42;

	EXPECT_EQ(outputStream.str(), L"0x5 42");
}

TEST(spk_flags, rawConstructorAndComparison_ShouldWorkWithZeroMask)
{
	const spk::Flags<TestFlag32> flags(0u);

	EXPECT_TRUE(flags.none());
	EXPECT_FALSE(flags.any());
	EXPECT_EQ(flags.raw(), 0u);
	EXPECT_FALSE(flags == TestFlag32::A);
	EXPECT_TRUE(flags != TestFlag32::A);
}