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

	void IFrame::_emitMouseEvent(const spk::MouseEventRecord& p_event)
	{
		_mouseEventContractProvider.trigger(p_event);
	}

	void IFrame::_emitKeyboardEvent(const spk::KeyboardEventRecord& p_event)
	{
		_keyboardEventContractProvider.trigger(p_event);
	}

	void IFrame::_emitFrameEvent(const spk::FrameEventRecord& p_event)
	{
		if (spk::holds<spk::WindowDestroyedRecord>(p_event))
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

	IFrame::MouseEventContract IFrame::subscribeToMouseEvents(MouseEventCallback p_callback)
	{
		return _mouseEventContractProvider.subscribe(std::move(p_callback));
	}

	IFrame::KeyboardEventContract IFrame::subscribeToKeyboardEvents(KeyboardEventCallback p_callback)
	{
		return _keyboardEventContractProvider.subscribe(std::move(p_callback));
	}

	IFrame::FrameEventContract IFrame::subscribeToFrameEvents(FrameEventCallback p_callback)
	{
		return _frameEventContractProvider.subscribe(std::move(p_callback));
	}
}
