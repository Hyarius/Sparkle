#pragma once

#include "battle/presentation/battle_hud_view_model.hpp"
#include "structures/widget/spk_push_button.hpp"

#include <functional>

namespace pg
{
	// A compact status icon in the action bar. The view model retains authored icon coordinates;
	// the widget uses the configured UI icon atlas when available and preserves a short text
	// fallback for headless/default-style frontends.
	class StatusEffectSlotWidget final : public spk::Widget
	{
	private:
		spk::PushButton _button;
		StatusViewModel _model;
		std::function<void()> _onDetailsClick;

	protected:
		void _onGeometryChange() override;
		void _onMouseButtonReleasedEvent(spk::MouseButtonReleasedEvent &p_event) override;

	public:
		explicit StatusEffectSlotWidget(const std::string &p_name, spk::Widget *p_parent = nullptr);
		void render(const StatusViewModel &p_model);
		void setOnDetailsClick(std::function<void()> p_callback);
		void invokeDetailsClick();
	};
}
