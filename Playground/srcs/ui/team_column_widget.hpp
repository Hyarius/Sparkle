#pragma once

#include "battle/battle_side.hpp"
#include "structures/widget/spk_panel.hpp"
#include "structures/widget/spk_text_label.hpp"
#include "ui/creature_card_widget.hpp"

#include <array>
#include <functional>
#include <memory>
#include <vector>

namespace pg
{
	class BattleUnit;

	class TeamColumnWidget : public spk::Panel
	{
	public:
		static constexpr std::size_t Capacity = 6;

	private:
		spk::TextLabel _title;
		std::array<std::unique_ptr<CreatureCardWidget>, Capacity> _cards;
		std::size_t _unitCount = 0;

	protected:
		void _onGeometryChange() override;

	public:
		TeamColumnWidget(
			const std::string &p_name,
			std::shared_ptr<spk::SpriteSheet> p_icons,
			spk::Widget *p_parent = nullptr);

		void bind(const std::vector<BattleUnit *> &p_units, BattleSide p_side);
		void unbind();
		void setSelected(BattleUnit *p_unit);
		void setSelectionHandler(std::function<bool(BattleUnit *)> p_handler);
		void setInfoHandler(std::function<void(BattleUnit *)> p_handler);
		[[nodiscard]] std::size_t unitCount() const noexcept;
		[[nodiscard]] const CreatureCardWidget &card(std::size_t p_index) const;
	};
}
