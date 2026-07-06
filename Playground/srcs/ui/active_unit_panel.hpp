#pragma once

#include "structures/widget/spk_panel.hpp"
#include "structures/widget/spk_push_button.hpp"
#include "structures/widget/spk_text_label.hpp"
#include "ui/ability_bar_widget.hpp"
#include "ui/progress_bar_widget.hpp"

#include <functional>
#include <memory>

namespace pg
{
	class BattleContext;
	class BattleUnit;

	class ActiveUnitPanel : public spk::Panel
	{
	private:
		spk::TextLabel _name;
		ProgressBarWidget _hp;
		ProgressBarWidget _ap;
		ProgressBarWidget _mp;
		AbilityBarWidget _abilities;
		spk::PushButton _endTurn;
		spk::PushButton::Contract _endTurnContract;
		BattleUnit *_unit = nullptr;
		std::function<void()> _endTurnHandler;

	protected:
		void _onGeometryChange() override;

	public:
		ActiveUnitPanel(
			const std::string &p_name,
			std::shared_ptr<spk::SpriteSheet> p_icons,
			spk::Widget *p_parent = nullptr);

		void bind(const BattleContext *p_context, BattleUnit *p_unit);
		void unbind();
		void refresh(const Ability *p_selected = nullptr);
		void setAbilitySelectionHandler(std::function<void(std::size_t)> p_handler);
		void setAbilityHoverHandler(std::function<void(const Ability *)> p_handler);
		void setEndTurnHandler(std::function<void()> p_handler);
		[[nodiscard]] BattleUnit *unit() const noexcept;
		[[nodiscard]] AbilityBarWidget &abilityBar() noexcept;
		[[nodiscard]] const AbilityBarWidget &abilityBar() const noexcept;
	};
}
