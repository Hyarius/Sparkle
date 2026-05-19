#include <gtest/gtest.h>

#ifdef _WIN32

#include <stdexcept>
#include <string>

#include <Windows.h>

#include "spk_winapi_helpers.hpp"

TEST(WinAPIHelpersTest, ConvertsEmptyAsciiAndUtf8RoundTripStrings)
{
	EXPECT_TRUE(spk::WinAPI::toWideString("").empty());
	EXPECT_TRUE(spk::WinAPI::toString(L"").empty());
	EXPECT_EQ(spk::WinAPI::toWideString("Sparkle"), L"Sparkle");
	EXPECT_EQ(spk::WinAPI::toString(L"Sparkle"), "Sparkle");

	const std::string utf8Text = std::string("Window ") + "\xE2\x9C\xA8" + " " + "\xF0\x9F\x9A\x80";
	EXPECT_EQ(spk::WinAPI::toString(spk::WinAPI::toWideString(utf8Text)), utf8Text);
}

TEST(WinAPIHelpersTest, LastErrorMessagesIncludeContextAndErrorCode)
{
	SetLastError(ERROR_FILE_NOT_FOUND);

	const std::string message = spk::WinAPI::lastErrorMessage("TestContext");

	EXPECT_NE(message.find("TestContext failed"), std::string::npos);
	EXPECT_NE(message.find(std::to_string(ERROR_FILE_NOT_FOUND)), std::string::npos);
}

TEST(WinAPIHelpersTest, ThrowLastErrorThrowsRuntimeErrorWithContext)
{
	SetLastError(ERROR_ACCESS_DENIED);

	try
	{
		spk::WinAPI::throwLastError("DeniedContext");
		FAIL() << "Expected throwLastError to throw";
	}
	catch (const std::runtime_error& p_error)
	{
		EXPECT_NE(std::string(p_error.what()).find("DeniedContext failed"), std::string::npos);
		EXPECT_NE(std::string(p_error.what()).find(std::to_string(ERROR_ACCESS_DENIED)), std::string::npos);
	}
}

#endif
