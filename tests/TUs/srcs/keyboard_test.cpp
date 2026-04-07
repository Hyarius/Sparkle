#include <gtest/gtest.h>

#include <sstream>
#include <string>

#include "spk_keyboard.hpp"

TEST(KeyboardTest, DefaultConstructorInitializesAllKeysToUp)
{
	const spk::Keyboard keyboard;

	for (std::size_t index = 0; index < spk::Keyboard::NbKey; ++index)
	{
		EXPECT_EQ(keyboard.keys[index], spk::InputState::Up) << "Failed at index " << index;
	}
}

TEST(KeyboardTest, DefaultConstructorInitializesGlyphToNullCharacter)
{
	const spk::Keyboard keyboard;

	EXPECT_EQ(keyboard.glyph, U'\0');
}

TEST(KeyboardTest, NonConstOperatorBracketReturnsMutableReference)
{
	spk::Keyboard keyboard;

	keyboard[spk::Keyboard::A] = spk::InputState::Down;

	EXPECT_EQ(keyboard[spk::Keyboard::A], spk::InputState::Down);
}

TEST(KeyboardTest, ConstOperatorBracketReturnsConstReference)
{
	spk::Keyboard keyboard;
	keyboard[spk::Keyboard::Return] = spk::InputState::Down;

	const spk::Keyboard& constKeyboard = keyboard;

	EXPECT_EQ(constKeyboard[spk::Keyboard::Return], spk::InputState::Down);
}

TEST(KeyboardTest, ChangingOneKeyDoesNotChangeOtherKeys)
{
	spk::Keyboard keyboard;

	keyboard[spk::Keyboard::A] = spk::InputState::Down;

	EXPECT_EQ(keyboard[spk::Keyboard::A], spk::InputState::Down);
	EXPECT_EQ(keyboard[spk::Keyboard::B], spk::InputState::Up);
	EXPECT_EQ(keyboard[spk::Keyboard::Escape], spk::InputState::Up);
}

TEST(KeyboardTest, GlyphCanBeAssigned)
{
	spk::Keyboard keyboard;

	keyboard.glyph = U'A';

	EXPECT_EQ(keyboard.glyph, U'A');
}

TEST(KeyboardTest, ToStringReturnsExpectedValueForRepresentativeKeys)
{
	EXPECT_EQ(spk::toString(spk::Keyboard::Backspace), "Backspace");
	EXPECT_EQ(spk::toString(spk::Keyboard::A), "A");
	EXPECT_EQ(spk::toString(spk::Keyboard::F12), "F12");
	EXPECT_EQ(spk::toString(spk::Keyboard::LeftArrow), "LeftArrow");
	EXPECT_EQ(spk::toString(spk::Keyboard::Numpad3), "Numpad3");
}

TEST(KeyboardTest, ToStringReturnsFallbackForInvalidValue)
{
	const auto invalidKey = static_cast<spk::Keyboard::Key>(999);

	EXPECT_EQ(spk::toString(invalidKey), "Unknow key");
}

TEST(KeyboardTest, ToWstringReturnsExpectedValueForRepresentativeKeys)
{
	EXPECT_EQ(spk::toWstring(spk::Keyboard::Backspace), L"Backspace");
	EXPECT_EQ(spk::toWstring(spk::Keyboard::A), L"A");
	EXPECT_EQ(spk::toWstring(spk::Keyboard::F12), L"F12");
	EXPECT_EQ(spk::toWstring(spk::Keyboard::LeftArrow), L"LeftArrow");
	EXPECT_EQ(spk::toWstring(spk::Keyboard::Numpad3), L"Numpad3");
}

TEST(KeyboardTest, ToWstringReturnsFallbackForInvalidValue)
{
	const auto invalidKey = static_cast<spk::Keyboard::Key>(999);

	EXPECT_EQ(spk::toWstring(invalidKey), L"Unknow key");
}

TEST(KeyboardTest, OstreamOperatorPrintsExpectedText)
{
	std::ostringstream outputStream;
	outputStream << spk::Keyboard::Escape;

	EXPECT_EQ(outputStream.str(), "Escape");
}

TEST(KeyboardTest, WostreamOperatorPrintsExpectedText)
{
	std::wostringstream outputStream;
	outputStream << spk::Keyboard::Escape;

	EXPECT_EQ(outputStream.str(), L"Escape");
}

TEST(KeyboardTest, HighestDefinedRepresentativeKeyCanBeAccessed)
{
	spk::Keyboard keyboard;

	keyboard[spk::Keyboard::Process] = spk::InputState::Down;

	EXPECT_EQ(keyboard[spk::Keyboard::Process], spk::InputState::Down);
}

TEST(KeyboardTest, UnknownNamedKeyCanBeAccessed)
{
	spk::Keyboard keyboard;

	keyboard[spk::Keyboard::Unknow] = spk::InputState::Down;

	EXPECT_EQ(keyboard[spk::Keyboard::Unknow], spk::InputState::Down);
}