#include "ui/active_unit_panel.hpp"

#include "battle/battle_unit.hpp"

#include <utility>

namespace pg
{
	ActiveUnitPanel::ActiveUnitPanel(
		const std::string &p_name,
		std::shared_ptr<spk::SpriteSheet> p_icons,
		spk::Widget *p_parent) :
		spk::Panel(p_name, p_parent),
		_name(p_name + "/Name", this),
		_hp(p_name + "/HP", this),
		_ap(p_name + "/AP", this),
		_mp(p_name + "/MP", this),
		_abilities(p_name + "/Abilities", std::move(p_icons), this),
		_endTurn(p_name + "/EndTurn", "End Turn", this),
		_endTurnContract(_endTurn.subscribeToClick([this] {
			if (_endTurnHandler) _endTurnHandler();
		}))
	{
		_name.setTextSize(spk::Font::Size(18, 1));
		unbind();
	}

	void ActiveUnitPanel::bind(const BattleContext *p_context, BattleUnit *p_unit)
	{
		unbind();
		_unit = p_unit;
		if (_unit == nullptr)
		{
			return;
		}
		_name.setText(_unit->displayName());
		_hp.bind(_unit->attributes.hp, "HP");
		_ap.bind(_unit->attributes.ap, "AP");
		_mp.bind(_unit->attributes.mp, "MP");
		_abilities.bind(p_context, _unit);
		_hp.activate();
		_ap.activate();
		_mp.activate();
		_abilities.activate();
		if (_unit->side == BattleSide::Player) _endTurn.activate();
		else _endTurn.deactivate();
	}

	void ActiveUnitPanel::unbind()
	{
		_hp.unbind();
		_ap.unbind();
		_mp.unbind();
		_abilities.unbind();
		_hp.deactivate();
		_ap.deactivate();
		_mp.deactivate();
		_abilities.deactivate();
		_endTurn.deactivate();
		_name.setText("Waiting for turn");
		_unit = nullptr;
	}

	void ActiveUnitPanel::refresh(const Ability *p_selected)
	{
		_abilities.setSelected(p_selected);
	}

	void ActiveUnitPanel::setAbilitySelectionHandler(std::function<void(std::size_t)> p_handler)
	{
		_abilities.setSelectionHandler(std::move(p_handler));
	}

	void ActiveUnitPanel::setAbilityHoverHandler(std::function<void(const Ability *)> p_handler)
	{
		_abilities.setHoverHandler(std::move(p_handler));
	}

	void ActiveUnitPanel::setEndTurnHandler(std::function<void()> p_handler)
	{
		_endTurnHandler = std::move(p_handler);
	}

	void ActiveUnitPanel::_onGeometryChange()
	{
		spk::Panel::_onGeometryChange();
		const unsigned int width = geometry().width();
		const unsigned int resourceWidth = width > 170 ? (width - 170) / 3 : 0;
		_name.setGeometry(spk::Rect2D(8, 4, 150, 25));
		_hp.setGeometry(spk::Rect2D(160, 5, resourceWidth, 22));
		_ap.setGeometry(spk::Rect2D(162 + static_cast<int>(resourceWidth), 5, resourceWidth, 22));
		_mp.setGeometry(spk::Rect2D(164 + static_cast<int>(resourceWidth * 2), 5, resourceWidth, 22));
		const unsigned int endWidth = 100;
		const unsigned int barWidth = width > endWidth + 12 ? width - endWidth - 12 : 0;
		_abilities.setGeometry(spk::Rect2D(6, 34, barWidth, geometry().height() > 40 ? geometry().height() - 40 : 0));
		_endTurn.setGeometry(spk::Rect2D(static_cast<int>(barWidth + 8), 34, endWidth, geometry().height() > 40 ? geometry().height() - 40 : 0));
	}

	BattleUnit *ActiveUnitPanel::unit() const noexcept { return _unit; }
	AbilityBarWidget &ActiveUnitPanel::abilityBar() noexcept { return _abilities; }
	const AbilityBarWidget &ActiveUnitPanel::abilityBar() const noexcept { return _abilities; }
}
