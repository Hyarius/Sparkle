#include "spk_window_host.hpp"

namespace spk
{
	WindowHost::WindowHost(std::unique_ptr<IFrame> p_frame, std::shared_ptr<IGPUPlatformRuntime> p_gpuPlatformRuntime) :
		_frame(std::move(p_frame))
	{
		if (_frame == nullptr)
		{
			throw std::runtime_error("spk::WindowHost requires a valid spk::IFrame");
		}

		if (p_gpuPlatformRuntime == nullptr)
		{
			throw std::runtime_error("spk::WindowHost requires a valid spk::IGPUPlatformRuntime");
		}

		_renderContext = p_gpuPlatformRuntime->createRenderContext(*_frame);

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

	void WindowHost::validateClosure()
	{
		_frame->validateClosure();
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
