#pragma once

#include <memory>
#include <string>
#include <utility>

#include "spk_rect_2d.hpp"

namespace spk
{
	class Window;

	class WindowHandle
	{
	private:
		std::weak_ptr<spk::Window> _window;

	public:
		WindowHandle() = default;
		explicit WindowHandle(std::weak_ptr<spk::Window> p_window);

		[[nodiscard]] bool isValid() const;
		[[nodiscard]] bool expired() const;
		[[nodiscard]] bool isClosed() const;

		void requestClosure() const;
		void setTitle(std::string p_title) const;
		void resize(const spk::Rect2D& p_rect) const;
		void setVSync(bool p_enabled) const;
	};
}
