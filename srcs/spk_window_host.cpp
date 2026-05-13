#include "spk_window_host.hpp"

#include <stdexcept>

namespace spk
{
	WindowHost::WindowHost(std::unique_ptr<IFrame> p_frame, std::shared_ptr<IGPUPlatformRuntime> p_gpuPlatformRuntime) :
		_frame(std::move(p_frame)),
		_gpuPlatformRuntime(std::move(p_gpuPlatformRuntime)),
		_platformThreadID(std::this_thread::get_id())
	{
		if (_frame == nullptr)
		{
			throw std::runtime_error("spk::WindowHost requires a valid spk::IFrame");
		}

		if (_gpuPlatformRuntime == nullptr)
		{
			throw std::runtime_error("spk::WindowHost requires a valid spk::IGPUPlatformRuntime");
		}
	}

	WindowHost::~WindowHost() = default;

	void WindowHost::_bindOrValidatePlatformThread(const char* p_operation) const
	{
		if (std::this_thread::get_id() != _platformThreadID)
		{
			throw std::runtime_error(
				"spk::WindowHost::" + std::string(p_operation) +
				" must be called from the platform thread");
		}
	}

	void WindowHost::_bindOrValidateRenderThreadLocked(const char* p_operation)
	{
		const std::thread::id currentThreadID = std::this_thread::get_id();
		if (_renderThreadID.has_value() == false)
		{
			_renderThreadID = currentThreadID;
			return;
		}

		if (_renderThreadID.value() != currentThreadID)
		{
			throw std::runtime_error(
				"spk::WindowHost::" + std::string(p_operation) +
				" must be called from the render thread");
		}
	}

	bool WindowHost::_ensureRenderContextLocked()
	{
		_bindOrValidateRenderThreadLocked(__FUNCTION__);

		if (_renderContext != nullptr)
		{
			return true;
		}

		if (_frame == nullptr)
		{
			throw std::runtime_error("spk::WindowHost can't create a render context after its frame has been released");
		}

		std::shared_ptr<ISurfaceState> surfaceState = _frame->surfaceState();
		if (surfaceState == nullptr || surfaceState->isValid() == false)
		{
			return false;
		}

		_renderContext = _gpuPlatformRuntime->createRenderContext(*_frame);
		if (_renderContext == nullptr)
		{
			throw std::runtime_error("spk::WindowHost failed to create its render context");
		}

		return true;
	}

	bool WindowHost::isPlatformThread() const
	{
		return (std::this_thread::get_id() == _platformThreadID);
	}

	bool WindowHost::isRenderThread() const
	{
		std::scoped_lock lock(_renderThreadMutex);
		return (_renderThreadID.has_value() == true && _renderThreadID.value() == std::this_thread::get_id());
	}

	void WindowHost::resize(const spk::Rect2D& p_rect)
	{
		_bindOrValidatePlatformThread(__FUNCTION__);

		if (_frame == nullptr)
		{
			throw std::runtime_error("spk::WindowHost::resize called after its frame has been released");
		}

		_frame->resize(p_rect);
	}

	void WindowHost::notifyFrameResized(const spk::Rect2D& p_rect)
	{
		std::scoped_lock lock(_renderThreadMutex);
		if (_ensureRenderContextLocked() == false)
		{
			return;
		}

		if (_renderContext->isValid() == false)
		{
			return;
		}

		_renderContext->notifyResize(p_rect);
	}

	void WindowHost::setTitle(const std::string& p_title)
	{
		_bindOrValidatePlatformThread(__FUNCTION__);

		if (_frame == nullptr)
		{
			throw std::runtime_error("spk::WindowHost::setTitle called after its frame has been released");
		}

		_frame->setTitle(p_title);
	}

	void WindowHost::requestClosure()
	{
		_bindOrValidatePlatformThread(__FUNCTION__);

		if (_frame == nullptr)
		{
			throw std::runtime_error("spk::WindowHost::requestClosure called after its frame has been released");
		}

		_frame->requestClosure();
	}

	void WindowHost::validateClosure()
	{
		_bindOrValidatePlatformThread(__FUNCTION__);

		if (_frame == nullptr)
		{
			throw std::runtime_error("spk::WindowHost::validateClosure called after its frame has been released");
		}

		_frame->validateClosure();
	}

	void WindowHost::releaseFrame()
	{
		_bindOrValidatePlatformThread(__FUNCTION__);
		std::scoped_lock lock(_renderThreadMutex);

		if (_frame != nullptr && _frame->surfaceState() != nullptr)
		{
			_frame->surfaceState()->invalidate();
		}

		_frame.reset();
	}

	void WindowHost::releaseRenderContext()
	{
		std::scoped_lock lock(_renderThreadMutex);
		_bindOrValidateRenderThreadLocked(__FUNCTION__);
		_renderContext.reset();
	}

	spk::Rect2D WindowHost::rect() const
	{
		_bindOrValidatePlatformThread(__FUNCTION__);

		if (_frame == nullptr)
		{
			throw std::runtime_error("spk::WindowHost::rect called after its frame has been released");
		}

		return _frame->rect();
	}

	std::string WindowHost::title() const
	{
		_bindOrValidatePlatformThread(__FUNCTION__);

		if (_frame == nullptr)
		{
			throw std::runtime_error("spk::WindowHost::title called after its frame has been released");
		}

		return _frame->title();
	}

	bool WindowHost::makeCurrent()
	{
		std::scoped_lock lock(_renderThreadMutex);
		if (_ensureRenderContextLocked() == false)
		{
			return false;
		}

		if (_renderContext->isValid() == false)
		{
			return false;
		}

		_renderContext->makeCurrent();
		return true;
	}

	void WindowHost::present()
	{
		std::scoped_lock lock(_renderThreadMutex);
		if (_ensureRenderContextLocked() == false)
		{
			return;
		}

		if (_renderContext->isValid() == false)
		{
			return;
		}

		_renderContext->present();
	}

	void WindowHost::setVSync(bool p_enabled)
	{
		std::scoped_lock lock(_renderThreadMutex);
		if (_ensureRenderContextLocked() == false)
		{
			return;
		}

		if (_renderContext->isValid() == false)
		{
			return;
		}

		_renderContext->setVSync(p_enabled);
	}

	IFrame::MouseEventContract WindowHost::subscribeToMouseEvents(IFrame::MouseEventCallback p_callback)
	{
		_bindOrValidatePlatformThread(__FUNCTION__);

		if (_frame == nullptr)
		{
			throw std::runtime_error("spk::WindowHost::subscribeToMouseEvents called after its frame has been released");
		}

		return _frame->subscribeToMouseEvents(std::move(p_callback));
	}

	IFrame::KeyboardEventContract WindowHost::subscribeToKeyboardEvents(IFrame::KeyboardEventCallback p_callback)
	{
		_bindOrValidatePlatformThread(__FUNCTION__);

		if (_frame == nullptr)
		{
			throw std::runtime_error("spk::WindowHost::subscribeToKeyboardEvents called after its frame has been released");
		}

		return _frame->subscribeToKeyboardEvents(std::move(p_callback));
	}

	IFrame::FrameEventContract WindowHost::subscribeToFrameEvents(IFrame::FrameEventCallback p_callback)
	{
		_bindOrValidatePlatformThread(__FUNCTION__);

		if (_frame == nullptr)
		{
			throw std::runtime_error("spk::WindowHost::subscribeToFrameEvents called after its frame has been released");
		}

		return _frame->subscribeToFrameEvents(std::move(p_callback));
	}
}
