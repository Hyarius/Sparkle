#include "spk_window.hpp"

namespace spk
{
	std::unique_ptr<IFrame::Backend> Window::_createDefaultFrameBackend()
	{
		throw std::runtime_error("No default spk::IFrame::Backend is implemented for this platform yet");
	}

	std::unique_ptr<IRenderContext::Backend> Window::_createDefaultRenderBackend()
	{
		throw std::runtime_error("No default spk::IRenderContext::Backend is implemented yet");
	}

	Window::Window(Configuration p_configuration) :
		_frameBackend(p_configuration.frameBackend != nullptr ? std::move(p_configuration.frameBackend) : _createDefaultFrameBackend()),
		_renderBackend(p_configuration.renderBackend != nullptr ? std::move(p_configuration.renderBackend) : _createDefaultRenderBackend())
	{
		_frame = _frameBackend->createFrame(p_configuration.rect, p_configuration.title);

		if (_frame == nullptr)
		{
			throw std::runtime_error("spk::Window failed to create its frame");
		}

		_renderContext = _renderBackend->createRenderContext(*_frame);

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

	void Window::pumpEvents()
	{
		_frameBackend->pumpEvents();
	}

	IFrame::Backend::EventContract Window::subscribeToMouseEvents(IFrame::Backend::EventCallback p_callback)
	{
		return _frameBackend->subscribeToMouseEvents(std::move(p_callback));
	}

	IFrame::Backend::EventContract Window::subscribeToKeyboardEvents(IFrame::Backend::EventCallback p_callback)
	{
		return _frameBackend->subscribeToKeyboardEvents(std::move(p_callback));
	}

	IFrame::Backend::EventContract Window::subscribeToFrameEvents(IFrame::Backend::EventCallback p_callback)
	{
		return _frameBackend->subscribeToFrameEvents(std::move(p_callback));
	}
}
