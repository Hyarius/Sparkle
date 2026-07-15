#include "widgets/battle/creature_card_widget.hpp"

#include "structures/widget/spk_widget_style.hpp"

#include <array>

namespace pg
{
	CreatureCardWidget::CreatureCardWidget(const std::string &p_name, spk::Widget *p_parent) :
		TeamRosterCard(p_name, p_parent),
		_summaryButton(p_name + "/summary", this),
		_detailsPanel(p_name + "/details", spk::WidgetStyle::Collection::style(spk::WidgetStyle::Collection::DefaultPressed), this)
	{
		for (std::size_t index = 0; index < 7; ++index)
		{
			_statistics.push_back(std::make_unique<spk::PushButton>(p_name + "/stat/" + std::to_string(index), this));
		}
		_selectionContract = _summaryButton.subscribeToClick([this] {
			if (_hasCreature && _onSelectedClick)
			{
				_onSelectedClick();
			}
		});
		_summaryButton.releasedLabel().setGlyphColor({1.0f, 1.0f, 1.0f, 1.0f});
		_summaryButton.pressedLabel().setGlyphColor({1.0f, 1.0f, 1.0f, 1.0f});
		for (const std::unique_ptr<spk::PushButton> &statistic : _statistics)
		{
			statistic->releasedLabel().setGlyphColor({1.0f, 1.0f, 1.0f, 1.0f});
			statistic->pressedLabel().setGlyphColor({1.0f, 1.0f, 1.0f, 1.0f});
		}
		_setDetailsVisible(false);
		activate();
	}

	void CreatureCardWidget::_setDetailsVisible(bool p_visible)
	{
		if (p_visible)
		{
			_detailsPanel.activate();
		}
		else
		{
			_detailsPanel.deactivate();
		}
		for (const std::unique_ptr<spk::PushButton> &statistic : _statistics)
		{
			if (p_visible)
			{
				statistic->activate();
			}
			else
			{
				statistic->deactivate();
			}
		}
	}

	void CreatureCardWidget::_applyLayout()
	{
		const std::size_t width = geometry().width();
		const std::size_t headerHeight = std::min<std::size_t>(40, geometry().height());
		_summaryButton.setGeometry({4, 4, width > 8 ? width - 8 : width, headerHeight > 8 ? headerHeight - 8 : headerHeight});
		if (!_expanded)
		{
			return;
		}
		const std::size_t detailY = headerHeight;
		const std::size_t detailHeight = geometry().height() > detailY + 4 ? geometry().height() - detailY - 4 : 0;
		const std::size_t detailWidth = width > 8 ? width - 8 : width;
		_detailsPanel.setGeometry({4, static_cast<int>(detailY), detailWidth, detailHeight});
		const std::size_t padding = 4;
		const std::size_t threeColumnWidth = detailWidth > padding * 4 ? (detailWidth - padding * 4) / 3 : 0;
		const std::size_t twoColumnWidth = detailWidth > padding * 3 ? (detailWidth - padding * 3) / 2 : 0;
		const std::size_t rowHeight = 27;
		for (std::size_t index = 0; index < 3; ++index)
		{
			_statistics[index]->setGeometry({static_cast<int>(4 + padding + index * (threeColumnWidth + padding)), static_cast<int>(detailY + padding), threeColumnWidth, rowHeight});
		}
		for (std::size_t index = 0; index < 2; ++index)
		{
			_statistics[3 + index]->setGeometry({static_cast<int>(4 + padding + index * (twoColumnWidth + padding)), static_cast<int>(detailY + padding * 2 + rowHeight), twoColumnWidth, rowHeight});
			_statistics[5 + index]->setGeometry({static_cast<int>(4 + padding + index * (twoColumnWidth + padding)), static_cast<int>(detailY + padding * 3 + rowHeight * 2), twoColumnWidth, rowHeight});
		}
	}

	void CreatureCardWidget::_onGeometryChange()
	{
		_applyLayout();
	}

	void CreatureCardWidget::_onMouseButtonReleasedEvent(spk::MouseButtonReleasedEvent &p_event)
	{
		if (p_event->button == spk::Mouse::Right && _hasCreature && absoluteGeometry().contains(p_event.device().position))
		{
			if (_onDetailsClick)
			{
				_onDetailsClick();
			}
			p_event.consume();
		}
	}

	void CreatureCardWidget::renderCard(const CreatureCardViewModel &p_model, bool p_expanded, bool p_actionable)
	{
		(void)p_actionable;
		_hasCreature = p_model.unit.has_value();
		_expanded = _hasCreature && p_expanded;
		_summaryButton.setText(_hasCreature ? p_model.displayName : "Empty");
		if (_hasCreature && p_model.icon.has_value())
		{
			const std::shared_ptr<spk::SpriteSheet> &iconset = spk::WidgetStyle::Collection::style(spk::WidgetStyle::Collection::Default).iconSpriteSheet();
			if (iconset != nullptr)
			{
				_summaryButton.setIcon(iconset, iconset->spriteID({(*p_model.icon)[0], (*p_model.icon)[1]}));
			}
			else
			{
				_summaryButton.removeIcon();
			}
		}
		else
		{
			_summaryButton.removeIcon();
		}
		const CreatureDetailsViewModel &details = p_model.details;
		const std::array<std::string, 7> labels = {
			"HP " + std::to_string(details.health.current) + "/" + std::to_string(details.health.maximum),
			"AP " + std::to_string(details.actionPoints.current) + "/" + std::to_string(details.actionPoints.maximum),
			"MP " + std::to_string(details.movementPoints.current) + "/" + std::to_string(details.movementPoints.maximum),
			"Attack " + std::to_string(details.strength),
			"Armor " + std::to_string(details.armor),
			"Magic " + std::to_string(details.magicPower),
			"Resistance " + std::to_string(details.resistance)};
		for (std::size_t index = 0; index < _statistics.size(); ++index)
		{
			_statistics[index]->setText(labels[index]);
		}
		_setDetailsVisible(_expanded);
		_applyLayout();
	}

	std::size_t CreatureCardWidget::rosterHeight() const noexcept
	{
		return _expanded ? 144 : 44;
	}

	void CreatureCardWidget::setOnSelectedClick(Callback p_callback)
	{
		_onSelectedClick = std::move(p_callback);
	}

	void CreatureCardWidget::setOnDetailsClick(Callback p_callback)
	{
		_onDetailsClick = std::move(p_callback);
	}
}
