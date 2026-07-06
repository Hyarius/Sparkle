#pragma once

#include "battle/battle_side.hpp"
#include "structures/graphics/texture/spk_sprite_sheet.hpp"
#include "structures/widget/spk_image_label.hpp"
#include "structures/widget/spk_panel.hpp"
#include "structures/widget/spk_text_label.hpp"
#include "ui/progress_bar_widget.hpp"
#include "core/observable_resource.hpp"

#include <memory>
#include <functional>
#include <string>

namespace pg
{
	class BattleUnit;
	class CreatureUnit;

	class CreatureCardWidget : public spk::Panel
	{
	private:
		std::shared_ptr<spk::SpriteSheet> _icons;
		spk::ImageLabel _avatar;
		spk::TextLabel _name;
		ProgressBarWidget _hp;
		ProgressBarWidget _turnBar;
		spk::TextLabel _details;
		BattleUnit *_unit = nullptr;
		CreatureUnit *_creature = nullptr;
		ObservableResource _explorationHp;
		bool _battleVariant = true;
		bool _hovered = false;
		bool _selected = false;
		std::string _expandedText;
		std::function<bool(BattleUnit *)> _selectionHandler;
		std::function<void(BattleUnit *)> _infoHandler;

		void _refreshPresentation();
		void _refreshColor();

	protected:
		void _onGeometryChange() override;
		void _onMouseMovedEvent(spk::MouseMovedEvent &p_event) override;
		void _onMouseButtonPressedEvent(spk::MouseButtonPressedEvent &p_event) override;

	public:
		CreatureCardWidget(
			const std::string &p_name,
			std::shared_ptr<spk::SpriteSheet> p_icons,
			spk::Widget *p_parent = nullptr);

		void bind(BattleUnit *p_unit, bool p_battleVariant = true);
		void bind(CreatureUnit *p_creature);
		void unbind();
		void setSelected(bool p_selected);
		void setSelectionHandler(std::function<bool(BattleUnit *)> p_handler);
		void setInfoHandler(std::function<void(BattleUnit *)> p_handler);
		[[nodiscard]] BattleUnit *unit() const noexcept;
		[[nodiscard]] CreatureUnit *creature() const noexcept;
		[[nodiscard]] bool selected() const noexcept;
		[[nodiscard]] const std::string &expandedText() const noexcept;
		[[nodiscard]] const ProgressBarWidget &hpBar() const noexcept;
		[[nodiscard]] const ProgressBarWidget &turnBar() const noexcept;
	};
}
