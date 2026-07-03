#pragma once

#include "battle/battle_side.hpp"
#include "structures/widget/spk_panel.hpp"
#include "structures/widget/spk_text_label.hpp"

#include <functional>

namespace pg
{
	class BattleBanner : public spk::Panel
	{
	private:
		spk::TextLabel _label;
		std::function<void()> _confirmation;
		float _elapsedSeconds = 0.0f;

		void _confirm();

	protected:
		void _onGeometryChange() override;
		void _onUpdate(const spk::UpdateTick &p_tick) override;
		void _onMouseButtonPressedEvent(spk::MouseButtonPressedEvent &p_event) override;

	public:
		static constexpr float AutoConfirmSeconds = 2.0f;

		explicit BattleBanner(const std::string &p_name, spk::Widget *p_parent = nullptr);

		void showResult(BattleSide p_winner, std::function<void()> p_confirmation);
		void hideResult();
		[[nodiscard]] bool showing() const noexcept;
	};
}
