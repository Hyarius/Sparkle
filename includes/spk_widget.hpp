#pragma once

#include <string>

#include "spk_activable_trait.hpp"
#include "spk_events.hpp"
#include "spk_inherence_trait.hpp"
#include "spk_rect_2d.hpp"
#include "spk_update_tick.hpp"

namespace spk
{
	class Widget : public spk::InherenceTrait<Widget>, public spk::ActivableTrait
	{
	private:
		std::string _name;
		bool _geometryChangeRequested = false;
		spk::Rect2D _geometry;

	protected:
		virtual void _onGeometryUpdate();
		virtual void _onRender();
		virtual void _onUpdate(const spk::UpdateTick& p_tick);
		virtual void _onFrameEvent(const spk::Event& p_event);
		virtual void _onMouseEvent(const spk::Event& p_event);
		virtual void _onKeyboardEvent(const spk::Event& p_event);

		void _propagateFrameEvent(const spk::Event& p_event);
		void _propagateMouseEvent(const spk::Event& p_event);
		void _propagateKeyboardEvent(const spk::Event& p_event);
		void _updateGeometry();

	public:
		explicit Widget(const std::string& p_name, spk::Widget* p_parent = nullptr);
		virtual ~Widget();

		[[nodiscard]] const std::string& name() const;

		void setGeometry(const spk::Rect2D& p_geometry);
		void requireGeometryUpdate();
		void updateGeometry();

		[[nodiscard]] const spk::Rect2D& geometry() const;

		void render();
		void update(const spk::UpdateTick& p_tick);

		void dispatchFrameEvent(const spk::Event& p_event);
		void dispatchMouseEvent(const spk::Event& p_event);
		void dispatchKeyboardEvent(const spk::Event& p_event);
	};
}
