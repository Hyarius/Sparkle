#include "spk_widget.hpp"

namespace spk
{
	void Widget::_appendRenderCommands(spk::RenderCommandBuilder& p_builder) const
	{
		(void)p_builder;
	}

	void Widget::_onUpdate(const spk::UpdateTick& p_tick)
	{
		(void)p_tick;
	}

#define SPK_DEFINE_EMPTY_EVENT_HANDLER(MethodName, EventType) \
	void Widget::MethodName(EventType& p_event) \
	{ \
		(void)p_event; \
	}

	SPK_DEFINE_EMPTY_EVENT_HANDLER(_onWindowCloseRequestedEvent, spk::WindowCloseRequestedEvent)
	SPK_DEFINE_EMPTY_EVENT_HANDLER(_onWindowDestroyedEvent, spk::WindowDestroyedEvent)
	SPK_DEFINE_EMPTY_EVENT_HANDLER(_onWindowMovedEvent, spk::WindowMovedEvent)
	SPK_DEFINE_EMPTY_EVENT_HANDLER(_onWindowResizedEvent, spk::WindowResizedEvent)
	SPK_DEFINE_EMPTY_EVENT_HANDLER(_onWindowFocusGainedEvent, spk::WindowFocusGainedEvent)
	SPK_DEFINE_EMPTY_EVENT_HANDLER(_onWindowFocusLostEvent, spk::WindowFocusLostEvent)
	SPK_DEFINE_EMPTY_EVENT_HANDLER(_onWindowShownEvent, spk::WindowShownEvent)
	SPK_DEFINE_EMPTY_EVENT_HANDLER(_onWindowHiddenEvent, spk::WindowHiddenEvent)
	SPK_DEFINE_EMPTY_EVENT_HANDLER(_onMouseEnteredEvent, spk::MouseEnteredWindowEvent)
	SPK_DEFINE_EMPTY_EVENT_HANDLER(_onMouseLeftEvent, spk::MouseLeftWindowEvent)
	SPK_DEFINE_EMPTY_EVENT_HANDLER(_onMouseMovedEvent, spk::MouseMovedEvent)
	SPK_DEFINE_EMPTY_EVENT_HANDLER(_onMouseWheelScrolledEvent, spk::MouseWheelScrolledEvent)
	SPK_DEFINE_EMPTY_EVENT_HANDLER(_onMouseButtonPressedEvent, spk::MouseButtonPressedEvent)
	SPK_DEFINE_EMPTY_EVENT_HANDLER(_onMouseButtonReleasedEvent, spk::MouseButtonReleasedEvent)
	SPK_DEFINE_EMPTY_EVENT_HANDLER(_onMouseButtonDoubleClickedEvent, spk::MouseButtonDoubleClickedEvent)
	SPK_DEFINE_EMPTY_EVENT_HANDLER(_onKeyPressedEvent, spk::KeyPressedEvent)
	SPK_DEFINE_EMPTY_EVENT_HANDLER(_onKeyReleasedEvent, spk::KeyReleasedEvent)
	SPK_DEFINE_EMPTY_EVENT_HANDLER(_onTextInputEvent, spk::TextInputEvent)

#undef SPK_DEFINE_EMPTY_EVENT_HANDLER

#define SPK_DEFINE_EVENT_PROPAGATION(MethodName, ViewType, HandlerName) \
	void Widget::MethodName(ViewType& p_event) \
	{ \
		if (isActivated() == false) \
		{ \
			return; \
		} \
		for (auto* child : children()) \
		{ \
			if (child != nullptr) \
			{ \
				child->MethodName(p_event); \
				if (p_event.isConsumed() == true) \
				{ \
					return; \
				} \
			} \
		} \
		HandlerName(p_event); \
	}

	SPK_DEFINE_EVENT_PROPAGATION(_propagateWindowCloseRequestedEvent, spk::WindowCloseRequestedEvent, _onWindowCloseRequestedEvent)
	SPK_DEFINE_EVENT_PROPAGATION(_propagateWindowDestroyedEvent, spk::WindowDestroyedEvent, _onWindowDestroyedEvent)
	SPK_DEFINE_EVENT_PROPAGATION(_propagateWindowMovedEvent, spk::WindowMovedEvent, _onWindowMovedEvent)
	SPK_DEFINE_EVENT_PROPAGATION(_propagateWindowResizedEvent, spk::WindowResizedEvent, _onWindowResizedEvent)
	SPK_DEFINE_EVENT_PROPAGATION(_propagateWindowFocusGainedEvent, spk::WindowFocusGainedEvent, _onWindowFocusGainedEvent)
	SPK_DEFINE_EVENT_PROPAGATION(_propagateWindowFocusLostEvent, spk::WindowFocusLostEvent, _onWindowFocusLostEvent)
	SPK_DEFINE_EVENT_PROPAGATION(_propagateWindowShownEvent, spk::WindowShownEvent, _onWindowShownEvent)
	SPK_DEFINE_EVENT_PROPAGATION(_propagateWindowHiddenEvent, spk::WindowHiddenEvent, _onWindowHiddenEvent)
	SPK_DEFINE_EVENT_PROPAGATION(_propagateMouseEnteredEvent, spk::MouseEnteredWindowEvent, _onMouseEnteredEvent)
	SPK_DEFINE_EVENT_PROPAGATION(_propagateMouseLeftEvent, spk::MouseLeftWindowEvent, _onMouseLeftEvent)
	SPK_DEFINE_EVENT_PROPAGATION(_propagateMouseMovedEvent, spk::MouseMovedEvent, _onMouseMovedEvent)
	SPK_DEFINE_EVENT_PROPAGATION(_propagateMouseWheelScrolledEvent, spk::MouseWheelScrolledEvent, _onMouseWheelScrolledEvent)
	SPK_DEFINE_EVENT_PROPAGATION(_propagateMouseButtonPressedEvent, spk::MouseButtonPressedEvent, _onMouseButtonPressedEvent)
	SPK_DEFINE_EVENT_PROPAGATION(_propagateMouseButtonReleasedEvent, spk::MouseButtonReleasedEvent, _onMouseButtonReleasedEvent)
	SPK_DEFINE_EVENT_PROPAGATION(_propagateMouseButtonDoubleClickedEvent, spk::MouseButtonDoubleClickedEvent, _onMouseButtonDoubleClickedEvent)
	SPK_DEFINE_EVENT_PROPAGATION(_propagateKeyPressedEvent, spk::KeyPressedEvent, _onKeyPressedEvent)
	SPK_DEFINE_EVENT_PROPAGATION(_propagateKeyReleasedEvent, spk::KeyReleasedEvent, _onKeyReleasedEvent)
	SPK_DEFINE_EVENT_PROPAGATION(_propagateTextInputEvent, spk::TextInputEvent, _onTextInputEvent)

#undef SPK_DEFINE_EVENT_PROPAGATION

	Widget::Widget(const std::string& p_name, spk::Widget* p_parent) :
		spk::InherenceTrait<Widget>(p_parent),
		_name(p_name)
	{
	}

	Widget::~Widget() = default;

	const std::string& Widget::name() const
	{
		return (_name);
	}

	void Widget::setGeometry(const spk::Rect2D& p_geometry)
	{
		if (_geometry == p_geometry)
		{
			return;
		}

		_geometry = p_geometry;
		_renderCommandsDirty = true;
	}

	void Widget::requireRenderCommandRebuild()
	{
		_renderCommandsDirty = true;
	}

	const spk::Rect2D& Widget::geometry() const
	{
		return (_geometry);
	}

	bool Widget::isRenderCommandDirty() const
	{
		return (_renderCommandsDirty);
	}

	void Widget::appendRenderCommands(spk::RenderCommandBuilder& p_builder) const
	{
		if (isActivated() == false)
		{
			return;
		}

		_appendRenderCommands(p_builder);
		_renderCommandsDirty = false;

		for (const auto* child : children())
		{
			if (child != nullptr)
			{
				child->appendRenderCommands(p_builder);
			}
		}
	}

	void Widget::update(const spk::UpdateTick& p_tick)
	{
		if (isActivated() == false)
		{
			return;
		}

		for (auto* child : children())
		{
			if (child != nullptr)
			{
				child->update(p_tick);
			}
		}

		_onUpdate(p_tick);
	}

	bool Widget::dispatchFrameEvent(spk::FrameEventRecord& p_event)
	{
		return std::visit(
			spk::Overloaded{
				[this](const spk::WindowCloseRequestedRecord& p_record) { spk::WindowCloseRequestedEvent event(p_record); _propagateWindowCloseRequestedEvent(event); return event.isConsumed(); },
				[this](const spk::WindowDestroyedRecord& p_record) { spk::WindowDestroyedEvent event(p_record); _propagateWindowDestroyedEvent(event); return event.isConsumed(); },
				[this](const spk::WindowMovedRecord& p_record) { spk::WindowMovedEvent event(p_record); _propagateWindowMovedEvent(event); return event.isConsumed(); },
				[this](const spk::WindowResizedRecord& p_record) { spk::WindowResizedEvent event(p_record); _propagateWindowResizedEvent(event); return event.isConsumed(); },
				[this](const spk::WindowFocusGainedRecord& p_record) { spk::WindowFocusGainedEvent event(p_record); _propagateWindowFocusGainedEvent(event); return event.isConsumed(); },
				[this](const spk::WindowFocusLostRecord& p_record) { spk::WindowFocusLostEvent event(p_record); _propagateWindowFocusLostEvent(event); return event.isConsumed(); },
				[this](const spk::WindowShownRecord& p_record) { spk::WindowShownEvent event(p_record); _propagateWindowShownEvent(event); return event.isConsumed(); },
				[this](const spk::WindowHiddenRecord& p_record) { spk::WindowHiddenEvent event(p_record); _propagateWindowHiddenEvent(event); return event.isConsumed(); }
			},
			p_event);
	}

	void Widget::dispatchMouseEvent(spk::MouseEventRecord& p_event, spk::Mouse& p_mouse)
	{
		std::visit(
			spk::Overloaded{
				[this, &p_mouse](const spk::MouseEnteredRecord& p_record) { spk::MouseEnteredWindowEvent event(p_record, p_mouse); _propagateMouseEnteredEvent(event); },
				[this, &p_mouse](const spk::MouseLeftRecord& p_record) { spk::MouseLeftWindowEvent event(p_record, p_mouse); _propagateMouseLeftEvent(event); },
				[this, &p_mouse](const spk::MouseMovedRecord& p_record) { spk::MouseMovedEvent event(p_record, p_mouse); _propagateMouseMovedEvent(event); },
				[this, &p_mouse](const spk::MouseWheelScrolledRecord& p_record) { spk::MouseWheelScrolledEvent event(p_record, p_mouse); _propagateMouseWheelScrolledEvent(event); },
				[this, &p_mouse](const spk::MouseButtonPressedRecord& p_record) { spk::MouseButtonPressedEvent event(p_record, p_mouse); _propagateMouseButtonPressedEvent(event); },
				[this, &p_mouse](const spk::MouseButtonReleasedRecord& p_record) { spk::MouseButtonReleasedEvent event(p_record, p_mouse); _propagateMouseButtonReleasedEvent(event); },
				[this, &p_mouse](const spk::MouseButtonDoubleClickedRecord& p_record) { spk::MouseButtonDoubleClickedEvent event(p_record, p_mouse); _propagateMouseButtonDoubleClickedEvent(event); }
			},
			p_event);
	}

	void Widget::dispatchKeyboardEvent(spk::KeyboardEventRecord& p_event, spk::Keyboard& p_keyboard)
	{
		std::visit(
			spk::Overloaded{
				[this, &p_keyboard](const spk::KeyPressedRecord& p_record) { spk::KeyPressedEvent event(p_record, p_keyboard); _propagateKeyPressedEvent(event); },
				[this, &p_keyboard](const spk::KeyReleasedRecord& p_record) { spk::KeyReleasedEvent event(p_record, p_keyboard); _propagateKeyReleasedEvent(event); },
				[this, &p_keyboard](const spk::TextInputRecord& p_record) { spk::TextInputEvent event(p_record, p_keyboard); _propagateTextInputEvent(event); }
			},
			p_event);
	}
}
