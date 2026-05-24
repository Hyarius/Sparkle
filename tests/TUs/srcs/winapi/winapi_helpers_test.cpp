#include <gtest/gtest.h>


#include <stdexcept>
#include <string>

#include <Windows.h>

#include "winapi/spk_winapi_helpers.hpp"

TEST(WinAPIHelpersTest, ConvertsEmptyAsciiAndUtf8RoundTripStrings)
{
	EXPECT_TRUE(spk::toWideString("").empty());
	EXPECT_TRUE(spk::toString(L"").empty());
	EXPECT_EQ(spk::toWideString("Sparkle"), L"Sparkle");
	EXPECT_EQ(spk::toString(L"Sparkle"), "Sparkle");

	const std::string utf8Text = std::string("Window ") + "\xE2\x9C\xA8" + " " + "\xF0\x9F\x9A\x80";
	EXPECT_EQ(spk::toString(spk::toWideString(utf8Text)), utf8Text);
}

TEST(WinAPIHelpersTest, LastErrorMessagesIncludeContextAndErrorCode)
{
	SetLastError(ERROR_FILE_NOT_FOUND);

	const std::string message = spk::lastErrorMessage("TestContext");

	EXPECT_NE(message.find("TestContext failed"), std::string::npos);
	EXPECT_NE(message.find(std::to_string(ERROR_FILE_NOT_FOUND)), std::string::npos);
}

TEST(WinAPIHelpersTest, ThrowLastErrorThrowsRuntimeErrorWithContext)
{
	SetLastError(ERROR_ACCESS_DENIED);

	try
	{
		spk::throwLastError("DeniedContext");
		FAIL() << "Expected throwLastError to throw";
	}
	catch (const std::runtime_error& p_error)
	{
		EXPECT_NE(std::string(p_error.what()).find("DeniedContext failed"), std::string::npos);
		EXPECT_NE(std::string(p_error.what()).find(std::to_string(ERROR_ACCESS_DENIED)), std::string::npos);
	}
}

