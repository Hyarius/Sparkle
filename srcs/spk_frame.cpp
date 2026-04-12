#include "spk_frame.hpp"

#include <stdexcept>

namespace spk
{
	IFrame::IFrame(std::shared_ptr<ISurfaceState> p_surfaceState) :
		_surfaceState(std::move(p_surfaceState))
	{
		if (_surfaceState == nullptr)
		{
			throw std::invalid_argument("IFrame requires a valid surface state");
		}
	}

	IFrame::~IFrame()
	{
		_invalidateSurfaceState();
	}

	void IFrame::_emitMouseEvent(const spk::Event& p_event)
	{
		_mouseEventContractProvider.trigger(p_event);
	}

	void IFrame::_emitKeyboardEvent(const spk::Event& p_event)
	{
		_keyboardEventContractProvider.trigger(p_event);
	}

	void IFrame::_emitFrameEvent(const spk::Event& p_event)
	{
		if (p_event.holds<spk::WindowDestroyedPayload>())
		{
			_invalidateSurfaceState();
		}

		_frameEventContractProvider.trigger(p_event);
	}

	void IFrame::_invalidateSurfaceState()
	{
		if (_surfaceState != nullptr)
		{
			_surfaceState->invalidate();
		}
	}

	std::shared_ptr<ISurfaceState> IFrame::surfaceState() const
	{
		return _surfaceState;
	}

	IFrame::EventContract IFrame::subscribeToMouseEvents(EventCallback p_callback)
	{
		return _mouseEventContractProvider.subscribe(std::move(p_callback));
	}

	IFrame::EventContract IFrame::subscribeToKeyboardEvents(EventCallback p_callback)
	{
		return _keyboardEventContractProvider.subscribe(std::move(p_callback));
	}

	IFrame::EventContract IFrame::subscribeToFrameEvents(EventCallback p_callback)
	{
		return _frameEventContractProvider.subscribe(std::move(p_callback));
	}
}
