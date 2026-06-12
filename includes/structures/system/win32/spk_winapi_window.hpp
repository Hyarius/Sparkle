#pragma once


#include <functional>
#include <memory>
#include <string>

#include <Windows.h>

#include "structures/math/spk_rect_2d.hpp"
#include "structures/system/win32/spk_winapi_class.hpp"

namespace spk
{
	class Cursor;

	class WindowRuntime
	{
	public:
		using Procedure = std::function<LRESULT(HWND, UINT, WPARAM, LPARAM)>;

	private:
		friend class PlatformRuntime;

		std::shared_ptr<WindowClass> _windowClass;
		HWND _handle = nullptr;
		Procedure _procedure = nullptr;

	private:
		static LRESULT CALLBACK _staticWindowProcedure(HWND p_handle, UINT p_message, WPARAM p_wParam, LPARAM p_lParam);
		LRESULT _dispatch(UINT p_message, WPARAM p_wParam, LPARAM p_lParam);

	public:
		WindowRuntime(
			std::shared_ptr<WindowClass> p_windowClass,
			const spk::Rect2D& p_rect,
			const std::string& p_title,
			Procedure p_procedure = nullptr,
			DWORD p_style = WS_OVERLAPPEDWINDOW,
			DWORD p_extendedStyle = 0);
		WindowRuntime(const WindowRuntime&) = delete;
		WindowRuntime(WindowRuntime&& p_other) noexcept;
		~WindowRuntime();

		WindowRuntime& operator=(const WindowRuntime&) = delete;
		WindowRuntime& operator=(WindowRuntime&& p_other) noexcept;

		void show(int p_command = SW_SHOWNORMAL);
		void hide();
		void destroy();
		void setTitle(const std::string& p_title);
		void resize(const spk::Rect2D& p_rect);
		void setCursor(const spk::Cursor& p_cursor) const;

		[[nodiscard]] HWND handle() const;
		[[nodiscard]] HDC deviceContext() const;
		[[nodiscard]] spk::Rect2D rect() const;
		[[nodiscard]] spk::Rect2D clientRect() const;
		[[nodiscard]] std::string title() const;
		[[nodiscard]] bool isValid() const;
	};
}

