#pragma once

#ifdef _WIN32

#include <memory>
#include <string>

#include <Windows.h>

#include "spk_frame.hpp"
#include "spk_winapi_class.hpp"
#include "spk_winapi_cursor.hpp"
#include "spk_winapi_window.hpp"

namespace spk::WinAPI
{
	class SurfaceState : public spk::ISurfaceState
	{
	};

	class Frame : public spk::IFrame
	{
	private:
		std::shared_ptr<Class> _class;
		Window _window;
		std::string _title;
		spk::Rect2D _rect;
		spk::Vector2Int _lastMousePosition = {0, 0};
		bool _isTrackingMouseLeave = false;

	private:
		LRESULT _windowProcedure(HWND p_handle, UINT p_message, WPARAM p_wParam, LPARAM p_lParam);
		void _startMouseLeaveTracking();

	public:
		Frame(std::shared_ptr<Class> p_class, const spk::Rect2D& p_rect, const std::string& p_title);
		~Frame() override;

		void resize(const spk::Rect2D& p_rect) override;
		void setTitle(const std::string& p_title) override;
		void requestClosure() override;
		void validateClosure() override;

		[[nodiscard]] spk::Rect2D rect() const override;
		[[nodiscard]] std::string title() const override;
		[[nodiscard]] HWND handle() const;
		[[nodiscard]] HDC deviceContext() const;
	};
}

#endif
