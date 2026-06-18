#pragma once

#include <memory>
#include <string>

#include <Windows.h>

#include "structures/system/device/window/spk_frame.hpp"
#include "structures/system/win32/spk_winapi_class.hpp"
#include "structures/system/win32/spk_winapi_cursor.hpp"
#include "structures/system/win32/spk_winapi_window.hpp"

namespace spk
{
	class Frame : public spk::IFrame
	{
	private:
		std::shared_ptr<WindowClass> _class;
		WindowRuntime _window;
		std::string _title;
		spk::Rect2D _rect;
		spk::Vector2Int _lastMousePosition = {0, 0};
		bool _isTrackingMouseLeave = false;

	private:
		LRESULT _windowProcedure(HWND p_handle, UINT p_message, WPARAM p_wParam, LPARAM p_lParam);
		void _startMouseLeaveTracking();

	public:
		Frame(std::shared_ptr<WindowClass> p_class, const spk::Rect2D& p_rect, const std::string& p_title);
		~Frame() override;

		void resize(const spk::Rect2D& p_rect) override;
		void setTitle(const std::string& p_title) override;
		void setCursor(const std::string& p_name) override;
		void hide() override;
		void requestClosure() override;
		void validateClosure() override;

		[[nodiscard]] spk::Rect2D rect() const override;
		[[nodiscard]] std::string title() const override;
		[[nodiscard]] HWND handle() const;
		[[nodiscard]] HDC deviceContext() const;
	};
}
