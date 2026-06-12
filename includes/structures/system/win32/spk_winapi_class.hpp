#pragma once


#include <string>

#include <Windows.h>

namespace spk
{
	class WindowRuntime;

	class WindowClass
	{
	private:
		friend class WindowRuntime;

		HINSTANCE _instance = nullptr;
		std::string _name;
		std::wstring _nativeName;
		bool _isRegistered = false;

	public:
		WindowClass(std::string p_name, WNDPROC p_windowProcedure, HINSTANCE p_instance = GetModuleHandleW(nullptr));
		WindowClass(const WindowClass&) = delete;
		WindowClass(WindowClass&& p_other) noexcept;
		~WindowClass();

		WindowClass& operator=(const WindowClass&) = delete;
		WindowClass& operator=(WindowClass&& p_other) noexcept;

		[[nodiscard]] HINSTANCE instance() const;
		[[nodiscard]] const std::string& name() const;
	};
}

