#include "spk_window_host.hpp"

namespace spk
{
	std::shared_ptr<IPlatformRuntime> WindowHost::_createDefaultPlatformRuntime()
	{
		throw std::runtime_error("No default spk::IPlatformRuntime is implemented for this platform yet");
	}

	std::unique_ptr<IRenderContext::Backend> WindowHost::_createDefaultRenderBackend()
	{
		throw std::runtime_error("No default spk::IRenderContext::Backend is implemented yet");
	}

	WindowHost::WindowHost(Configuration p_configuration) :
		_platformRuntime(p_configuration.platformRuntime != nullptr ? std::move(p_configuration.platformRuntime) : _createDefaultPlatformRuntime()),
		_renderBackend(p_configuration.renderBackend != nullptr ? std::move(p_configuration.renderBackend) : _createDefaultRenderBackend())
	{
		_frame = _platformRuntime->createFrame(p_configuration.rect, p_configuration.title);

		if (_frame == nullptr)
		{
			throw std::runtime_error("spk::WindowHost failed to create its frame");
		}

		_renderContext = _frame->createRenderContext(*_renderBackend);

		if (_renderContext == nullptr)
		{
			throw std::runtime_error("spk::WindowHost failed to create its render context");
		}
	}

	WindowHost::~WindowHost() = default;

	void WindowHost::resize(const spk::Rect2D& p_rect)
	{
		_frame->resize(p_rect);
	}

	void WindowHost::notifyFrameResized(const spk::Rect2D& p_rect)
	{
		_renderContext->notifyResize(p_rect);
	}

	void WindowHost::setTitle(const std::string& p_title)
	{
		_frame->setTitle(p_title);
	}

	void WindowHost::requestClosure()
	{
		_frame->requestClosure();
	}

	spk::Rect2D WindowHost::rect() const
	{
		return _frame->rect();
	}

	std::string WindowHost::title() const
	{
		return _frame->title();
	}

	void WindowHost::makeCurrent()
	{
		_renderContext->makeCurrent();
	}

	void WindowHost::present()
	{
		_renderContext->present();
	}

	void WindowHost::setVSync(bool p_enabled)
	{
		_renderContext->setVSync(p_enabled);
	}

	void WindowHost::pollEvents()
	{
		_platformRuntime->pollEvents();
	}

	IFrame::EventContract WindowHost::subscribeToMouseEvents(IFrame::EventCallback p_callback)
	{
		return _frame->subscribeToMouseEvents(std::move(p_callback));
	}

	IFrame::EventContract WindowHost::subscribeToKeyboardEvents(IFrame::EventCallback p_callback)
	{
		return _frame->subscribeToKeyboardEvents(std::move(p_callback));
	}

	IFrame::EventContract WindowHost::subscribeToFrameEvents(IFrame::EventCallback p_callback)
	{
		return _frame->subscribeToFrameEvents(std::move(p_callback));
	}
}
