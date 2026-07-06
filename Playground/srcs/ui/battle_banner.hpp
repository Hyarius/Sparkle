#pragma once

#include "battle/battle_side.hpp"
#include "structures/widget/spk_panel.hpp"
#include "structures/widget/spk_text_label.hpp"

#include <functional>
#include <optional>
#include <string>

namespace pg
{
	class BattleBanner : public spk::Panel
	{
	private:
		spk::TextLabel _label;
		std::function<void()> _confirmation;
		float _elapsedSeconds = 0.0f;
		float _autoHideSeconds = AutoConfirmSeconds;
		bool _notification = false;
		std::optional<BattleSide> _queuedResult;
		std::function<void()> _queuedConfirmation;

		void _confirm();

	protected:
		void _onGeometryChange() override;
		void _onUpdate(const spk::UpdateTick &p_tick) override;
		void _onMouseButtonPressedEvent(spk::MouseButtonPressedEvent &p_event) override;

	public:
		static constexpr float AutoConfirmSeconds = 2.0f;

		explicit BattleBanner(const std::string &p_name, spk::Widget *p_parent = nullptr);

		void showResult(BattleSide p_winner, std::function<void()> p_confirmation);
		void showMessage(std::string p_message, float p_seconds = AutoConfirmSeconds);
		void hideResult();
		[[nodiscard]] bool showing() const noexcept;
	};
}
