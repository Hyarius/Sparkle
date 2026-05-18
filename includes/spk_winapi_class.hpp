#pragma once

#ifdef _WIN32

#include <string>

#include <Windows.h>

namespace spk::WinAPI
{
	class Window;

	class Class
	{
	private:
		friend class Window;

		HINSTANCE _instance = nullptr;
		std::string _name;
		std::wstring _nativeName;
		bool _isRegistered = false;

	public:
		Class(std::string p_name, WNDPROC p_windowProcedure, HINSTANCE p_instance = GetModuleHandleW(nullptr));
		Class(const Class&) = delete;
		Class(Class&& p_other) noexcept;
		~Class();

		Class& operator=(const Class&) = delete;
		Class& operator=(Class&& p_other) noexcept;

		[[nodiscard]] HINSTANCE instance() const;
		[[nodiscard]] const std::string& name() const;
	};
}

#endif
