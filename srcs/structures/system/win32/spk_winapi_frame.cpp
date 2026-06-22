#include "structures/system/win32/spk_winapi_frame.hpp"

#include <windowsx.h>

#include "utils/spk_winapi_helpers.hpp"

namespace
{
	[[nodiscard]] spk::Vector2Int mousePositionFromLParam(LPARAM p_lParam)
	{
		return spk::Vector2Int(GET_X_LPARAM(p_lParam), GET_Y_LPARAM(p_lParam));
	}

	[[nodiscard]] spk::Mouse::Button mouseButtonFromMessage(UINT p_message)
	{
		switch (p_message)
		{
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_RBUTTONDBLCLK:
			return spk::Mouse::Right;
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_MBUTTONDBLCLK:
			return spk::Mouse::Middle;
		default:
			return spk::Mouse::Left;
		}
	}

	[[nodiscard]] spk::Keyboard::Key keyFromWParam(WPARAM p_wParam)
	{
		const std::size_t value = static_cast<std::size_t>(p_wParam);
		if (value >= spk::Keyboard::NbKey)
		{
			return spk::Keyboard::Unknown;
		}

		return static_cast<spk::Keyboard::Key>(value);
	}
}

namespace spk
{
	LRESULT Frame::_windowProcedure(HWND p_handle, UINT p_message, WPARAM p_wParam, LPARAM p_lParam)
	{
		switch (p_message)
		{
		case WM_CLOSE:
			_emitFrameEvent(spk::FrameEventRecord(spk::WindowCloseRequestedRecord{}));
			return 0;
		case WM_DESTROY:
			_emitFrameEvent(spk::FrameEventRecord(spk::WindowDestroyedRecord{}));
			return 0;
		case WM_MOVE:
		{
			_rect = _window.clientRect();
			surfaceState()->setRect(_rect);
			spk::WindowMovedRecord record;
			record.position = _rect.anchor;
			_emitFrameEvent(spk::FrameEventRecord(record));
			return 0;
		}
		case WM_SIZE:
		{
			_rect = _window.clientRect();
			surfaceState()->setRect(_rect);
			spk::WindowResizedRecord record;
			record.rect = _rect;
			_emitFrameEvent(spk::FrameEventRecord(record));
			return 0;
		}
		case WM_SETFOCUS:
			_emitFrameEvent(spk::FrameEventRecord(spk::WindowFocusGainedRecord{}));
			return 0;
		case WM_KILLFOCUS:
			_emitFrameEvent(spk::FrameEventRecord(spk::WindowFocusLostRecord{}));
			return 0;
		case WM_SHOWWINDOW:
			_emitFrameEvent(spk::FrameEventRecord(p_wParam != 0 ? spk::FrameEventRecord(spk::WindowShownRecord{}) : spk::FrameEventRecord(spk::WindowHiddenRecord{})));
			return 0;
		case WM_MOUSEMOVE:
		{
			const spk::Vector2Int position = mousePositionFromLParam(p_lParam);
			if (_isTrackingMouseLeave == false)
			{
				_startMouseLeaveTracking();
				spk::MouseEnteredRecord enteredRecord;
				enteredRecord.position = position;
				_emitMouseEvent(spk::MouseEventRecord(enteredRecord));
			}

			spk::MouseMovedRecord record;
			record.position = position;
			record.delta = position - _lastMousePosition;
			_lastMousePosition = position;
			_emitMouseEvent(spk::MouseEventRecord(record));
			return 0;
		}
		case WM_MOUSELEAVE:
		{
			_isTrackingMouseLeave = false;
			spk::MouseLeftRecord record;
			record.position = _lastMousePosition;
			_emitMouseEvent(spk::MouseEventRecord(record));
			return 0;
		}
		case WM_MOUSEWHEEL:
		{
			spk::MouseWheelScrolledRecord record;
			record.delta = spk::Vector2(0.0f, static_cast<float>(GET_WHEEL_DELTA_WPARAM(p_wParam)) / static_cast<float>(WHEEL_DELTA));
			_emitMouseEvent(spk::MouseEventRecord(record));
			return 0;
		}
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
		{
			SetCapture(p_handle);
			spk::MouseButtonPressedRecord record;
			record.button = mouseButtonFromMessage(p_message);
			_emitMouseEvent(spk::MouseEventRecord(record));
			return 0;
		}
		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MBUTTONUP:
		{
			ReleaseCapture();
			spk::MouseButtonReleasedRecord record;
			record.button = mouseButtonFromMessage(p_message);
			_emitMouseEvent(spk::MouseEventRecord(record));
			return 0;
		}
		case WM_LBUTTONDBLCLK:
		case WM_RBUTTONDBLCLK:
		case WM_MBUTTONDBLCLK:
		{
			spk::MouseButtonDoubleClickedRecord record;
			record.button = mouseButtonFromMessage(p_message);
			_emitMouseEvent(spk::MouseEventRecord(record));
			return 0;
		}
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
		{
			spk::KeyPressedRecord record;
			record.key = keyFromWParam(p_wParam);
			record.isRepeated = ((p_lParam & (1 << 30)) != 0);
			_emitKeyboardEvent(spk::KeyboardEventRecord(record));
			return 0;
		}
		case WM_KEYUP:
		case WM_SYSKEYUP:
		{
			spk::KeyReleasedRecord record;
			record.key = keyFromWParam(p_wParam);
			_emitKeyboardEvent(spk::KeyboardEventRecord(record));
			return 0;
		}
		case WM_CHAR:
		{
			spk::TextInputRecord record;
			record.glyph = static_cast<char32_t>(p_wParam);
			_emitKeyboardEvent(spk::KeyboardEventRecord(record));
			return 0;
		}
		default:
			return DefWindowProcW(p_handle, p_message, p_wParam, p_lParam);
		}
	}

	void Frame::_startMouseLeaveTracking()
	{
		TRACKMOUSEEVENT event{};
		event.cbSize = sizeof(event);
		event.dwFlags = TME_LEAVE;
		event.hwndTrack = _window.handle();
		if (TrackMouseEvent(&event) == TRUE)
		{
			_isTrackingMouseLeave = true;
		}
	}

	Frame::Frame(std::shared_ptr<WindowClass> p_class, const spk::Rect2D &p_rect, const std::string &p_title) :
		spk::IFrame(std::make_shared<spk::SurfaceState>()),
		_class(std::move(p_class)),
		_window(
			_class,
			p_rect,
			p_title,
			[this](HWND p_handle, UINT p_message, WPARAM p_wParam, LPARAM p_lParam) {
				return _windowProcedure(p_handle, p_message, p_wParam, p_lParam);
			}),
		_title(p_title),
		_rect(_window.clientRect())
	{
		surfaceState()->setRect(_rect);
		_window.show();
	}

	Frame::~Frame()
	{
		_window.destroy();
	}

	void Frame::resize(const spk::Rect2D &p_rect)
	{
		_window.resize(p_rect);
		_rect = _window.clientRect();
		surfaceState()->setRect(_rect);
	}

	void Frame::setTitle(const std::string &p_title)
	{
		_window.setTitle(p_title);
		_title = p_title;
	}

	void Frame::setCursor(const std::string &p_name)
	{
		spk::Cursor cursor = spk::Cursor::get(p_name);
		_window.setCursor(cursor);
		cursor.activate();
	}

	void Frame::hide()
	{
		_window.hide();
	}

	void Frame::requestClosure()
	{
		if (_window.isValid() == true)
		{
			PostMessageW(_window.handle(), WM_CLOSE, 0, 0);
		}
	}

	void Frame::validateClosure()
	{
		_window.destroy();
	}

	spk::Rect2D Frame::rect() const
	{
		return _rect;
	}

	std::string Frame::title() const
	{
		return _title;
	}

	HWND Frame::handle() const
	{
		return _window.handle();
	}

	HDC Frame::deviceContext() const
	{
		return _window.deviceContext();
	}
}
