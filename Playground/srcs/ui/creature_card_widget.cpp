#include "ui/creature_card_widget.hpp"

#include "abilities/ability.hpp"
#include "battle/battle_unit.hpp"
#include "creatures/creature_species.hpp"
#include "creatures/creature_unit.hpp"
#include "statuses/status.hpp"

#include <sstream>
#include <utility>

namespace pg
{
	CreatureCardWidget::CreatureCardWidget(
		const std::string &p_name,
		std::shared_ptr<spk::SpriteSheet> p_icons,
		spk::Widget *p_parent) :
		spk::Panel(p_name, p_parent),
		_icons(std::move(p_icons)),
		_avatar(p_name + "/Avatar", this),
		_name(p_name + "/Name", this),
		_hp(p_name + "/HP", this),
		_turnBar(p_name + "/Turn", this),
		_details(p_name + "/Details", this)
	{
		_name.setTextSize(spk::Font::Size(14, 1));
		_details.setTextSize(spk::Font::Size(11, 1));
		_details.deactivate();
		if (_icons != nullptr) _avatar.setTexture(_icons);
		bind(static_cast<BattleUnit *>(nullptr));
	}

	void CreatureCardWidget::bind(BattleUnit *p_unit, bool p_battleVariant)
	{
		unbind();
		_unit = p_unit;
		_battleVariant = p_battleVariant;
		if (_unit == nullptr || _unit->source() == nullptr)
		{
			_name.setText("-");
			_avatar.deactivate();
			_turnBar.deactivate();
			return;
		}
		_name.setText(_unit->displayName());
		_refreshColor();
		if (_icons != nullptr)
		{
			const spk::Vector2Int avatar = _unit->source()->species->form(_unit->source()->currentFormId).avatar;
			_avatar.setTexture(_icons);
			_avatar.setSection(_icons->sprite({static_cast<unsigned int>(avatar.x), static_cast<unsigned int>(avatar.y)}));
			_avatar.activate();
		}
		_hp.bind(_unit->attributes.hp, "HP");
		if (_battleVariant)
		{
			_turnBar.bind(_unit->attributes.turnBar, "Turn");
			_turnBar.activate();
		}

		std::ostringstream details;
		details << "ATK " << _unit->attributes.attack.value()
			<< "  ARM " << _unit->attributes.armor.value()
			<< "  MAG " << _unit->attributes.magic.value()
			<< "  RES " << _unit->attributes.resistance.value() << '\n';
		details << "Abilities:";
		for (const Ability *ability : _unit->source()->abilities)
		{
			if (ability != nullptr) details << " " << ability->displayName;
		}
		details << "\nEffects:";
		for (const BattleStatus &status : _unit->statuses.entries())
		{
			if (status.definition != nullptr) details << " " << status.definition->displayName;
		}
		_expandedText = details.str();
		_details.setText(_expandedText);
		_refreshPresentation();
	}

	void CreatureCardWidget::bind(CreatureUnit *p_creature)
	{
		unbind();
		_creature = p_creature;
		_battleVariant = false;
		if (_creature == nullptr || _creature->species == nullptr)
		{
			_name.setText("-");
			_avatar.deactivate();
			return;
		}
		_name.setText(_creature->displayName());
		_refreshColor();
		if (_icons != nullptr)
		{
			const spk::Vector2Int avatar = _creature->species->form(_creature->currentFormId).avatar;
			_avatar.setSection(_icons->sprite({static_cast<unsigned int>(avatar.x), static_cast<unsigned int>(avatar.y)}));
			_avatar.activate();
		}
		_explorationHp.set(_creature->currentHealth, _creature->attributes.health, true);
		_hp.bind(_explorationHp, "HP");
		std::ostringstream details;
		details << "ATK " << _creature->attributes.attack
			<< "  ARM " << _creature->attributes.armor
			<< "  MAG " << _creature->attributes.magic
			<< "  RES " << _creature->attributes.resistance << '\n';
		details << "Abilities:";
		for (const Ability *ability : _creature->abilities)
		{
			if (ability != nullptr) details << " " << ability->displayName;
		}
		_expandedText = details.str();
		_details.setText(_expandedText);
		_refreshPresentation();
	}

	void CreatureCardWidget::unbind()
	{
		_hp.unbind();
		_turnBar.unbind();
		_unit = nullptr;
		_creature = nullptr;
		_hovered = false;
		_selected = false;
		_expandedText.clear();
		_details.setText("");
		_refreshPresentation();
	}

	void CreatureCardWidget::_refreshColor()
	{
		if (_unit == nullptr && _creature == nullptr) return;
		_name.setGlyphColor(_selected
			? spk::Color{1.0f, 0.9f, 0.25f, 1.0f}
			: (_unit == nullptr || _unit->side == BattleSide::Player
				? spk::Color{0.35f, 0.75f, 1.0f, 1.0f}
				: spk::Color{1.0f, 0.45f, 0.35f, 1.0f}));
	}

	void CreatureCardWidget::_refreshPresentation()
	{
		const bool hasCreature = _unit != nullptr || _creature != nullptr;
		const bool expanded = _hovered && hasCreature;
		if (expanded)
		{
			_avatar.deactivate();
			_name.deactivate();
			_hp.deactivate();
			_turnBar.deactivate();
			_details.activate();
		}
		else
		{
			_details.deactivate();
			_name.activate();
			if (hasCreature)
			{
				_hp.activate();
				if (_battleVariant) _turnBar.activate();
				if (_icons != nullptr) _avatar.activate();
			}
		}
	}

	void CreatureCardWidget::_onGeometryChange()
	{
		spk::Panel::_onGeometryChange();
		const unsigned int height = geometry().height();
		const unsigned int iconSize = std::min<unsigned int>(56, height > 8 ? height - 8 : 0);
		_avatar.setGeometry(spk::Rect2D(4, 4, iconSize, iconSize));
		const int contentX = static_cast<int>(iconSize) + 10;
		const unsigned int contentWidth = geometry().width() > static_cast<unsigned int>(contentX + 4)
			? geometry().width() - static_cast<unsigned int>(contentX + 4)
			: 0;
		_name.setGeometry(spk::Rect2D(contentX, 2, contentWidth, 21));
		_hp.setGeometry(spk::Rect2D(contentX, 24, contentWidth, 18));
		_turnBar.setGeometry(spk::Rect2D(contentX, 44, contentWidth, 16));
		_details.setGeometry(spk::Rect2D(5, 3, geometry().width() > 10 ? geometry().width() - 10 : 0, height > 6 ? height - 6 : 0));
	}

	void CreatureCardWidget::_onMouseMovedEvent(spk::MouseMovedEvent &p_event)
	{
		spk::Panel::_onMouseMovedEvent(p_event);
		const bool hovered = absoluteGeometry().contains(p_event->position);
		if (_hovered != hovered)
		{
			_hovered = hovered;
			_refreshPresentation();
		}
	}

	void CreatureCardWidget::_onMouseButtonPressedEvent(spk::MouseButtonPressedEvent &p_event)
	{
		spk::Panel::_onMouseButtonPressedEvent(p_event);
		if (_unit == nullptr || !absoluteGeometry().contains(p_event.device().position)) return;
		if (p_event->button == spk::Mouse::Left && _selectionHandler && _selectionHandler(_unit))
		{
			p_event.consume();
		}
		else if (p_event->button == spk::Mouse::Right && _infoHandler)
		{
			_infoHandler(_unit);
			p_event.consume();
		}
	}

	void CreatureCardWidget::setSelected(bool p_selected)
	{
		_selected = p_selected;
		_refreshColor();
	}
	void CreatureCardWidget::setSelectionHandler(std::function<bool(BattleUnit *)> p_handler)
	{
		_selectionHandler = std::move(p_handler);
	}
	void CreatureCardWidget::setInfoHandler(std::function<void(BattleUnit *)> p_handler)
	{
		_infoHandler = std::move(p_handler);
	}

	BattleUnit *CreatureCardWidget::unit() const noexcept { return _unit; }
	CreatureUnit *CreatureCardWidget::creature() const noexcept { return _creature; }
	bool CreatureCardWidget::selected() const noexcept { return _selected; }
	const std::string &CreatureCardWidget::expandedText() const noexcept { return _expandedText; }
	const ProgressBarWidget &CreatureCardWidget::hpBar() const noexcept { return _hp; }
	const ProgressBarWidget &CreatureCardWidget::turnBar() const noexcept { return _turnBar; }
}
