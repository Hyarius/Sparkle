#pragma once

#include <stdexcept>
#include <string>

#include <Windows.h>

namespace spk
{
	[[nodiscard]] std::wstring toWideString(const std::string &p_value);
	[[nodiscard]] std::string toString(const std::wstring &p_value);
	[[nodiscard]] std::string lastErrorMessage(const std::string &p_context);
	[[noreturn]] void throwLastError(const std::string &p_context);
}
