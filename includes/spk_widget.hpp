#pragma once

#include <memory>
#include <string>

#include "spk_activable_trait.hpp"
#include "spk_events.hpp"
#include "spk_inherence_trait.hpp"
#include "spk_rect_2d.hpp"
#include "spk_render_snapshot_builder.hpp"
#include "spk_render_unit_builder.hpp"
#include "spk_update_tick.hpp"

namespace spk
{
	class Widget : public spk::InherenceTrait<Widget>, public spk::ActivableTrait
	{
	private:
		std::string _name;
		mutable bool _renderCommandsDirty = true;
		mutable std::shared_ptr<spk::RenderUnit> _renderUnit = nullptr;
		spk::Rect2D _geometry;

		template <typename TEvent>
		void _propagate(TEvent& p_event, void (spk::Widget::*p_handler)(TEvent&))
		{
			if (isActivated() == false || p_event.isConsumed() == true)
			{
				return;
			}

			for (auto* child : children())
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

	protected:
		[[nodiscard]] virtual spk::RenderUnit _buildRenderUnit() const;

		virtual void _onUpdate(const spk::UpdateTick& p_tick);
		virtual void _onWindowCloseRequestedEvent(spk::WindowCloseRequestedEvent& p_event);
		virtual void _onWindowDestroyedEvent(spk::WindowDestroyedEvent& p_event);
		virtual void _onWindowMovedEvent(spk::WindowMovedEvent& p_event);
		virtual void _onWindowResizedEvent(spk::WindowResizedEvent& p_event);
		virtual void _onWindowFocusGainedEvent(spk::WindowFocusGainedEvent& p_event);
		virtual void _onWindowFocusLostEvent(spk::WindowFocusLostEvent& p_event);
		virtual void _onWindowShownEvent(spk::WindowShownEvent& p_event);
		virtual void _onWindowHiddenEvent(spk::WindowHiddenEvent& p_event);

		virtual void _onMouseEnteredEvent(spk::MouseEnteredWindowEvent& p_event);
		virtual void _onMouseLeftEvent(spk::MouseLeftWindowEvent& p_event);
		virtual void _onMouseMovedEvent(spk::MouseMovedEvent& p_event);
		virtual void _onMouseWheelScrolledEvent(spk::MouseWheelScrolledEvent& p_event);
		virtual void _onMouseButtonPressedEvent(spk::MouseButtonPressedEvent& p_event);
		virtual void _onMouseButtonReleasedEvent(spk::MouseButtonReleasedEvent& p_event);
		virtual void _onMouseButtonDoubleClickedEvent(spk::MouseButtonDoubleClickedEvent& p_event);

		virtual void _onKeyPressedEvent(spk::KeyPressedEvent& p_event);
		virtual void _onKeyReleasedEvent(spk::KeyReleasedEvent& p_event);
		virtual void _onTextInputEvent(spk::TextInputEvent& p_event);

	public:
		explicit Widget(const std::string& p_name, spk::Widget* p_parent = nullptr);
		virtual ~Widget();

		[[nodiscard]] const std::string& name() const;

		void setGeometry(const spk::Rect2D& p_geometry);
		void invalidateRenderUnit();

		[[nodiscard]] const spk::Rect2D& geometry() const;
		[[nodiscard]] bool isRenderCommandDirty() const;

		[[nodiscard]] std::shared_ptr<spk::RenderUnit> renderUnit() const;
		void appendRenderUnits(spk::RenderSnapshotBuilder& p_builder) const;
		void update(const spk::UpdateTick& p_tick);

		bool dispatchFrameEvent(spk::FrameEventRecord& p_event);
		void dispatchMouseEvent(spk::MouseEventRecord& p_event, spk::Mouse& p_mouse);
		void dispatchKeyboardEvent(spk::KeyboardEventRecord& p_event, spk::Keyboard& p_keyboard);
	};
}
