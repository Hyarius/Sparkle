#include <gtest/gtest.h>

#include "battle/battle_time.hpp"
#include "core/json.hpp"

#include <cmath>
#include <limits>
#include <stdexcept>

namespace
{
	using pg::BattleTime;

	constexpr BattleTime::Rep TickMinimum = std::numeric_limits<BattleTime::Rep>::min();
	constexpr BattleTime::Rep TickMaximum = std::numeric_limits<BattleTime::Rep>::max();

	pg::JsonReader readerFor(const spk::JSON::Value &p_value)
	{
		return pg::JsonReader(p_value, "fixture.json");
	}
}

TEST(BattleTimeTest, DefaultIsAValidZeroDurationNotASentinel)
{
	constexpr BattleTime value;
	static_assert(value.ticks() == 0);
	static_assert(value.isZero());
	EXPECT_EQ(value, BattleTime::fromTicks(0));
	EXPECT_DOUBLE_EQ(value.secondsForDisplay(), 0.0);
}

TEST(BattleTimeTest, OneTickIsOneMillisecond)
{
	static_assert(BattleTime::TicksPerSecond == 1000);
	EXPECT_EQ(BattleTime::fromSecondsExact(1.0).ticks(), 1000);
}

TEST(BattleTimeTest, ConvertsExactlyRepresentableSeconds)
{
	EXPECT_EQ(BattleTime::fromSecondsExact(4).ticks(), 4000);
	EXPECT_EQ(BattleTime::fromSecondsExact(4.0).ticks(), 4000);
	EXPECT_EQ(BattleTime::fromSecondsExact(0.1).ticks(), 100);
	EXPECT_EQ(BattleTime::fromSecondsExact(0.125).ticks(), 125);
	EXPECT_EQ(BattleTime::fromSecondsExact(-0.250).ticks(), -250);
	EXPECT_EQ(BattleTime::fromSecondsExact(0.0).ticks(), 0);
	EXPECT_EQ(BattleTime::fromSecondsExact(-0.0).ticks(), 0);
	EXPECT_EQ(BattleTime::fromSecondsExact(0.001).ticks(), 1);
}

TEST(BattleTimeTest, RejectsSubMillisecondPrecisionInsteadOfRounding)
{
	EXPECT_THROW((void)BattleTime::fromSecondsExact(0.0005), std::invalid_argument);
	EXPECT_THROW((void)BattleTime::fromSecondsExact(0.1234), std::invalid_argument);
	EXPECT_THROW((void)BattleTime::fromSecondsExact(-0.0001), std::invalid_argument);
}

TEST(BattleTimeTest, RejectsNonFiniteValues)
{
	EXPECT_THROW((void)BattleTime::fromSecondsExact(std::nan("")), std::invalid_argument);
	EXPECT_THROW(
		(void)BattleTime::fromSecondsExact(std::numeric_limits<double>::infinity()), std::invalid_argument);
	EXPECT_THROW(
		(void)BattleTime::fromSecondsExact(-std::numeric_limits<double>::infinity()), std::invalid_argument);
}

TEST(BattleTimeTest, RejectsValuesOutsideTheTickRange)
{
	EXPECT_THROW((void)BattleTime::fromSecondsExact(1e18), std::overflow_error);
	EXPECT_THROW((void)BattleTime::fromSecondsExact(-1e18), std::overflow_error);
	EXPECT_THROW((void)BattleTime::fromSecondsExact(std::numeric_limits<double>::max()), std::overflow_error);
	// 2^63 milliseconds is the first value above the range; 2^62 is comfortably inside it.
	EXPECT_NO_THROW((void)BattleTime::fromSecondsExact(4611686018427387.904));
}

TEST(BattleTimeTest, OrdersAndComparesByTicks)
{
	EXPECT_TRUE(BattleTime::fromTicks(100) == BattleTime::fromSecondsExact(0.1));
	EXPECT_TRUE(BattleTime::fromTicks(99) < BattleTime::fromTicks(100));
	EXPECT_TRUE(BattleTime::fromTicks(-1) < BattleTime{});
	EXPECT_TRUE(BattleTime::fromTicks(101) >= BattleTime::fromTicks(101));
	EXPECT_NE(BattleTime::fromTicks(1), BattleTime::fromTicks(2));
}

TEST(BattleTimeTest, AdditionAndSubtractionAreChecked)
{
	EXPECT_EQ((BattleTime::fromTicks(400) + BattleTime::fromTicks(350)).ticks(), 750);
	EXPECT_EQ((BattleTime::fromTicks(400) - BattleTime::fromTicks(350)).ticks(), 50);
	EXPECT_EQ((BattleTime::fromTicks(-400) + BattleTime::fromTicks(350)).ticks(), -50);

	BattleTime accumulator = BattleTime::fromTicks(1000);
	accumulator += BattleTime::fromTicks(250);
	accumulator -= BattleTime::fromTicks(50);
	EXPECT_EQ(accumulator.ticks(), 1200);

	EXPECT_THROW(
		(void)(BattleTime::fromTicks(TickMaximum) + BattleTime::fromTicks(1)), std::overflow_error);
	EXPECT_THROW(
		(void)(BattleTime::fromTicks(TickMinimum) + BattleTime::fromTicks(-1)), std::overflow_error);
	EXPECT_THROW(
		(void)(BattleTime::fromTicks(TickMinimum) - BattleTime::fromTicks(1)), std::overflow_error);
	EXPECT_THROW(
		(void)(BattleTime::fromTicks(TickMaximum) - BattleTime::fromTicks(-1)), std::overflow_error);
	EXPECT_NO_THROW((void)(BattleTime::fromTicks(TickMaximum) + BattleTime::fromTicks(0)));
	EXPECT_NO_THROW((void)(BattleTime::fromTicks(TickMinimum) - BattleTime::fromTicks(0)));
}

TEST(BattleTimeTest, NegationIsCheckedAtTheAsymmetricEdge)
{
	EXPECT_EQ((-BattleTime::fromTicks(250)).ticks(), -250);
	EXPECT_EQ((-BattleTime::fromTicks(TickMaximum)).ticks(), -TickMaximum);
	// Two's complement has no positive counterpart for the minimum tick value.
	EXPECT_THROW((void)(-BattleTime::fromTicks(TickMinimum)), std::overflow_error);
}

TEST(BattleTimeTest, MultiplicationByAnIntegerIsChecked)
{
	EXPECT_EQ((BattleTime::fromTicks(250) * 4).ticks(), 1000);
	EXPECT_EQ((BattleTime::fromTicks(250) * -4).ticks(), -1000);
	EXPECT_EQ((BattleTime::fromTicks(-250) * -4).ticks(), 1000);
	EXPECT_EQ((BattleTime::fromTicks(TickMaximum) * 0).ticks(), 0);
	EXPECT_EQ((BattleTime::fromTicks(TickMaximum) * 1).ticks(), TickMaximum);

	BattleTime total = BattleTime::fromTicks(125);
	total *= 8;
	EXPECT_EQ(total.ticks(), 1000);

	EXPECT_THROW((void)(BattleTime::fromTicks(TickMaximum) * 2), std::overflow_error);
	EXPECT_THROW((void)(BattleTime::fromTicks(TickMinimum) * 2), std::overflow_error);
	EXPECT_THROW((void)(BattleTime::fromTicks(TickMinimum) * -1), std::overflow_error);
	EXPECT_THROW((void)(BattleTime::fromTicks(-2) * TickMinimum), std::overflow_error);
	EXPECT_THROW((void)(BattleTime::fromTicks(TickMaximum / 2 + 1) * 2), std::overflow_error);
	EXPECT_NO_THROW((void)(BattleTime::fromTicks(TickMaximum / 2) * 2));
}

TEST(BattleTimeTest, ClampIsAnExplicitSaturatingFreeFunction)
{
	const BattleTime low = BattleTime::fromTicks(0);
	const BattleTime high = BattleTime::fromTicks(1000);

	EXPECT_EQ(pg::clamp(BattleTime::fromTicks(-5), low, high).ticks(), 0);
	EXPECT_EQ(pg::clamp(BattleTime::fromTicks(400), low, high).ticks(), 400);
	EXPECT_EQ(pg::clamp(BattleTime::fromTicks(5000), low, high).ticks(), 1000);
	EXPECT_EQ(pg::clamp(low, low, high).ticks(), 0);
	EXPECT_EQ(pg::clamp(high, low, high).ticks(), 1000);
	EXPECT_THROW((void)pg::clamp(low, high, low), std::invalid_argument);
}

TEST(BattleTimeTest, DisplayConversionsNeverMutateTicks)
{
	const BattleTime value = BattleTime::fromTicks(1250);
	EXPECT_DOUBLE_EQ(value.secondsForDisplay(), 1.25);
	EXPECT_DOUBLE_EQ(pg::ratioForDisplay(value, BattleTime::fromTicks(5000)), 0.25);
	// A zero total is a display state, not a division by zero.
	EXPECT_DOUBLE_EQ(pg::ratioForDisplay(value, BattleTime{}), 0.0);
	EXPECT_EQ(value.ticks(), 1250);
}

TEST(BattleTimeTest, JsonPositivePolicyAcceptsOnlyStrictlyPositiveDurations)
{
	const spk::JSON::Value json = spk::JSON::Value::fromString(R"({"stamina": 0.1})");
	pg::JsonReader reader = readerFor(json);
	EXPECT_EQ(pg::requireBattleSeconds(reader, "stamina", pg::BattleTimeSign::Positive).ticks(), 100);

	const spk::JSON::Value zeroJson = spk::JSON::Value::fromString(R"({"stamina": 0})");
	pg::JsonReader zeroReader = readerFor(zeroJson);
	EXPECT_THROW(
		(void)pg::requireBattleSeconds(zeroReader, "stamina", pg::BattleTimeSign::Positive), pg::JsonError);
}

TEST(BattleTimeTest, JsonNonNegativeAndAnyPoliciesAcceptTheirOwnRanges)
{
	const spk::JSON::Value json = spk::JSON::Value::fromString(R"({"delay": 0, "delta": -0.25})");
	pg::JsonReader reader = readerFor(json);

	EXPECT_EQ(pg::requireBattleSeconds(reader, "delay", pg::BattleTimeSign::NonNegative).ticks(), 0);
	EXPECT_EQ(pg::requireBattleSeconds(reader, "delta", pg::BattleTimeSign::Any).ticks(), -250);
	EXPECT_THROW(
		(void)pg::requireBattleSeconds(reader, "delta", pg::BattleTimeSign::NonNegative), pg::JsonError);
}

TEST(BattleTimeTest, JsonErrorsCarryTheFileAndTheExactPath)
{
	const spk::JSON::Value json = spk::JSON::Value::fromString(R"({"effect": {"duration": 0.0005}})");
	pg::JsonReader reader = readerFor(json);
	pg::JsonReader effectReader = reader.child("effect");

	try
	{
		(void)pg::requireBattleSeconds(effectReader, "duration", pg::BattleTimeSign::Positive);
		FAIL() << "a sub-millisecond duration must be rejected";
	} catch (const pg::JsonError &error)
	{
		EXPECT_EQ(error.file(), std::filesystem::path("fixture.json"));
		EXPECT_EQ(error.path(), "$.effect.duration");
		EXPECT_NE(error.message().find("millisecond"), std::string::npos);
	}
}

TEST(BattleTimeTest, JsonRejectsNonNumbersWithContext)
{
	const spk::JSON::Value json = spk::JSON::Value::fromString(R"({"duration": "fast"})");
	pg::JsonReader reader = readerFor(json);

	try
	{
		(void)pg::requireBattleSeconds(reader, "duration", pg::BattleTimeSign::Positive);
		FAIL() << "a string duration must be rejected";
	} catch (const pg::JsonError &error)
	{
		EXPECT_EQ(error.path(), "$.duration");
	}
}
