#include "ui/passive_card_widget.hpp"

#include "abilities/effect.hpp"
#include "statuses/status.hpp"

#include <sstream>
#include <utility>

namespace
{
	std::string hookName(pg::StatusHookPoint p_hook)
	{
		switch (p_hook)
		{
		case pg::StatusHookPoint::TurnStart: return "At turn start";
		case pg::StatusHookPoint::TurnEnd: return "At turn end";
		case pg::StatusHookPoint::BeforeConsumingResources: return "Before paying costs";
		case pg::StatusHookPoint::AfterConsumingResources: return "After paying costs";
		case pg::StatusHookPoint::OnHPLoss: return "On HP loss";
		case pg::StatusHookPoint::OnAPLoss: return "On AP loss";
		case pg::StatusHookPoint::OnMPLoss: return "On MP loss";
		case pg::StatusHookPoint::OnHPGain: return "On HP gain";
		case pg::StatusHookPoint::OnAPGain: return "On AP gain";
		case pg::StatusHookPoint::OnMPGain: return "On MP gain";
		}
		return "When triggered";
	}
}

namespace pg
{
	PassiveCardWidget::PassiveCardWidget(
		const std::string &p_name,
		std::shared_ptr<spk::SpriteSheet> p_icons,
		spk::Widget *p_parent) :
		spk::Panel(p_name, p_parent),
		_icons(std::move(p_icons)),
		_icon(p_name + "/Icon", this),
		_name(p_name + "/Name", this),
		_rules(p_name + "/Rules", this)
	{
		_name.setTextSize(spk::Font::Size(17, 1));
		_rules.setTextSize(spk::Font::Size(13, 1));
		bind(nullptr);
	}

	std::string PassiveCardWidget::rulesText(const Status &p_status)
	{
		std::ostringstream result;
		result << hookName(p_status.hookPoint) << ": ";
		for (std::size_t index = 0; index < p_status.effects.size(); ++index)
		{
			if (p_status.effects[index] == nullptr) continue;
			if (index > 0) result << ' ';
			result << describe(*p_status.effects[index]);
		}
		return result.str();
	}

	void PassiveCardWidget::bind(const Status *p_status)
	{
		_status = p_status;
		if (_status == nullptr)
		{
			_name.setText("");
			_rules.setText("");
			_icon.deactivate();
			deactivate();
			return;
		}
		_name.setText(_status->displayName);
		_rules.setText(rulesText(*_status));
		if (_icons != nullptr)
		{
			_icon.setTexture(_icons);
			_icon.setSection(_icons->sprite({
				static_cast<unsigned int>(_status->icon[0]),
				static_cast<unsigned int>(_status->icon[1])}));
			_icon.activate();
		}
		activate();
	}

	void PassiveCardWidget::_onGeometryChange()
	{
		spk::Panel::_onGeometryChange();
		_icon.setGeometry(spk::Rect2D(6, 6, 40, 40));
		_name.setGeometry(spk::Rect2D(52, 4, geometry().width() > 58 ? geometry().width() - 58 : 0, 24));
		_rules.setGeometry(spk::Rect2D(52, 28, geometry().width() > 58 ? geometry().width() - 58 : 0, geometry().height() > 34 ? geometry().height() - 34 : 0));
	}

	const Status *PassiveCardWidget::status() const noexcept { return _status; }
}
