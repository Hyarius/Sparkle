#include "spk_window_handle.hpp"

#include "spk_window.hpp"

namespace spk
{
	WindowHandle::WindowHandle(std::weak_ptr<spk::Window> p_window) :
		_window(std::move(p_window))
	{
	}

	bool WindowHandle::isValid() const
	{
		return _window.expired() == false;
	}

	bool WindowHandle::expired() const
	{
		return _window.expired();
	}

	bool WindowHandle::isClosed() const
	{
		std::shared_ptr<spk::Window> window = _window.lock();

		if (window == nullptr)
		{
			return true;
		}

		return window->isClosed();
	}

	void WindowHandle::requestClosure() const
	{
		std::shared_ptr<spk::Window> window = _window.lock();

		if (window != nullptr)
		{
			window->requestClosure();
		}
	}

	void WindowHandle::setTitle(std::string p_title) const
	{
		std::shared_ptr<spk::Window> window = _window.lock();

		if (window != nullptr)
		{
			window->_setTitle(std::move(p_title));
		}
	}

	void WindowHandle::resize(const spk::Rect2D& p_rect) const
	{
		std::shared_ptr<spk::Window> window = _window.lock();

		if (window != nullptr)
		{
			window->_resize(p_rect);
		}
	}

	void WindowHandle::setVSync(bool p_enabled) const
	{
		std::shared_ptr<spk::Window> window = _window.lock();

		if (window != nullptr)
		{
			window->_setVSync(p_enabled);
		}
	}

	spk::Widget& WindowHandle::centralWidget() const
	{
		std::shared_ptr<spk::Window> window = _window.lock();

		if (window == nullptr)
		{
			throw std::runtime_error("WindowHandle::centralWidget : can't access an expired window");
		}

		return window->_centralWidget();
	}
}
