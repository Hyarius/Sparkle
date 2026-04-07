#include <gtest/gtest.h>

#include <sstream>
#include <string>

#include "spk_mouse.hpp"

TEST(MouseTest, DefaultConstructorInitializesAllButtonsToUp)
{
	const spk::Mouse mouse;

	for (std::size_t index = 0; index < spk::Mouse::NbButton; ++index)
	{
		EXPECT_EQ(mouse.buttons[index], spk::InputState::Up) << "Failed at index " << index;
	}
}

TEST(MouseTest, DefaultConstructorInitializesPositionToZero)
{
	const spk::Mouse mouse;

	EXPECT_EQ(mouse.position.x, 0);
	EXPECT_EQ(mouse.position.y, 0);
}

TEST(MouseTest, DefaultConstructorInitializesDeltaPositionToZero)
{
	const spk::Mouse mouse;

	EXPECT_EQ(mouse.deltaPosition.x, 0);
	EXPECT_EQ(mouse.deltaPosition.y, 0);
}

TEST(MouseTest, DefaultConstructorInitializesWheelToZero)
{
	const spk::Mouse mouse;

	EXPECT_FLOAT_EQ(mouse.wheel, 0.0f);
}

TEST(MouseTest, NonConstOperatorBracketReturnsMutableReference)
{
	spk::Mouse mouse;

	mouse[spk::Mouse::Left] = spk::InputState::Down;

	EXPECT_EQ(mouse[spk::Mouse::Left], spk::InputState::Down);
}

TEST(MouseTest, ConstOperatorBracketReturnsConstReference)
{
	spk::Mouse mouse;
	mouse[spk::Mouse::Right] = spk::InputState::Down;

	const spk::Mouse& constMouse = mouse;

	EXPECT_EQ(constMouse[spk::Mouse::Right], spk::InputState::Down);
}

TEST(MouseTest, ChangingOneButtonDoesNotChangeOtherButtons)
{
	spk::Mouse mouse;

	mouse[spk::Mouse::Middle] = spk::InputState::Down;

	EXPECT_EQ(mouse[spk::Mouse::Middle], spk::InputState::Down);
	EXPECT_EQ(mouse[spk::Mouse::Left], spk::InputState::Up);
	EXPECT_EQ(mouse[spk::Mouse::Right], spk::InputState::Up);
}

TEST(MouseTest, PositionCanBeAssigned)
{
	spk::Mouse mouse;

	mouse.position = spk::Vector2Int(42, -7);

	EXPECT_EQ(mouse.position.x, 42);
	EXPECT_EQ(mouse.position.y, -7);
}

TEST(MouseTest, DeltaPositionCanBeAssigned)
{
	spk::Mouse mouse;

	mouse.deltaPosition = spk::Vector2Int(-3, 9);

	EXPECT_EQ(mouse.deltaPosition.x, -3);
	EXPECT_EQ(mouse.deltaPosition.y, 9);
}

TEST(MouseTest, WheelCanBeAssigned)
{
	spk::Mouse mouse;

	mouse.wheel = 1.5f;

	EXPECT_FLOAT_EQ(mouse.wheel, 1.5f);
}

TEST(MouseTest, ToStringReturnsExpectedValueForButtons)
{
	EXPECT_EQ(spk::toString(spk::Mouse::Right), "Right");
	EXPECT_EQ(spk::toString(spk::Mouse::Middle), "Middle");
	EXPECT_EQ(spk::toString(spk::Mouse::Left), "Left");
}

TEST(MouseTest, ToStringReturnsFallbackForInvalidValue)
{
	const auto invalidButton = static_cast<spk::Mouse::Button>(999);

	EXPECT_EQ(spk::toString(invalidButton), "Unknow mouse button");
}

TEST(MouseTest, ToWstringReturnsExpectedValueForButtons)
{
	EXPECT_EQ(spk::toWstring(spk::Mouse::Right), L"Right");
	EXPECT_EQ(spk::toWstring(spk::Mouse::Middle), L"Middle");
	EXPECT_EQ(spk::toWstring(spk::Mouse::Left), L"Left");
}

TEST(MouseTest, ToWstringReturnsFallbackForInvalidValue)
{
	const auto invalidButton = static_cast<spk::Mouse::Button>(999);

	EXPECT_EQ(spk::toWstring(invalidButton), L"Unknow mouse button");
}

TEST(MouseTest, OstreamOperatorPrintsExpectedText)
{
	std::ostringstream outputStream;
	outputStream << spk::Mouse::Left;

	EXPECT_EQ(outputStream.str(), "Left");
}

TEST(MouseTest, WostreamOperatorPrintsExpectedText)
{
	std::wostringstream outputStream;
	outputStream << spk::Mouse::Left;

	EXPECT_EQ(outputStream.str(), L"Left");
}