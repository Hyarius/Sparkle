#include <gtest/gtest.h>

#include <sstream>
#include <string>

#include "spk_input_state.hpp"

TEST(InputStateTest, ToStringUpReturnsUp)
{
	EXPECT_EQ(spk::toString(spk::InputState::Up), "Up");
}

TEST(InputStateTest, ToStringDownReturnsDown)
{
	EXPECT_EQ(spk::toString(spk::InputState::Down), "Down");
}

TEST(InputStateTest, ToStringUnknownValueReturnsFallbackMessage)
{
	const auto invalidValue = static_cast<spk::InputState>(999);

	EXPECT_EQ(spk::toString(invalidValue), "Unknow InputState");
}

TEST(InputStateTest, ToWstringUpReturnsUp)
{
	EXPECT_EQ(spk::toWstring(spk::InputState::Up), L"Up");
}

TEST(InputStateTest, ToWstringDownReturnsDown)
{
	EXPECT_EQ(spk::toWstring(spk::InputState::Down), L"Down");
}

TEST(InputStateTest, ToWstringUnknownValueReturnsFallbackMessage)
{
	const auto invalidValue = static_cast<spk::InputState>(999);

	EXPECT_EQ(spk::toWstring(invalidValue), L"Unknow InputState");
}

TEST(InputStateTest, OstreamOperatorPrintsUp)
{
	std::ostringstream outputStream;
	outputStream << spk::InputState::Up;

	EXPECT_EQ(outputStream.str(), "Up");
}

TEST(InputStateTest, OstreamOperatorPrintsDown)
{
	std::ostringstream outputStream;
	outputStream << spk::InputState::Down;

	EXPECT_EQ(outputStream.str(), "Down");
}

TEST(InputStateTest, WostreamOperatorPrintsUp)
{
	std::wostringstream outputStream;
	outputStream << spk::InputState::Up;

	EXPECT_EQ(outputStream.str(), L"Up");
}

TEST(InputStateTest, WostreamOperatorPrintsDown)
{
	std::wostringstream outputStream;
	outputStream << spk::InputState::Down;

	EXPECT_EQ(outputStream.str(), L"Down");
}