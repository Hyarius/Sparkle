#include "widget/spk_widget.hpp"

#include <memory>
#include "rendering/spk_render_unit_builder.hpp"
#include "rendering/render_command/spk_viewport_render_command.hpp"

#if defined(SPARKLE_GPU_BACKEND_OPENGL)
#include "opengl/spk_opengl_viewport.hpp"
#endif

namespace spk
{
	namespace
	{
		std::unique_ptr<spk::Viewport> makeViewport()
		{
#if defined(SPARKLE_GPU_BACKEND_OPENGL)
			return std::make_unique<spk::OpenGL::Viewport>();
#else
			return nullptr;
#endif
		}

		std::shared_ptr<spk::RenderUnit> makeViewportUnit(const spk::Viewport* p_viewport)
		{
			if (p_viewport == nullptr ||
				p_viewport->geometry().empty() == true ||
				p_viewport->scissor().empty() == true)
			{
				return nullptr;
			}

			spk::RenderUnitBuilder builder;
			builder.emplace<spk::ViewportCommand>(*p_viewport);
			return std::make_shared<spk::RenderUnit>(builder.build());
		}
	}

	spk::RenderUnit Widget::_buildRenderUnit() const
	{
		return spk::RenderUnit();
	}

	void Widget::onParentChanged(spk::Widget* p_oldParent, spk::Widget* p_newParent)
	{
		(void)p_oldParent;
		(void)p_newParent;
		_updateAbsoluteGeometryAndScissor();
	}

	void Widget::_onUpdate(const spk::UpdateTick &p_tick)
	{
		(void)p_tick;
	}

	void Widget::_onWindowCloseRequestedEvent(spk::WindowCloseRequestedEvent &p_event) { (void)p_event; }
	void Widget::_onWindowDestroyedEvent(spk::WindowDestroyedEvent &p_event) { (void)p_event; }
	void Widget::_onWindowMovedEvent(spk::WindowMovedEvent &p_event) { (void)p_event; }
	void Widget::_onWindowResizedEvent(spk::WindowResizedEvent &p_event) { (void)p_event; }
	void Widget::_onWindowFocusGainedEvent(spk::WindowFocusGainedEvent &p_event) { (void)p_event; }
	void Widget::_onWindowFocusLostEvent(spk::WindowFocusLostEvent &p_event) { (void)p_event; }
	void Widget::_onWindowShownEvent(spk::WindowShownEvent &p_event) { (void)p_event; }
	void Widget::_onWindowHiddenEvent(spk::WindowHiddenEvent &p_event) { (void)p_event; }
	void Widget::_onMouseEnteredEvent(spk::MouseEnteredWindowEvent &p_event) { (void)p_event; }
	void Widget::_onMouseLeftEvent(spk::MouseLeftWindowEvent &p_event) { (void)p_event; }
	void Widget::_onMouseMovedEvent(spk::MouseMovedEvent &p_event) { (void)p_event; }
	void Widget::_onMouseWheelScrolledEvent(spk::MouseWheelScrolledEvent &p_event) { (void)p_event; }
	void Widget::_onMouseButtonPressedEvent(spk::MouseButtonPressedEvent &p_event) { (void)p_event; }
	void Widget::_onMouseButtonReleasedEvent(spk::MouseButtonReleasedEvent &p_event) { (void)p_event; }
	void Widget::_onMouseButtonDoubleClickedEvent(spk::MouseButtonDoubleClickedEvent &p_event) { (void)p_event; }
	void Widget::_onKeyPressedEvent(spk::KeyPressedEvent &p_event) { (void)p_event; }
	void Widget::_onKeyReleasedEvent(spk::KeyReleasedEvent &p_event) { (void)p_event; }
	void Widget::_onTextInputEvent(spk::TextInputEvent &p_event) { (void)p_event; }

	Widget::Widget(const std::string &p_name, spk::Widget *p_parent) : spk::InherenceTrait<Widget>(p_parent),
																	   _name(p_name),
																	   _viewport(makeViewport())
	{
		_updateAbsoluteGeometryAndScissor();
	}

	Widget::~Widget() = default;

	const std::string &Widget::name() const
	{
		return (_name);
	}

	void Widget::setGeometry(const spk::Rect2D &p_geometry)
	{
		if (_geometry == p_geometry)
		{
			return;
		}

		_geometry = p_geometry;
		_updateAbsoluteGeometryAndScissor();
		invalidateRenderUnit();
	}

	void Widget::_updateAbsoluteGeometryAndScissor()
	{
		_absoluteGeometry = _geometry;
		if (parent() != nullptr)
		{
			_absoluteGeometry = _geometry.translated(parent()->absoluteGeometry().anchor);
			_scissor = _absoluteGeometry.intersect(parent()->scissor());
		}
		else
		{
			_scissor = _absoluteGeometry;
		}

		if (_viewport != nullptr)
		{
			_viewport->setGeometry(_absoluteGeometry);
			_viewport->setScissor(_scissor);
		}

		for (auto* child : children())
		{
			if (child != nullptr)
			{
				child->_updateAbsoluteGeometryAndScissor();
			}
		}
	}

	void Widget::invalidateRenderUnit() const
	{
		_renderCommandsDirty = true;
	}

	const spk::Rect2D &Widget::geometry() const
	{
		return (_geometry);
	}

	const spk::Rect2D& Widget::absoluteGeometry() const
	{
		return _absoluteGeometry;
	}

	const spk::Rect2D& Widget::scissor() const
	{
		return _scissor;
	}

	bool Widget::isRenderCommandDirty() const
	{
		return (_renderCommandsDirty);
	}

	const spk::Viewport& Widget::viewport() const
	{
		return *_viewport;
	}

	std::shared_ptr<spk::RenderUnit> Widget::renderUnit() const
	{
		if (_renderCommandsDirty == true || _renderUnit == nullptr)
		{
			spk::RenderUnit unit = _buildRenderUnit();
			_renderUnit = std::make_shared<spk::RenderUnit>(std::move(unit));
			_renderCommandsDirty = false;
		}

		return (_renderUnit);
	}

	void Widget::appendRenderUnits(spk::RenderSnapshotBuilder &p_builder) const
	{
		if (isActivated() == false)
		{
			return;
		}

		if (_viewport != nullptr &&
			_viewport->geometry().empty() == false &&
			_viewport->scissor().empty() == true)
		{
			return;
		}

		std::shared_ptr<spk::RenderUnit> viewportUnit = makeViewportUnit(_viewport.get());
		p_builder.append(viewportUnit);

		p_builder.append(renderUnit());

		for (const auto *child : children())
		{
			if (child != nullptr)
			{
				p_builder.append(viewportUnit);
				child->appendRenderUnits(p_builder);
			}
		}
	}

	void Widget::update(const spk::UpdateTick &p_tick)
	{
		if (isActivated() == false)
		{
			return;
		}

		for (auto *child : children())
		{
			if (child != nullptr)
			{
				child->update(p_tick);
			}
		}

		_onUpdate(p_tick);
	}

	bool Widget::dispatchFrameEvent(spk::FrameEventRecord &p_event)
	{
		return std::visit(
			spk::Overloaded{
				[this](const spk::WindowCloseRequestedRecord &p_record)
				{ spk::WindowCloseRequestedEvent event(p_record); _propagate(event, &spk::Widget::_onWindowCloseRequestedEvent); return event.isConsumed(); },
				[this](const spk::WindowDestroyedRecord &p_record)
				{ spk::WindowDestroyedEvent event(p_record); _propagate(event, &spk::Widget::_onWindowDestroyedEvent); return event.isConsumed(); },
				[this](const spk::WindowMovedRecord &p_record)
				{ spk::WindowMovedEvent event(p_record); _propagate(event, &spk::Widget::_onWindowMovedEvent); return event.isConsumed(); },
				[this](const spk::WindowResizedRecord &p_record)
				{ spk::WindowResizedEvent event(p_record); _propagate(event, &spk::Widget::_onWindowResizedEvent); return event.isConsumed(); },
				[this](const spk::WindowFocusGainedRecord &p_record)
				{ spk::WindowFocusGainedEvent event(p_record); _propagate(event, &spk::Widget::_onWindowFocusGainedEvent); return event.isConsumed(); },
				[this](const spk::WindowFocusLostRecord &p_record)
				{ spk::WindowFocusLostEvent event(p_record); _propagate(event, &spk::Widget::_onWindowFocusLostEvent); return event.isConsumed(); },
				[this](const spk::WindowShownRecord &p_record)
				{ spk::WindowShownEvent event(p_record); _propagate(event, &spk::Widget::_onWindowShownEvent); return event.isConsumed(); },
				[this](const spk::WindowHiddenRecord &p_record)
				{ spk::WindowHiddenEvent event(p_record); _propagate(event, &spk::Widget::_onWindowHiddenEvent); return event.isConsumed(); }},
			p_event);
	}

	void Widget::dispatchMouseEvent(spk::MouseEventRecord &p_event, spk::Mouse &p_mouse)
	{
		std::visit(
			spk::Overloaded{
				[this, &p_mouse](const spk::MouseEnteredRecord &p_record)
				{ spk::MouseEnteredWindowEvent event(p_record, p_mouse); _propagate(event, &spk::Widget::_onMouseEnteredEvent); },
				[this, &p_mouse](const spk::MouseLeftRecord &p_record)
				{ spk::MouseLeftWindowEvent event(p_record, p_mouse); _propagate(event, &spk::Widget::_onMouseLeftEvent); },
				[this, &p_mouse](const spk::MouseMovedRecord &p_record)
				{ spk::MouseMovedEvent event(p_record, p_mouse); _propagate(event, &spk::Widget::_onMouseMovedEvent); },
				[this, &p_mouse](const spk::MouseWheelScrolledRecord &p_record)
				{ spk::MouseWheelScrolledEvent event(p_record, p_mouse); _propagate(event, &spk::Widget::_onMouseWheelScrolledEvent); },
				[this, &p_mouse](const spk::MouseButtonPressedRecord &p_record)
				{ spk::MouseButtonPressedEvent event(p_record, p_mouse); _propagate(event, &spk::Widget::_onMouseButtonPressedEvent); },
				[this, &p_mouse](const spk::MouseButtonReleasedRecord &p_record)
				{ spk::MouseButtonReleasedEvent event(p_record, p_mouse); _propagate(event, &spk::Widget::_onMouseButtonReleasedEvent); },
				[this, &p_mouse](const spk::MouseButtonDoubleClickedRecord &p_record)
				{ spk::MouseButtonDoubleClickedEvent event(p_record, p_mouse); _propagate(event, &spk::Widget::_onMouseButtonDoubleClickedEvent); }},
			p_event);
	}

	void Widget::dispatchKeyboardEvent(spk::KeyboardEventRecord &p_event, spk::Keyboard &p_keyboard)
	{
		std::visit(
			spk::Overloaded{
				[this, &p_keyboard](const spk::KeyPressedRecord &p_record)
				{ spk::KeyPressedEvent event(p_record, p_keyboard); _propagate(event, &spk::Widget::_onKeyPressedEvent); },
				[this, &p_keyboard](const spk::KeyReleasedRecord &p_record)
				{ spk::KeyReleasedEvent event(p_record, p_keyboard); _propagate(event, &spk::Widget::_onKeyReleasedEvent); },
				[this, &p_keyboard](const spk::TextInputRecord &p_record)
				{ spk::TextInputEvent event(p_record, p_keyboard); _propagate(event, &spk::Widget::_onTextInputEvent); }},
			p_event);
	}
}
