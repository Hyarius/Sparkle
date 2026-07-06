#include "ui/turn_order_strip.hpp"

#include "battle/battle_context.hpp"
#include "battle/battle_unit.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace pg
{
	TurnOrderStrip::TurnOrderStrip(const std::string &p_name, spk::Widget *p_parent) :
		spk::Panel(p_name, p_parent),
		_title(p_name + "/Title", this)
	{
		_title.setText("Turn order");
		_title.setTextSize(spk::Font::Size(15, 1));
		for (std::size_t index = 0; index < Capacity; ++index)
		{
			_names[index] = std::make_unique<spk::TextLabel>(p_name + "/Name" + std::to_string(index + 1), this);
			_names[index]->setTextSize(spk::Font::Size(12, 1));
			_bars[index] = std::make_unique<ProgressBarWidget>(p_name + "/Bar" + std::to_string(index + 1), this);
			_names[index]->deactivate();
			_bars[index]->deactivate();
		}
	}

	float TurnOrderStrip::timeToReady(const BattleUnit &p_unit) noexcept
	{
		const float rate = p_unit.attributes.staminaRate.value();
		if (!p_unit.isActiveInBattle() || p_unit.hasStatusTag("stun") || rate <= 0.0f)
		{
			return std::numeric_limits<float>::infinity();
		}
		return std::max(0.0f, (p_unit.attributes.turnBar.max() - p_unit.attributes.turnBar.current()) / rate);
	}

	std::vector<BattleUnit *> TurnOrderStrip::sortedUnits(const BattleContext &p_context)
	{
		std::vector<BattleUnit *> result = p_context.allUnits();
		std::stable_sort(result.begin(), result.end(), [](const BattleUnit *p_left, const BattleUnit *p_right) {
			const float left = timeToReady(*p_left);
			const float right = timeToReady(*p_right);
			if (left != right) return left < right;
			if (p_left->side != p_right->side)
			{
				return p_left->side == BattleSide::Player;
			}
			return false;
		});
		return result;
	}

	void TurnOrderStrip::bind(BattleContext &p_context)
	{
		unbind();
		_context = &p_context;
		for (BattleUnit *unit : _context->allUnits())
		{
			_turnContracts.push_back(unit->attributes.turnBar.subscribe([this](const ObservableFloatResource &) {
				refresh();
			}));
			_rateContracts.push_back(unit->attributes.staminaRate.subscribe([this](const float &) {
				refresh();
			}));
		}
		refresh();
	}

	void TurnOrderStrip::unbind()
	{
		for (auto &contract : _turnContracts) contract.resign();
		for (auto &contract : _rateContracts) contract.resign();
		_turnContracts.clear();
		_rateContracts.clear();
		for (std::size_t index = 0; index < Capacity; ++index)
		{
			_bars[index]->unbind();
			_bars[index]->deactivate();
			_names[index]->deactivate();
		}
		_order.clear();
		_context = nullptr;
	}

	void TurnOrderStrip::refresh()
	{
		++_refreshCount;
		_order = _context != nullptr ? sortedUnits(*_context) : std::vector<BattleUnit *>{};
		const std::size_t visible = std::min(_order.size(), Capacity);
		for (std::size_t index = 0; index < Capacity; ++index)
		{
			if (index >= visible)
			{
				_bars[index]->unbind();
				_bars[index]->deactivate();
				_names[index]->deactivate();
				continue;
			}
			BattleUnit *unit = _order[index];
			_names[index]->setText(std::to_string(index + 1) + ". " + unit->displayName());
			_names[index]->setGlyphColor(unit->side == BattleSide::Player
				? spk::Color{0.35f, 0.75f, 1.0f, 1.0f}
				: spk::Color{1.0f, 0.45f, 0.35f, 1.0f});
			_names[index]->activate();
			_bars[index]->setValue(unit->attributes.turnBar.current(), unit->attributes.turnBar.max());
			_bars[index]->activate();
		}
	}

	void TurnOrderStrip::_onGeometryChange()
	{
		spk::Panel::_onGeometryChange();
		constexpr unsigned int titleHeight = 28;
		_title.setGeometry(spk::Rect2D(4, 2, geometry().width() > 8 ? geometry().width() - 8 : 0, titleHeight - 4));
		const unsigned int available = geometry().height() > titleHeight ? geometry().height() - titleHeight : 0;
		const unsigned int rowHeight = Capacity > 0 ? available / static_cast<unsigned int>(Capacity) : 0;
		for (std::size_t index = 0; index < Capacity; ++index)
		{
			const int y = static_cast<int>(titleHeight + index * rowHeight);
			_names[index]->setGeometry(spk::Rect2D(4, y, geometry().width() > 8 ? geometry().width() - 8 : 0, rowHeight / 2));
			_bars[index]->setGeometry(spk::Rect2D(4, y + static_cast<int>(rowHeight / 2), geometry().width() > 8 ? geometry().width() - 8 : 0, rowHeight / 2));
		}
	}

	const std::vector<BattleUnit *> &TurnOrderStrip::order() const noexcept { return _order; }
	std::size_t TurnOrderStrip::refreshCount() const noexcept { return _refreshCount; }
}
