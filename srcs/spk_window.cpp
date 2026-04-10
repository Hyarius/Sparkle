#include "spk_window.hpp"

namespace spk
{
	std::shared_ptr<IPlatformRuntime> Window::_createDefaultPlatformRuntime()
	{
		throw std::runtime_error("No default spk::IPlatformRuntime is implemented for this platform yet");
	}

	std::unique_ptr<IRenderContext::Backend> Window::_createDefaultRenderBackend()
	{
		throw std::runtime_error("No default spk::IRenderContext::Backend is implemented yet");
	}

	Window::Window(Configuration p_configuration) :
		_platformRuntime(p_configuration.platformRuntime != nullptr ? std::move(p_configuration.platformRuntime) : _createDefaultPlatformRuntime()),
		_renderBackend(p_configuration.renderBackend != nullptr ? std::move(p_configuration.renderBackend) : _createDefaultRenderBackend())
	{
		_frame = _platformRuntime->createFrame(p_configuration.rect, p_configuration.title);

		if (_frame == nullptr)
		{
			throw std::runtime_error("spk::Window failed to create its frame");
		}

		_renderContext = _frame->createRenderContext(*_renderBackend);

		if (_renderContext == nullptr)
		{
			throw std::runtime_error("spk::Window failed to create its render context");
		}
	}

	Window::~Window() = default;

	void Window::resize(const spk::Rect2D& p_rect)
	{
		_frame->resize(p_rect);
	}

	void Window::notifyFrameResized(const spk::Rect2D& p_rect)
	{
		_renderContext->notifyResize(p_rect);
	}

	void Window::setTitle(const std::string& p_title)
	{
		_frame->setTitle(p_title);
	}

	void Window::requestClosure()
	{
		_frame->requestClosure();
	}

	spk::Rect2D Window::rect() const
	{
		return _frame->rect();
	}

	std::string Window::title() const
	{
		return _frame->title();
	}

	void Window::makeCurrent()
	{
		_renderContext->makeCurrent();
	}

	void Window::present()
	{
		_renderContext->present();
	}

	void Window::setVSync(bool p_enabled)
	{
		_renderContext->setVSync(p_enabled);
	}

	void Window::pollEvents()
	{
		_platformRuntime->pollEvents();
	}

	IFrame::EventContract Window::subscribeToMouseEvents(IFrame::EventCallback p_callback)
	{
		return _frame->subscribeToMouseEvents(std::move(p_callback));
	}

	IFrame::EventContract Window::subscribeToKeyboardEvents(IFrame::EventCallback p_callback)
	{
		return _frame->subscribeToKeyboardEvents(std::move(p_callback));
	}

	IFrame::EventContract Window::subscribeToFrameEvents(IFrame::EventCallback p_callback)
	{
		return _frame->subscribeToFrameEvents(std::move(p_callback));
	}
}
