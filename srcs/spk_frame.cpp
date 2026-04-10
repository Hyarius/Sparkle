#include "spk_frame.hpp"

namespace spk
{
	IFrame::~IFrame() = default;

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
		_frameEventContractProvider.trigger(p_event);
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
