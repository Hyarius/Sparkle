#pragma once

#include "battle/presentation/battle_hud_view_model.hpp"
#include "structures/widget/spk_push_button.hpp"

#include <functional>

namespace pg
{
	class AbilitySlotWidget final : public spk::Widget
	{
	private:
		spk::PushButton _button;
		spk::PushButton::Contract _clickContract;
		bool _enabled = false;
		std::optional<std::string> _abilityId;
		std::function<void()> _onEnabledClick;
		std::function<void()> _onDetailsClick;

		void _applyVisualState();

	protected:
		void _onGeometryChange() override;
		void _onMouseButtonReleasedEvent(spk::MouseButtonReleasedEvent &p_event) override;

	public:
		explicit AbilitySlotWidget(const std::string &p_name, spk::Widget *p_parent = nullptr);
		void render(const AbilitySlotViewModel &p_model);
		void setOnEnabledClick(std::function<void()> p_callback);
		void setOnDetailsClick(std::function<void()> p_callback);
		void invokeEnabledClick(); // keyboard-facing/widget-logic seam
		void invokeDetailsClick();
		[[nodiscard]] bool enabled() const noexcept;
	};
}
