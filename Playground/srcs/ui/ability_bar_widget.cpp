#include "ui/ability_bar_widget.hpp"

#include "battle/battle_context.hpp"
#include "battle/battle_unit.hpp"
#include "battle/rules/battle_action_validator.hpp"
#include "creatures/creature_unit.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace pg
{
	AbilityBarWidget::AbilityBarWidget(
		const std::string &p_name,
		std::shared_ptr<spk::SpriteSheet> p_icons,
		spk::Widget *p_parent) :
		spk::Panel(p_name, p_parent),
		_previous(p_name + "/Previous", "-", this),
		_next(p_name + "/Next", "+", this),
		_pageLabel(p_name + "/Page", this),
		_previousContract(_previous.subscribeToClick([this] {
			if (_page > 0) setPage(_page - 1);
		})),
		_nextContract(_next.subscribeToClick([this] {
			if (_page + 1 < pageCount()) setPage(_page + 1);
		}))
	{
		for (std::size_t index = 0; index < PageSize; ++index)
		{
			_slots[index] = std::make_unique<AbilitySlotWidget>(
				p_name + "/Slot" + std::to_string(index + 1), index, p_icons, this);
			_slots[index]->setSelectionHandler([this](std::size_t p_localIndex) {
				const std::size_t globalIndex = _page * PageSize + p_localIndex;
				if (_selection) _selection(globalIndex);
			});
		}
		_pageLabel.setTextSize(spk::Font::Size(12, 1));
		refresh();
	}

	std::size_t AbilityBarWidget::pageCountFor(std::size_t p_abilityCount) noexcept
	{
		return std::max<std::size_t>(1, (p_abilityCount + PageSize - 1) / PageSize);
	}

	void AbilityBarWidget::bind(const BattleContext *p_context, BattleUnit *p_unit)
	{
		_context = p_context;
		_unit = p_unit;
		_page = 0;
		_selected = nullptr;
		refresh();
	}

	void AbilityBarWidget::unbind()
	{
		_context = nullptr;
		_unit = nullptr;
		_selected = nullptr;
		_page = 0;
		refresh();
	}

	void AbilityBarWidget::setSelectionHandler(std::function<void(std::size_t)> p_selection)
	{
		_selection = std::move(p_selection);
	}

	void AbilityBarWidget::setHoverHandler(std::function<void(const Ability *)> p_hover)
	{
		for (auto &slot : _slots) slot->setHoverHandler(p_hover);
	}

	void AbilityBarWidget::setSelected(const Ability *p_selected)
	{
		_selected = p_selected;
		refresh();
	}

	void AbilityBarWidget::setPage(std::size_t p_page)
	{
		_page = std::min(p_page, pageCount() - 1);
		refresh();
	}

	void AbilityBarWidget::refresh()
	{
		const std::size_t count = (_unit != nullptr && _unit->source() != nullptr)
			? _unit->source()->abilities.size()
			: 0;
		_page = std::min(_page, pageCountFor(count) - 1);
		for (std::size_t local = 0; local < PageSize; ++local)
		{
			const std::size_t global = _page * PageSize + local;
			const Ability *ability = global < count ? _unit->source()->abilities[global] : nullptr;
			const bool enabled = ability != nullptr && _context != nullptr && _unit->side == BattleSide::Player &&
				_unit->isActiveInBattle() && BattleActionValidator::canUseAbility(*_context, *_unit, *ability);
			_slots[local]->setAbility(ability, enabled, ability == _selected);
		}
		_pageLabel.setText(std::to_string(_page + 1) + "/" + std::to_string(pageCountFor(count)));
	}

	void AbilityBarWidget::_onGeometryChange()
	{
		spk::Panel::_onGeometryChange();
		const unsigned int selectorWidth = 70;
		const unsigned int slotsWidth = geometry().width() > selectorWidth ? geometry().width() - selectorWidth : 0;
		const unsigned int slotWidth = slotsWidth / static_cast<unsigned int>(PageSize);
		for (std::size_t index = 0; index < PageSize; ++index)
		{
			_slots[index]->setGeometry(spk::Rect2D(
				static_cast<int>(index * slotWidth), 0, slotWidth > 2 ? slotWidth - 2 : slotWidth, geometry().height()));
		}
		const int selectorX = static_cast<int>(slotsWidth);
		_previous.setGeometry(spk::Rect2D(selectorX, 0, 22, geometry().height()));
		_pageLabel.setGeometry(spk::Rect2D(selectorX + 22, 0, 26, geometry().height()));
		_next.setGeometry(spk::Rect2D(selectorX + 48, 0, 22, geometry().height()));
	}

	std::size_t AbilityBarWidget::page() const noexcept { return _page; }
	std::size_t AbilityBarWidget::pageCount() const noexcept
	{
		return pageCountFor((_unit != nullptr && _unit->source() != nullptr) ? _unit->source()->abilities.size() : 0);
	}
	const AbilitySlotWidget &AbilityBarWidget::slot(std::size_t p_index) const
	{
		if (p_index >= PageSize) throw std::out_of_range("ability slot index out of range");
		return *_slots[p_index];
	}
}
