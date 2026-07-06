#include "ui/team_column_widget.hpp"

#include "battle/battle_unit.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace pg
{
	TeamColumnWidget::TeamColumnWidget(
		const std::string &p_name,
		std::shared_ptr<spk::SpriteSheet> p_icons,
		spk::Widget *p_parent) :
		spk::Panel(p_name, p_parent),
		_title(p_name + "/Title", this)
	{
		_title.setTextSize(spk::Font::Size(17, 1));
		for (std::size_t index = 0; index < Capacity; ++index)
		{
			_cards[index] = std::make_unique<CreatureCardWidget>(
				p_name + "/Card" + std::to_string(index + 1), p_icons, this);
			_cards[index]->deactivate();
		}
	}

	void TeamColumnWidget::bind(const std::vector<BattleUnit *> &p_units, BattleSide p_side)
	{
		unbind();
		_unitCount = std::min(p_units.size(), Capacity);
		_title.setText(p_side == BattleSide::Player ? "Your team" : "Opponents");
		_title.setGlyphColor(p_side == BattleSide::Player
			? spk::Color{0.35f, 0.75f, 1.0f, 1.0f}
			: spk::Color{1.0f, 0.45f, 0.35f, 1.0f});
		for (std::size_t index = 0; index < _unitCount; ++index)
		{
			_cards[index]->bind(p_units[index]);
			_cards[index]->activate();
		}
	}

	void TeamColumnWidget::unbind()
	{
		for (auto &card : _cards)
		{
			card->unbind();
			card->deactivate();
		}
		_unitCount = 0;
	}

	void TeamColumnWidget::setSelected(BattleUnit *p_unit)
	{
		for (auto &card : _cards) card->setSelected(card->unit() == p_unit);
	}

	void TeamColumnWidget::setSelectionHandler(std::function<bool(BattleUnit *)> p_handler)
	{
		for (auto &card : _cards) card->setSelectionHandler(p_handler);
	}

	void TeamColumnWidget::setInfoHandler(std::function<void(BattleUnit *)> p_handler)
	{
		for (auto &card : _cards) card->setInfoHandler(p_handler);
	}

	void TeamColumnWidget::_onGeometryChange()
	{
		spk::Panel::_onGeometryChange();
		constexpr unsigned int titleHeight = 30;
		_title.setGeometry(spk::Rect2D(4, 2, geometry().width() > 8 ? geometry().width() - 8 : 0, titleHeight - 4));
		const unsigned int available = geometry().height() > titleHeight ? geometry().height() - titleHeight : 0;
		const unsigned int cardHeight = Capacity > 0 ? available / static_cast<unsigned int>(Capacity) : 0;
		for (std::size_t index = 0; index < Capacity; ++index)
		{
			_cards[index]->setGeometry(spk::Rect2D(
				4,
				static_cast<int>(titleHeight + index * cardHeight),
				geometry().width() > 8 ? geometry().width() - 8 : 0,
				cardHeight > 4 ? cardHeight - 4 : cardHeight));
		}
	}

	std::size_t TeamColumnWidget::unitCount() const noexcept { return _unitCount; }
	const CreatureCardWidget &TeamColumnWidget::card(std::size_t p_index) const
	{
		if (p_index >= Capacity) throw std::out_of_range("team card index out of range");
		return *_cards[p_index];
	}
}
