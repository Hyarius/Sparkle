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
		mutable std::shared_ptr<const spk::RenderUnit> _renderUnit = nullptr;
		spk::Rect2D _geometry;

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

		void _propagateWindowCloseRequestedEvent(spk::WindowCloseRequestedEvent& p_event);
		void _propagateWindowDestroyedEvent(spk::WindowDestroyedEvent& p_event);
		void _propagateWindowMovedEvent(spk::WindowMovedEvent& p_event);
		void _propagateWindowResizedEvent(spk::WindowResizedEvent& p_event);
		void _propagateWindowFocusGainedEvent(spk::WindowFocusGainedEvent& p_event);
		void _propagateWindowFocusLostEvent(spk::WindowFocusLostEvent& p_event);
		void _propagateWindowShownEvent(spk::WindowShownEvent& p_event);
		void _propagateWindowHiddenEvent(spk::WindowHiddenEvent& p_event);

		void _propagateMouseEnteredEvent(spk::MouseEnteredWindowEvent& p_event);
		void _propagateMouseLeftEvent(spk::MouseLeftWindowEvent& p_event);
		void _propagateMouseMovedEvent(spk::MouseMovedEvent& p_event);
		void _propagateMouseWheelScrolledEvent(spk::MouseWheelScrolledEvent& p_event);
		void _propagateMouseButtonPressedEvent(spk::MouseButtonPressedEvent& p_event);
		void _propagateMouseButtonReleasedEvent(spk::MouseButtonReleasedEvent& p_event);
		void _propagateMouseButtonDoubleClickedEvent(spk::MouseButtonDoubleClickedEvent& p_event);

		void _propagateKeyPressedEvent(spk::KeyPressedEvent& p_event);
		void _propagateKeyReleasedEvent(spk::KeyReleasedEvent& p_event);
		void _propagateTextInputEvent(spk::TextInputEvent& p_event);

	public:
		explicit Widget(const std::string& p_name, spk::Widget* p_parent = nullptr);
		virtual ~Widget();

		[[nodiscard]] const std::string& name() const;

		void setGeometry(const spk::Rect2D& p_geometry);
		void invalidateRenderUnit();

		[[nodiscard]] const spk::Rect2D& geometry() const;
		[[nodiscard]] bool isRenderCommandDirty() const;

		[[nodiscard]] std::shared_ptr<const spk::RenderUnit> renderUnit() const;
		void appendRenderUnits(spk::RenderSnapshotBuilder& p_builder) const;
		void update(const spk::UpdateTick& p_tick);

		bool dispatchFrameEvent(spk::FrameEventRecord& p_event);
		void dispatchMouseEvent(spk::MouseEventRecord& p_event, spk::Mouse& p_mouse);
		void dispatchKeyboardEvent(spk::KeyboardEventRecord& p_event, spk::Keyboard& p_keyboard);
	};
}
