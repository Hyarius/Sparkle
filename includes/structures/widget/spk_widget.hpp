#pragma once

#include <array>
#include <limits>
#include <memory>
#include <string>

#include "structures/design_pattern/spk_activable_trait.hpp"
#include "structures/design_pattern/spk_inherence_trait.hpp"
#include "structures/graphics/rendering/snapshot/spk_render_snapshot_builder.hpp"
#include "structures/graphics/rendering/state/spk_viewport.hpp"
#include "structures/graphics/rendering/unit/spk_render_unit_builder.hpp"
#include "structures/math/spk_rect_2d.hpp"
#include "structures/system/event/spk_events.hpp"
#include "structures/system/event/spk_update_context.hpp"
#include "structures/widget/spk_resizable_element.hpp"

namespace spk
{
	class WidgetStyle;
	class FrameModule;
	class MouseModule;
	class KeyboardModule;

	class Widget : public spk::HierarchyTrait<Widget>, public spk::ActivableTrait, public spk::ResizableElement
	{
	public:
		enum class FocusType
		{
			Keyboard = 0,
			Mouse = 1
		};

	private:
		friend class FrameModule;
		friend class MouseModule;
		friend class KeyboardModule;

		static inline std::array<Widget *, 2> _focusedWidgets = {nullptr, nullptr};

		std::string _name;
		mutable bool _renderCommandsDirty = true;
		mutable std::shared_ptr<spk::RenderUnit> _renderUnit = nullptr;
		spk::Rect2D _geometry;
		spk::Rect2D _absoluteGeometry;
		spk::Rect2D _scissor;

		spk::Vector2 _anchorRatio{0.0f, 0.0f};
		spk::Vector2 _sizeRatio{1.0f, 1.0f};

		mutable std::unique_ptr<spk::Viewport> _viewport;

		void _updateAbsoluteGeometryAndScissor();
		void _updateSelfGeometryAndScissor();
		void _computeRatio();

		void _onResize(const spk::Rect2D &p_geometry);

		template <typename TEvent>
		void _propagate(TEvent &p_event, void (spk::Widget::*p_handler)(TEvent &))
		{
			spk::HierarchyTrait<Widget>::HierarchyMutationGuard guard(this);

			if (isActivated() == false || p_event.isConsumed() == true)
			{
				return;
			}

			for (auto *child : children())
			{
				if (child != nullptr)
				{
					child->_propagate(p_event, p_handler);
					if (p_event.isConsumed() == true)
					{
						return;
					}
				}
			}

			(this->*p_handler)(p_event);
		}

		bool _dispatchFrameEvent(spk::FrameEventRecord &p_event);
		void _dispatchMouseEvent(spk::MouseEventRecord &p_event, spk::Mouse &p_mouse);
		void _dispatchKeyboardEvent(spk::KeyboardEventRecord &p_event, spk::Keyboard &p_keyboard);

	protected:
		void _onParentChanged(spk::Widget *p_oldParent, spk::Widget *p_newParent) override;

		[[nodiscard]] virtual spk::RenderUnit _buildRenderUnit() const;
		virtual void _onGeometryChange();

		virtual void _onUpdate(const spk::UpdateContext &p_tick);
		virtual void _onWindowCloseRequestedEvent(spk::WindowCloseRequestedEvent &p_event);
		virtual void _onWindowDestroyedEvent(spk::WindowDestroyedEvent &p_event);
		virtual void _onWindowMovedEvent(spk::WindowMovedEvent &p_event);
		virtual void _onWindowResizedEvent(spk::WindowResizedEvent &p_event);
		virtual void _onWindowFocusGainedEvent(spk::WindowFocusGainedEvent &p_event);
		virtual void _onWindowFocusLostEvent(spk::WindowFocusLostEvent &p_event);
		virtual void _onWindowShownEvent(spk::WindowShownEvent &p_event);
		virtual void _onWindowHiddenEvent(spk::WindowHiddenEvent &p_event);

		virtual void _onMouseEnteredEvent(spk::MouseEnteredWindowEvent &p_event);
		virtual void _onMouseLeftEvent(spk::MouseLeftWindowEvent &p_event);
		virtual void _onMouseMovedEvent(spk::MouseMovedEvent &p_event);
		virtual void _onMouseWheelScrolledEvent(spk::MouseWheelScrolledEvent &p_event);
		virtual void _onMouseButtonPressedEvent(spk::MouseButtonPressedEvent &p_event);
		virtual void _onMouseButtonReleasedEvent(spk::MouseButtonReleasedEvent &p_event);
		virtual void _onMouseButtonDoubleClickedEvent(spk::MouseButtonDoubleClickedEvent &p_event);

		virtual void _onKeyPressedEvent(spk::KeyPressedEvent &p_event);
		virtual void _onKeyReleasedEvent(spk::KeyReleasedEvent &p_event);
		virtual void _onTextInputEvent(spk::TextInputEvent &p_event);

	public:
		explicit Widget(const std::string &p_name, spk::Widget *p_parent = nullptr);
		~Widget() override;

		[[nodiscard]] const std::string &name() const;

		virtual void applyStyle(const spk::WidgetStyle &p_style);

		static Widget *focusedWidget(FocusType p_focusType);
		void takeFocus(FocusType p_focusType);
		void releaseFocus(FocusType p_focusType);
		[[nodiscard]] bool hasFocus(FocusType p_focusType) const;
		void takeAllFocus();
		void releaseAllFocus();

		void setGeometry(const spk::Rect2D &p_geometry) override;
		// Re-applies the current geometry, forcing an _onGeometryChange() pass even though the
		// rectangle is unchanged (setGeometry() short-circuits in that case). Use it when a
		// widget's internal content changed and it must re-lay-out itself in place -- e.g. a
		// container whose child labels gained text and need re-composing/re-fitting.
		void refreshGeometry();
		void place(const spk::Vector2Int &p_anchor);
		void move(const spk::Vector2Int &p_delta);

		[[nodiscard]] const spk::Rect2D &absoluteGeometry() const;
		[[nodiscard]] const spk::Rect2D &scissor() const;

		void invalidateRenderUnit() const;
		void invalidateRenderUnitTree() const;

		[[nodiscard]] const spk::Rect2D &geometry() const;
		[[nodiscard]] bool isRenderCommandDirty() const;
		[[nodiscard]] const spk::Viewport &viewport() const;

		[[nodiscard]] std::shared_ptr<spk::RenderUnit> renderUnit() const;
		void appendRenderUnits(spk::RenderSnapshotBuilder &p_builder) const;
		void update(const spk::UpdateContext &p_tick);
	};
}
