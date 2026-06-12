#include "structures/system/win32/spk_winapi_window.hpp"


#include <stdexcept>
#include <vector>

#include "structures/system/win32/spk_winapi_cursor.hpp"
#include "utils/spk_winapi_helpers.hpp"

namespace spk
{
	LRESULT CALLBACK WindowRuntime::_staticWindowProcedure(HWND p_handle, UINT p_message, WPARAM p_wParam, LPARAM p_lParam)
	{
		WindowRuntime* window = reinterpret_cast<WindowRuntime*>(GetWindowLongPtrW(p_handle, GWLP_USERDATA));

		if (p_message == WM_NCCREATE)
		{
			auto* createStruct = reinterpret_cast<CREATESTRUCTW*>(p_lParam);
			window = reinterpret_cast<WindowRuntime*>(createStruct->lpCreateParams);
			window->_handle = p_handle;
			SetWindowLongPtrW(p_handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
		}

		if (window != nullptr)
		{
			return window->_dispatch(p_message, p_wParam, p_lParam);
		}

		return DefWindowProcW(p_handle, p_message, p_wParam, p_lParam);
	}

	LRESULT WindowRuntime::_dispatch(UINT p_message, WPARAM p_wParam, LPARAM p_lParam)
	{
		LRESULT result = 0;
		if (_procedure != nullptr)
		{
			result = _procedure(_handle, p_message, p_wParam, p_lParam);
		}
		else
		{
			result = DefWindowProcW(_handle, p_message, p_wParam, p_lParam);
		}

		if (p_message == WM_NCDESTROY)
		{
			SetWindowLongPtrW(_handle, GWLP_USERDATA, 0);
			_handle = nullptr;
		}

		return result;
	}

	WindowRuntime::WindowRuntime(
		std::shared_ptr<WindowClass> p_windowClass,
		const spk::Rect2D& p_rect,
		const std::string& p_title,
		Procedure p_procedure,
		DWORD p_style,
		DWORD p_extendedStyle) :
		_windowClass(std::move(p_windowClass)),
		_procedure(std::move(p_procedure))
	{
		if (_windowClass == nullptr)
		{
			throw std::invalid_argument("spk::WindowRuntime requires a valid window class");
		}

		RECT nativeRect{
			static_cast<LONG>(p_rect.left()),
			static_cast<LONG>(p_rect.top()),
			static_cast<LONG>(p_rect.right()),
			static_cast<LONG>(p_rect.bottom())
		};
		if (AdjustWindowRectEx(&nativeRect, p_style, FALSE, p_extendedStyle) == FALSE)
		{
			throwLastError("AdjustWindowRectEx");
		}

		_handle = CreateWindowExW(
			p_extendedStyle,
			_windowClass->_nativeName.c_str(),
			toWideString(p_title).c_str(),
			p_style,
			nativeRect.left,
			nativeRect.top,
			nativeRect.right - nativeRect.left,
			nativeRect.bottom - nativeRect.top,
			nullptr,
			nullptr,
			_windowClass->instance(),
			this);

		if (_handle == nullptr)
		{
			throwLastError("CreateWindowExW");
		}
	}

	WindowRuntime::WindowRuntime(WindowRuntime&& p_other) noexcept :
		_windowClass(std::move(p_other._windowClass)),
		_handle(p_other._handle),
		_procedure(std::move(p_other._procedure))
	{
		p_other._handle = nullptr;
		if (_handle != nullptr)
		{
			SetWindowLongPtrW(_handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
		}
	}

	WindowRuntime::~WindowRuntime()
	{
		destroy();
	}

	WindowRuntime& WindowRuntime::operator=(WindowRuntime&& p_other) noexcept
	{
		if (this == &p_other)
		{
			return *this;
		}

		destroy();
		_windowClass = std::move(p_other._windowClass);
		_handle = p_other._handle;
		_procedure = std::move(p_other._procedure);
		p_other._handle = nullptr;

		if (_handle != nullptr)
		{
			SetWindowLongPtrW(_handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
		}

		return *this;
	}

	void WindowRuntime::show(int p_command)
	{
		if (_handle != nullptr)
		{
			ShowWindow(_handle, p_command);
			UpdateWindow(_handle);
		}
	}

	void WindowRuntime::hide()
	{
		if (_handle != nullptr)
		{
			ShowWindow(_handle, SW_HIDE);
		}
	}

	void WindowRuntime::destroy()
	{
		if (_handle != nullptr)
		{
			HWND handle = _handle;
			DestroyWindow(handle);
			if (_handle == handle)
			{
				_handle = nullptr;
			}
		}
	}

	void WindowRuntime::setTitle(const std::string& p_title)
	{
		if (_handle != nullptr && SetWindowTextW(_handle, toWideString(p_title).c_str()) == FALSE)
		{
			throwLastError("SetWindowTextW");
		}
	}

	void WindowRuntime::resize(const spk::Rect2D& p_rect)
	{
		if (_handle == nullptr)
		{
			return;
		}

		RECT nativeRect{
			static_cast<LONG>(p_rect.left()),
			static_cast<LONG>(p_rect.top()),
			static_cast<LONG>(p_rect.right()),
			static_cast<LONG>(p_rect.bottom())
		};

		const DWORD style = static_cast<DWORD>(GetWindowLongPtrW(_handle, GWL_STYLE));
		const DWORD extendedStyle = static_cast<DWORD>(GetWindowLongPtrW(_handle, GWL_EXSTYLE));
		if (AdjustWindowRectEx(&nativeRect, style, FALSE, extendedStyle) == FALSE)
		{
			throwLastError("AdjustWindowRectEx");
		}

		if (SetWindowPos(
				_handle,
				nullptr,
				nativeRect.left,
				nativeRect.top,
				nativeRect.right - nativeRect.left,
				nativeRect.bottom - nativeRect.top,
				SWP_NOZORDER | SWP_NOACTIVATE) == FALSE)
		{
			throwLastError("SetWindowPos");
		}
	}

	void WindowRuntime::setCursor(const spk::Cursor& p_cursor) const
	{
		if (_handle != nullptr && p_cursor.isValid() == true)
		{
			SetClassLongPtrW(_handle, GCLP_HCURSOR, reinterpret_cast<LONG_PTR>(p_cursor.handle()));
		}
	}

	HWND WindowRuntime::handle() const
	{
		return _handle;
	}

	HDC WindowRuntime::deviceContext() const
	{
		if (_handle == nullptr)
		{
			return nullptr;
		}

		return GetDC(_handle);
	}

	spk::Rect2D WindowRuntime::rect() const
	{
		if (_handle == nullptr)
		{
			return {};
		}

		RECT rect{};
		GetWindowRect(_handle, &rect);
		return spk::Rect2D(rect.left, rect.top, static_cast<std::size_t>(rect.right - rect.left), static_cast<std::size_t>(rect.bottom - rect.top));
	}

	spk::Rect2D WindowRuntime::clientRect() const
	{
		if (_handle == nullptr)
		{
			return {};
		}

		RECT client{};
		GetClientRect(_handle, &client);

		POINT origin{0, 0};
		ClientToScreen(_handle, &origin);
		return spk::Rect2D(origin.x, origin.y, static_cast<std::size_t>(client.right - client.left), static_cast<std::size_t>(client.bottom - client.top));
	}

	std::string WindowRuntime::title() const
	{
		if (_handle == nullptr)
		{
			return {};
		}

		const int length = GetWindowTextLengthW(_handle);
		std::wstring result(static_cast<std::size_t>(length) + 1, L'\0');
		GetWindowTextW(_handle, result.data(), length + 1);
		result.resize(static_cast<std::size_t>(length));
		return toString(result);
	}

	bool WindowRuntime::isValid() const
	{
		return _handle != nullptr && IsWindow(_handle) == TRUE;
	}
}

