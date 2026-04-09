#include "spk_frame.hpp"

namespace spk
{
	IFrame::Backend::~Backend() = default;

	void IFrame::Backend::_emitMouseEvent(const spk::Event& p_event)
	{
		_mouseEventContractProvider.trigger(p_event);
	}

	void IFrame::Backend::_emitKeyboardEvent(const spk::Event& p_event)
	{
		_keyboardEventContractProvider.trigger(p_event);
	}

	void IFrame::Backend::_emitFrameEvent(const spk::Event& p_event)
	{
		_frameEventContractProvider.trigger(p_event);
	}

	IFrame::Backend::EventContract IFrame::Backend::subscribeToMouseEvents(EventCallback p_callback)
	{
		return _mouseEventContractProvider.subscribe(std::move(p_callback));
	}

	IFrame::Backend::EventContract IFrame::Backend::subscribeToKeyboardEvents(EventCallback p_callback)
	{
		return _keyboardEventContractProvider.subscribe(std::move(p_callback));
	}

	IFrame::Backend::EventContract IFrame::Backend::subscribeToFrameEvents(EventCallback p_callback)
	{
		return _frameEventContractProvider.subscribe(std::move(p_callback));
	}
}
