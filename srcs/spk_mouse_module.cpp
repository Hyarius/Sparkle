#include "spk_mouse_module.hpp"

#include <utility>

namespace spk
{
	MouseModule::MouseModule() = default;

	void MouseModule::_treatEvent(spk::MouseEventRecord& p_event)
	{
		if (widget() == nullptr)
		{
			return;
		}

		std::visit(
			spk::Overloaded{
				[this](spk::MouseEnteredRecord& p_record)
				{
					_mouse.position = p_record.position;
					_mouse.deltaPosition = {0, 0};
				},
				[this](spk::MouseLeftRecord& p_record)
				{
					_mouse.position = p_record.position;
					_mouse.deltaPosition = {0, 0};
				},
				[this](spk::MouseMovedRecord& p_record)
				{
					_mouse.deltaPosition = p_record.position - _mouse.position;
					_mouse.position = p_record.position;
				},
				[this](spk::MouseWheelScrolledRecord& p_record)
				{
					_mouse.wheel += p_record.delta.y;
				},
				[this](spk::MouseButtonPressedRecord& p_record)
				{
					_mouse[p_record.button] = spk::InputState::Down;
				},
				[this](spk::MouseButtonReleasedRecord& p_record)
				{
					_mouse[p_record.button] = spk::InputState::Up;
				},
				[](spk::MouseButtonDoubleClickedRecord&)
				{
				}
			},
			p_event);

		widget()->dispatchMouseEvent(p_event, _mouse);
	}

	void MouseModule::pushEvent(spk::MouseEventRecord p_event)
	{
		_events.pushBack(std::move(p_event));
	}

	void MouseModule::processEvents()
	{
		spk::MouseEventRecord event;

		while (_events.popFront(event) == true)
		{
			_treatEvent(event);
		}
	}

	spk::Mouse& MouseModule::mouse()
	{
		return _mouse;
	}

	const spk::Mouse& MouseModule::mouse() const
	{
		return _mouse;
	}
}
