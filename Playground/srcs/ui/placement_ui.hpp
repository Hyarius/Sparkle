#pragma once

#include "battle/phases/battle_phases.hpp"
#include "structures/widget/spk_panel.hpp"
#include "structures/widget/spk_push_button.hpp"
#include "structures/widget/spk_text_label.hpp"

#include <functional>

namespace pg
{
	class BattleUnit;

	class PlacementUI : public spk::Panel
	{
	private:
		spk::TextLabel _title;
		spk::TextLabel _instructions;
		spk::PushButton _autoPlace;
		spk::PushButton _confirm;
		spk::PushButton::Contract _autoContract;
		spk::PushButton::Contract _confirmContract;
		PlacementPhase *_phase = nullptr;
		PlacementPhase::ChangeContract _changeContract;
		std::function<void(BattleUnit *)> _selectionChanged;

		void _refresh();

	protected:
		void _onGeometryChange() override;

	public:
		explicit PlacementUI(const std::string &p_name, spk::Widget *p_parent = nullptr);

		void bind(PlacementPhase &p_phase);
		void unbind();
		[[nodiscard]] bool selectUnit(BattleUnit *p_unit);
		void setSelectionChangedHandler(std::function<void(BattleUnit *)> p_handler);
		[[nodiscard]] bool bound() const noexcept;
		[[nodiscard]] bool canConfirm() const noexcept;
		[[nodiscard]] BattleUnit *selected() const noexcept;
		[[nodiscard]] const spk::Font::Text &instructionText() const;
	};
}
