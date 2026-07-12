#include "structures/widget/spk_widget.hpp"

#include "structures/graphics/rendering/command/spk_viewport_render_command.hpp"
#include "structures/graphics/rendering/command/spk_render_unit_render_command.hpp"
#include "structures/graphics/rendering/pass/spk_render_pass_bucket_pack.hpp"
#include "structures/graphics/rendering/unit/spk_render_unit_builder.hpp"
#include "structures/widget/rendering/spk_widget_render_passes.hpp"
#include "structures/widget/rendering/spk_widget_render_priorities.hpp"
#include <algorithm>
#include <cmath>
#include <memory>

namespace spk
{
	namespace
	{
		std::unique_ptr<spk::Viewport> makeViewport()
		{
			return std::make_unique<spk::Viewport>();
		}

	}

	spk::RenderUnit Widget::_buildRenderUnit() const
	{
		return spk::RenderUnit();
	}

	void Widget::_contributeAdditionalRenderPasses(const spk::WidgetRenderBuildContext &) const
	{
	}

	void Widget::_onGeometryChange()
	{
	}

	void Widget::applyStyle(const spk::WidgetStyle &)
	{
	}

	void Widget::_onParentChanged(spk::Widget *p_oldParent, spk::Widget *p_newParent)
	{
		(void)p_oldParent;
		(void)p_newParent;
		_updateAbsoluteGeometryAndScissor();
	}

	void Widget::_onUpdate(const spk::UpdateContext &p_tick)
	{
		(void)p_tick;
	}

	void Widget::_onWindowCloseRequestedEvent(spk::WindowCloseRequestedEvent &p_event)
	{
		(void)p_event;
	}
	void Widget::_onWindowDestroyedEvent(spk::WindowDestroyedEvent &p_event)
	{
		(void)p_event;
	}
	void Widget::_onWindowMovedEvent(spk::WindowMovedEvent &p_event)
	{
		(void)p_event;
	}
	void Widget::_onWindowResizedEvent(spk::WindowResizedEvent &p_event)
	{
		(void)p_event;
	}
	void Widget::_onWindowFocusGainedEvent(spk::WindowFocusGainedEvent &p_event)
	{
		(void)p_event;
	}
	void Widget::_onWindowFocusLostEvent(spk::WindowFocusLostEvent &p_event)
	{
		(void)p_event;
	}
	void Widget::_onWindowShownEvent(spk::WindowShownEvent &p_event)
	{
		(void)p_event;
	}
	void Widget::_onWindowHiddenEvent(spk::WindowHiddenEvent &p_event)
	{
		(void)p_event;
	}
	void Widget::_onMouseEnteredEvent(spk::MouseEnteredWindowEvent &p_event)
	{
		(void)p_event;
	}
	void Widget::_onMouseLeftEvent(spk::MouseLeftWindowEvent &p_event)
	{
		(void)p_event;
	}
	void Widget::_onMouseMovedEvent(spk::MouseMovedEvent &p_event)
	{
		(void)p_event;
	}
	void Widget::_onMouseWheelScrolledEvent(spk::MouseWheelScrolledEvent &p_event)
	{
		(void)p_event;
	}
	void Widget::_onMouseButtonPressedEvent(spk::MouseButtonPressedEvent &p_event)
	{
		(void)p_event;
	}
	void Widget::_onMouseButtonReleasedEvent(spk::MouseButtonReleasedEvent &p_event)
	{
		(void)p_event;
	}
	void Widget::_onMouseButtonDoubleClickedEvent(spk::MouseButtonDoubleClickedEvent &p_event)
	{
		(void)p_event;
	}
	void Widget::_onKeyPressedEvent(spk::KeyPressedEvent &p_event)
	{
		(void)p_event;
	}
	void Widget::_onKeyReleasedEvent(spk::KeyReleasedEvent &p_event)
	{
		(void)p_event;
	}
	void Widget::_onTextInputEvent(spk::TextInputEvent &p_event)
	{
		(void)p_event;
	}

	Widget::Widget(const std::string &p_name, spk::Widget *p_parent) :
		spk::HierarchyTrait<Widget>(p_parent),
		_name(p_name),
		_viewport(makeViewport())
	{
		_updateAbsoluteGeometryAndScissor();
	}

	Widget::~Widget()
	{
		releaseAllFocus();
	}

	Widget *Widget::focusedWidget(FocusType p_focusType)
	{
		return _focusedWidgets[static_cast<int>(p_focusType)];
	}

	void Widget::takeFocus(FocusType p_focusType)
	{
		_focusedWidgets[static_cast<int>(p_focusType)] = this;
	}

	void Widget::releaseFocus(FocusType p_focusType)
	{
		int idx = static_cast<int>(p_focusType);
		if (_focusedWidgets[idx] == this)
		{
			_focusedWidgets[idx] = nullptr;
		}
	}

	bool Widget::hasFocus(FocusType p_focusType) const
	{
		return _focusedWidgets[static_cast<int>(p_focusType)] == this;
	}

	void Widget::takeAllFocus()
	{
		takeFocus(FocusType::Keyboard);
		takeFocus(FocusType::Mouse);
	}

	void Widget::releaseAllFocus()
	{
		releaseFocus(FocusType::Keyboard);
		releaseFocus(FocusType::Mouse);
	}

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
		_computeRatio();
		_updateAbsoluteGeometryAndScissor();
		_onGeometryChange();
		invalidateRenderUnit();
	}

	void Widget::refreshGeometry()
	{
		_computeRatio();
		_updateAbsoluteGeometryAndScissor();
		_onGeometryChange();
		invalidateRenderUnit();
	}

	void Widget::_onResize(const spk::Rect2D &p_geometry)
	{
		spk::HierarchyTrait<Widget>::HierarchyMutationGuard guard(this);

		_geometry = p_geometry;
		_updateSelfGeometryAndScissor();

		for (auto *child : children())
		{
			if (child == nullptr || child->_geometry.size.x == 0 || child->_geometry.size.y == 0)
			{
				continue;
			}

			child->_onResize(spk::Rect2D(
				spk::Vector2Int(
					static_cast<int>(std::lround(static_cast<float>(_geometry.size.x) * child->_anchorRatio.x)),
					static_cast<int>(std::lround(static_cast<float>(_geometry.size.y) * child->_anchorRatio.y))),
				spk::Vector2UInt(
					static_cast<unsigned int>(std::lround(static_cast<float>(_geometry.size.x) * child->_sizeRatio.x)),
					static_cast<unsigned int>(std::lround(static_cast<float>(_geometry.size.y) * child->_sizeRatio.y)))));
		}

		_onGeometryChange();
		invalidateRenderUnit();
	}

	void Widget::_computeRatio()
	{
		const spk::Vector2UInt referenceSize =
			(parent() != nullptr) ? parent()->_geometry.size : _geometry.size;

		_anchorRatio.x = (referenceSize.x != 0) ? static_cast<float>(_geometry.anchor.x) / static_cast<float>(referenceSize.x) : 0.0f;
		_anchorRatio.y = (referenceSize.y != 0) ? static_cast<float>(_geometry.anchor.y) / static_cast<float>(referenceSize.y) : 0.0f;
		_sizeRatio.x = (referenceSize.x != 0) ? static_cast<float>(_geometry.size.x) / static_cast<float>(referenceSize.x) : 1.0f;
		_sizeRatio.y = (referenceSize.y != 0) ? static_cast<float>(_geometry.size.y) / static_cast<float>(referenceSize.y) : 1.0f;
	}

	void Widget::place(const spk::Vector2Int &p_anchor)
	{
		setGeometry(spk::Rect2D(p_anchor, _geometry.size));
	}

	void Widget::move(const spk::Vector2Int &p_delta)
	{
		setGeometry(spk::Rect2D(_geometry.anchor + p_delta, _geometry.size));
	}

	void Widget::_updateSelfGeometryAndScissor()
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
	}

	void Widget::_updateAbsoluteGeometryAndScissor()
	{
		spk::HierarchyTrait<Widget>::HierarchyMutationGuard guard(this);

		_updateSelfGeometryAndScissor();

		for (auto *child : children())
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

		releaseSizeCache();
		for (Widget *ancestor = parent(); ancestor != nullptr; ancestor = ancestor->parent())
		{
			ancestor->releaseSizeCache();
		}
	}

	void Widget::invalidateRenderUnitTree() const
	{
		spk::HierarchyTrait<Widget>::HierarchyMutationGuard guard(this);

		invalidateRenderUnit();

		for (const auto *child : children())
		{
			if (child != nullptr)
			{
				child->invalidateRenderUnitTree();
			}
		}
	}

	const spk::Rect2D &Widget::geometry() const
	{
		return (_geometry);
	}

	const spk::Rect2D &Widget::absoluteGeometry() const
	{
		return _absoluteGeometry;
	}

	const spk::Rect2D &Widget::scissor() const
	{
		return _scissor;
	}

	bool Widget::isRenderCommandDirty() const
	{
		return (_renderCommandsDirty);
	}

	const spk::Viewport &Widget::viewport() const
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

	void Widget::_collectRenderPasses(const spk::WidgetRenderBuildContext &p_context) const
	{
		spk::HierarchyTrait<Widget>::HierarchyMutationGuard guard(this);

		if (isActivated() == false)
		{
			return;
		}
		_contributeAdditionalRenderPasses(p_context);

		if (_viewport != nullptr &&
			_viewport->geometry().empty() == false &&
			_viewport->scissor().empty() == true)
		{
			return;
		}

		const spk::Rect2D frameGeometry =
			(parent() != nullptr)
				? parent()->absoluteGeometry()
				: spk::Rect2D(
					  0, 0, static_cast<unsigned int>(std::max(_absoluteGeometry.right(), 1)), static_cast<unsigned int>(std::max(_absoluteGeometry.bottom(), 1)));

		auto &overlay = p_context.frame.passes.require({.type = spk::WidgetRenderPasses::Overlay, .scope = p_context.widgetScope});
		const std::size_t registrationOrder = (*p_context.nextRegistrationOrder)++;
		auto commands = overlay.contribute(spk::RenderContributionPriorities::Default, registrationOrder);
		if (!frameGeometry.empty() && !_scissor.empty())
		{
			commands.emplace<spk::ViewportCommand>(spk::Viewport(frameGeometry, _scissor));
		}
		std::shared_ptr<spk::RenderUnit> unit = renderUnit();
		if (unit != nullptr && !unit->empty())
		{
			commands.emplace<spk::RenderUnitRenderCommand>(std::move(unit));
		}

		for (const auto *child : children())
		{
			if (child != nullptr)
			{
				child->_collectRenderPasses(p_context);
			}
		}
	}

	void Widget::appendRenderUnits(spk::RenderSnapshotBuilder &p_builder) const
	{
		const spk::Rect2D frameGeometry =
			(parent() != nullptr)
				? parent()->absoluteGeometry()
				: spk::Rect2D(0, 0, static_cast<unsigned int>(std::max(_absoluteGeometry.right(), 1)), static_cast<unsigned int>(std::max(_absoluteGeometry.bottom(), 1)));
		appendRenderUnits(
			p_builder,
			spk::RenderTargetReference{.frameBuffer = nullptr, .viewport = spk::Viewport(frameGeometry), .activeTarget = true});
	}

	void Widget::appendRenderUnits(spk::RenderSnapshotBuilder &p_builder, const spk::RenderTargetReference &p_target) const
	{
		spk::RenderPlan plan = buildRenderPlan(p_target);
		p_builder.append(std::make_shared<spk::RenderUnit>(plan.compile()));
	}

	spk::RenderPlan Widget::buildRenderPlan(const spk::RenderTargetReference &p_target) const
	{
		spk::RenderPassBucketPack passes;
		spk::RenderFrameBuildContext frame{.passes = passes};
		passes.registerPassType(spk::WidgetRenderPasses::Overlay, "spk.widget.overlay");
		passes.emplacePass<spk::RenderPass>(
			{.type = spk::WidgetRenderPasses::Overlay, .scope = _renderScope},
			spk::WidgetRenderPriorities::Overlay,
			"WidgetOverlay",
			{.debugName = "WidgetOverlay", .target = p_target, .clear = {.color = spk::Color(0, 0, 0, 0), .depth = 1.0f, .stencil = 0}});
		std::size_t registrationOrder = 0;
		const spk::WidgetRenderBuildContext context{.frame = frame, .widgetScope = _renderScope, .nextRegistrationOrder = &registrationOrder};
		_collectRenderPasses(context);
		return passes.build();
	}

	void Widget::update(const spk::UpdateContext &p_tick)
	{
		spk::HierarchyTrait<Widget>::HierarchyMutationGuard guard(this);

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

	bool Widget::_dispatchFrameEvent(spk::FrameEventRecord &p_event)
	{
		return std::visit(
			spk::Overloaded{
				[this](const spk::WindowCloseRequestedRecord &p_record) {
					spk::WindowCloseRequestedEvent event(p_record);
					_propagate(event, &spk::Widget::_onWindowCloseRequestedEvent);
					return event.isConsumed();
				},
				[this](const spk::WindowDestroyedRecord &p_record) {
					spk::WindowDestroyedEvent event(p_record);
					_propagate(event, &spk::Widget::_onWindowDestroyedEvent);
					return event.isConsumed();
				},
				[this](const spk::WindowMovedRecord &p_record) {
					spk::WindowMovedEvent event(p_record);
					_propagate(event, &spk::Widget::_onWindowMovedEvent);
					return event.isConsumed();
				},
				[this](const spk::WindowResizedRecord &p_record) {
					spk::WindowResizedEvent event(p_record);
					_propagate(event, &spk::Widget::_onWindowResizedEvent);
					return event.isConsumed();
				},
				[this](const spk::WindowFocusGainedRecord &p_record) {
					spk::WindowFocusGainedEvent event(p_record);
					_propagate(event, &spk::Widget::_onWindowFocusGainedEvent);
					return event.isConsumed();
				},
				[this](const spk::WindowFocusLostRecord &p_record) {
					spk::WindowFocusLostEvent event(p_record);
					_propagate(event, &spk::Widget::_onWindowFocusLostEvent);
					return event.isConsumed();
				},
				[this](const spk::WindowShownRecord &p_record) {
					spk::WindowShownEvent event(p_record);
					_propagate(event, &spk::Widget::_onWindowShownEvent);
					return event.isConsumed();
				},
				[this](const spk::WindowHiddenRecord &p_record) {
					spk::WindowHiddenEvent event(p_record);
					_propagate(event, &spk::Widget::_onWindowHiddenEvent);
					return event.isConsumed();
				}},
			p_event);
	}

	void Widget::_dispatchMouseEvent(spk::MouseEventRecord &p_event, spk::Mouse &p_mouse)
	{
		Widget *focused = _focusedWidgets[static_cast<int>(FocusType::Mouse)];
		Widget *target = (focused != nullptr) ? focused : this;

		std::visit(
			spk::Overloaded{
				[target, &p_mouse](const spk::MouseEnteredRecord &p_record) {
					spk::MouseEnteredWindowEvent event(p_record, p_mouse);
					target->_propagate(event, &spk::Widget::_onMouseEnteredEvent);
				},
				[target, &p_mouse](const spk::MouseLeftRecord &p_record) {
					spk::MouseLeftWindowEvent event(p_record, p_mouse);
					target->_propagate(event, &spk::Widget::_onMouseLeftEvent);
				},
				[target, &p_mouse](const spk::MouseMovedRecord &p_record) {
					spk::MouseMovedEvent event(p_record, p_mouse);
					target->_propagate(event, &spk::Widget::_onMouseMovedEvent);
				},
				[target, &p_mouse](const spk::MouseWheelScrolledRecord &p_record) {
					spk::MouseWheelScrolledEvent event(p_record, p_mouse);
					target->_propagate(event, &spk::Widget::_onMouseWheelScrolledEvent);
				},
				[target, &p_mouse](const spk::MouseButtonPressedRecord &p_record) {
					spk::MouseButtonPressedEvent event(p_record, p_mouse);
					target->_propagate(event, &spk::Widget::_onMouseButtonPressedEvent);
				},
				[target, &p_mouse](const spk::MouseButtonReleasedRecord &p_record) {
					spk::MouseButtonReleasedEvent event(p_record, p_mouse);
					target->_propagate(event, &spk::Widget::_onMouseButtonReleasedEvent);
				},
				[target, &p_mouse](const spk::MouseButtonDoubleClickedRecord &p_record) {
					spk::MouseButtonDoubleClickedEvent event(p_record, p_mouse);
					target->_propagate(event, &spk::Widget::_onMouseButtonDoubleClickedEvent);
				}},
			p_event);
	}

	void Widget::_dispatchKeyboardEvent(spk::KeyboardEventRecord &p_event, spk::Keyboard &p_keyboard)
	{
		Widget *focused = _focusedWidgets[static_cast<int>(FocusType::Keyboard)];
		Widget *target = (focused != nullptr) ? focused : this;

		std::visit(
			spk::Overloaded{
				[target, &p_keyboard](const spk::KeyPressedRecord &p_record) {
					spk::KeyPressedEvent event(p_record, p_keyboard);
					target->_propagate(event, &spk::Widget::_onKeyPressedEvent);
				},
				[target, &p_keyboard](const spk::KeyReleasedRecord &p_record) {
					spk::KeyReleasedEvent event(p_record, p_keyboard);
					target->_propagate(event, &spk::Widget::_onKeyReleasedEvent);
				},
				[target, &p_keyboard](const spk::TextInputRecord &p_record) {
					spk::TextInputEvent event(p_record, p_keyboard);
					target->_propagate(event, &spk::Widget::_onTextInputEvent);
				}},
			p_event);
	}
}
