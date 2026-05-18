#include "spk_winapi_helpers.hpp"

#ifdef _WIN32

#include <vector>

namespace spk::WinAPI
{
	std::wstring toWideString(const std::string& p_value)
	{
		if (p_value.empty())
		{
			return {};
		}

		const int size = MultiByteToWideChar(CP_UTF8, 0, p_value.data(), static_cast<int>(p_value.size()), nullptr, 0);
		if (size <= 0)
		{
			throwLastError("MultiByteToWideChar");
		}

		std::wstring result(static_cast<std::size_t>(size), L'\0');
		MultiByteToWideChar(CP_UTF8, 0, p_value.data(), static_cast<int>(p_value.size()), result.data(), size);
		return result;
	}

	std::string toString(const std::wstring& p_value)
	{
		if (p_value.empty())
		{
			return {};
		}

		const int size = WideCharToMultiByte(CP_UTF8, 0, p_value.data(), static_cast<int>(p_value.size()), nullptr, 0, nullptr, nullptr);
		if (size <= 0)
		{
			throwLastError("WideCharToMultiByte");
		}

		std::string result(static_cast<std::size_t>(size), '\0');
		WideCharToMultiByte(CP_UTF8, 0, p_value.data(), static_cast<int>(p_value.size()), result.data(), size, nullptr, nullptr);
		return result;
	}

	std::string lastErrorMessage(const std::string& p_context)
	{
		const DWORD errorCode = GetLastError();
		LPWSTR buffer = nullptr;

		const DWORD size = FormatMessageW(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr,
			errorCode,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			reinterpret_cast<LPWSTR>(&buffer),
			0,
			nullptr);

		std::string message = p_context + " failed";
		if (errorCode != 0)
		{
			message += " (" + std::to_string(errorCode) + ")";
		}

		if (size != 0 && buffer != nullptr)
		{
			message += ": " + toString(std::wstring(buffer, size));
		}

		if (buffer != nullptr)
		{
			LocalFree(buffer);
		}

		return message;
	}

	void throwLastError(const std::string& p_context)
	{
		throw std::runtime_error(lastErrorMessage(p_context));
	}
}

#endif
