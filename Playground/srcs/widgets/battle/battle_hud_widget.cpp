#include "widgets/battle/battle_hud_widget.hpp"

#include "battle/presentation/battle_interaction_controller.hpp"
#include "widgets/battle/ability_slot_widget.hpp"
#include "widgets/battle/creature_card_widget.hpp"
#include "widgets/battle/resource_bar_widget.hpp"
#include "widgets/battle/status_effect_slot_widget.hpp"

#include <algorithm>

namespace
{
	[[nodiscard]] spk::Rect2D rect(int p_x, int p_y, std::size_t p_width, std::size_t p_height)
	{
		return {p_x, p_y, p_width, p_height};
	}

	void useWhiteText(spk::PushButton &p_button)
	{
		const spk::Color white{1.0f, 1.0f, 1.0f, 1.0f};
		p_button.releasedLabel().setGlyphColor(white);
		p_button.pressedLabel().setGlyphColor(white);
	}

	void useWhiteText(spk::MessageBox &p_window)
	{
		const spk::Color white{1.0f, 1.0f, 1.0f, 1.0f};
		p_window.textArea().setGlyphColor(white);
		p_window.menuBar().titleLabel().setGlyphColor(white);
		useWhiteText(p_window.menuBar().closeButton());
		useWhiteText(p_window.menuBar().minimizeButton());
		useWhiteText(p_window.menuBar().maximizeButton());
	}

	[[nodiscard]] const std::string &effectName(const pg::StatusViewModel &p_effect)
	{
		return p_effect.displayName.empty() ? p_effect.id : p_effect.displayName;
	}
}

namespace pg
{
	BattleHudWidget::BattleHudWidget(const std::string &p_name, spk::Widget *p_parent) :
		spk::Widget(p_name, p_parent),
		_playerRoster(p_name + "/player", this),
		_enemyRoster(p_name + "/enemy", this),
		_activePanel(p_name + "/actionBar", this),
		_statusPanel(p_name + "/status/panel", this),
		_status(p_name + "/status", "", this),
		_forecast(p_name + "/forecast", "", this),
		_contextButton(p_name + "/actionBar/context", "Ready", &_activePanel)
	{
		_health = std::make_unique<ResourceBarWidget>(p_name + "/actionBar/hp", &_activePanel);
		_actionPoints = std::make_unique<ResourceBarWidget>(p_name + "/actionBar/ap", &_activePanel);
		_movementPoints = std::make_unique<ResourceBarWidget>(p_name + "/actionBar/mp", &_activePanel);
		for (std::size_t index = 0; index < 8; ++index)
		{
			_abilities.push_back(std::make_unique<AbilitySlotWidget>(p_name + "/actionBar/ability/" + std::to_string(index), &_activePanel));
		}
		// Inspection windows are created after persistent HUD controls so they stay above them in
		// both rendering and hit-testing order.
		_creatureDetailsWindow = std::make_unique<spk::MessageBox>(p_name + "/creatureDetails", this);
		_abilityDetailsWindow = std::make_unique<spk::MessageBox>(p_name + "/abilityDetails", this);
		_effectDetailsWindow = std::make_unique<spk::MessageBox>(p_name + "/effectDetails", this);
		_creatureDetailsWindow->setMinimalWidth(340);
		_abilityDetailsWindow->setMinimalWidth(340);
		_effectDetailsWindow->setMinimalWidth(300);
		_contextContract = _contextButton.subscribeToClick([this] {
			if (_interaction == nullptr || !_model.has_value())
			{
				return;
			}
			if (_model->phase == BattlePhase::Deployment)
			{
				if (_model->deployment.readyEnabled)
				{
					(void)_interaction->confirmDeployment();
				}
				return;
			}
			if (_model->phase == BattlePhase::Activation && _model->activeUnit.has_value() && _model->activeUnit->side == BattleSide::Player)
			{
				(void)_interaction->endTurn();
			}
		});
		const spk::Color white{1.0f, 1.0f, 1.0f, 1.0f};
		_status.setGlyphColor(white);
		_forecast.setGlyphColor(white);
		useWhiteText(_contextButton);
		useWhiteText(*_creatureDetailsWindow);
		useWhiteText(*_abilityDetailsWindow);
		useWhiteText(*_effectDetailsWindow);
		_creatureDetailsWindow->deactivate();
		_abilityDetailsWindow->deactivate();
		_effectDetailsWindow->deactivate();
		_playerRoster.setOnPreferredHeightChanged([this] {
			_layout();
		});
		_enemyRoster.setOnPreferredHeightChanged([this] {
			_layout();
		});
		deactivate();
	}

	BattleHudWidget::~BattleHudWidget() = default;

	void BattleHudWidget::_bindCallbacks()
	{
		_playerRoster.setOnCardSelected([this](std::size_t p_index) {
			if (_interaction == nullptr || !_model.has_value() || p_index >= _model->playerTeam.size())
			{
				return;
			}
			const CreatureCardViewModel &card = _model->playerTeam[p_index];
			if (card.creature.has_value())
			{
				(void)_interaction->selectPlacementCreature(*card.creature);
			}
		});
		_playerRoster.setOnCardDetails([this](std::size_t p_index) {
			if (_model.has_value() && p_index < _model->playerTeam.size())
			{
				_openDetails(_model->playerTeam[p_index]);
			}
		});
		_enemyRoster.setOnCardDetails([this](std::size_t p_index) {
			if (_model.has_value() && p_index < _model->opponentTeam.size())
			{
				_openDetails(_model->opponentTeam[p_index]);
			}
		});
		for (std::size_t index = 0; index < _abilities.size(); ++index)
		{
			_abilities[index]->setOnEnabledClick([this, index] {
				if (_interaction != nullptr)
				{
					(void)_interaction->selectAbility(index);
				}
			});
			_abilities[index]->setOnDetailsClick([this, index] {
				if (_model.has_value() && index < _model->abilities.size())
				{
					_openAbilityDetails(_model->abilities[index]);
				}
			});
		}
	}

	void BattleHudWidget::bind(BattleInteractionController &p_interaction)
	{
		_interaction = &p_interaction;
		// A scene can enter battle before its next window-resize notification. In that case the
		// child still has a construction-time 0x0 geometry and would be clipped despite being
		// activated, so adopt the current parent extent immediately.
		if ((geometry().width() == 0 || geometry().height() == 0) && parent() != nullptr)
		{
			setGeometry({{0, 0}, parent()->geometry().width(), parent()->geometry().height()});
		}
		_bindCallbacks();
		activate();
	}

	void BattleHudWidget::unbind() noexcept
	{
		_interaction = nullptr;
		_model.reset();
		if (_creatureDetailsWindow != nullptr)
		{
			_creatureDetailsWindow->deactivate();
		}
		if (_abilityDetailsWindow != nullptr)
		{
			_abilityDetailsWindow->deactivate();
		}
		if (_effectDetailsWindow != nullptr)
		{
			_effectDetailsWindow->deactivate();
		}
		deactivate();
	}

	void BattleHudWidget::render(const BattleHudViewModel &p_model)
	{
		if (_model.has_value() && *_model == p_model)
		{
			return;
		}
		_model = p_model;
		const bool deployment = p_model.phase == BattlePhase::Deployment;
		_playerRoster.render(p_model.playerTeam, deployment);
		_enemyRoster.render(p_model.opponentTeam, false);
		_status.setText(p_model.statusLine);
		std::string forecast = "Next:";
		for (const TurnForecastEntryViewModel &entry : p_model.forecast)
		{
			forecast += " " + entry.shortName;
		}
		_forecast.setText(forecast);

		const bool hasActionBar = p_model.actionBar.has_value();
		if (hasActionBar)
		{
			_health->render(p_model.actionBar->health);
			_actionPoints->render(p_model.actionBar->actionPoints);
			_movementPoints->render(p_model.actionBar->movementPoints);
			_health->activate();
			_actionPoints->activate();
			_movementPoints->activate();
			while (_effects.size() < p_model.actionBar->effects.size())
			{
				const std::size_t index = _effects.size();
				auto effect = std::make_unique<StatusEffectSlotWidget>("effect/" + std::to_string(index), &_activePanel);
				effect->setOnDetailsClick([this, index] {
					if (_model.has_value() && _model->actionBar.has_value() && index < _model->actionBar->effects.size())
					{
						_openEffectDetails(_model->actionBar->effects[index]);
					}
				});
				effect->render(p_model.actionBar->effects[index]);
				_effects.push_back(std::move(effect));
			}
			while (_effects.size() > p_model.actionBar->effects.size())
			{
				_effects.pop_back();
			}
			for (std::size_t index = 0; index < _effects.size(); ++index)
			{
				_effects[index]->render(p_model.actionBar->effects[index]);
			}
		}
		else
		{
			_health->deactivate();
			_actionPoints->deactivate();
			_movementPoints->deactivate();
			_effects.clear();
		}

		const bool showAbilities = p_model.phase == BattlePhase::Activation && hasActionBar;
		for (std::size_t index = 0; index < _abilities.size(); ++index)
		{
			_abilities[index]->render(p_model.abilities[index]);
			if (showAbilities)
			{
				_abilities[index]->activate();
			}
			else
			{
				_abilities[index]->deactivate();
			}
		}
		if (p_model.phase == BattlePhase::Deployment)
		{
			_contextButton.setText("Ready");
		}
		else if (p_model.phase == BattlePhase::Activation && p_model.activeUnit.has_value() && p_model.activeUnit->side == BattleSide::Player)
		{
			_contextButton.setText("End Turn");
		}
		else
		{
			_contextButton.setText("Waiting");
		}
		useWhiteText(_contextButton);
		_layout();
	}

	void BattleHudWidget::_openDetails(const CreatureCardViewModel &p_model)
	{
		if (_creatureDetailsWindow == nullptr || !p_model.unit.has_value())
		{
			return;
		}
		const CreatureDetailsViewModel &details = p_model.details;
		std::string text =
			"HP: " + std::to_string(details.health.current) + "/" + std::to_string(details.health.maximum) + "\n" +
			"AP: " + std::to_string(details.actionPoints.current) + "/" + std::to_string(details.actionPoints.maximum) +
			"    MP: " + std::to_string(details.movementPoints.current) + "/" + std::to_string(details.movementPoints.maximum) + "\n" +
			"Initiative cost: " + std::to_string(details.stamina.ticks()) + "    Range: " + std::to_string(details.range) + "\n" +
			"Attack: " + std::to_string(details.strength) + "    Armor: " + std::to_string(details.armor) + "\n" +
			"Magic: " + std::to_string(details.magicPower) + "    Resistance: " + std::to_string(details.resistance) + "\n" +
			"Shield: " + std::to_string(details.shield) + "\n\nAbilities:";
		if (details.abilities.empty())
		{
			text += " none";
		}
		for (const std::string &ability : details.abilities)
		{
			text += "\n- " + ability;
		}
		text += "\n\nPassive effects:";
		if (details.passiveEffects.empty())
		{
			text += " none";
		}
		for (const StatusViewModel &effect : details.passiveEffects)
		{
			text += "\n- " + effectName(effect) + " x" + std::to_string(effect.stacks) + " (" + effect.duration + ")";
		}
		text += "\n\nActive effects:";
		if (details.activeEffects.empty())
		{
			text += " none";
		}
		for (const StatusViewModel &effect : details.activeEffects)
		{
			text += "\n- " + effectName(effect) + " x" + std::to_string(effect.stacks) + " (" + effect.duration + ")";
		}
		_creatureDetailsWindow->setTitle(p_model.displayName);
		_creatureDetailsWindow->setText(text);
		_layoutWindow(*_creatureDetailsWindow, 420, 440);
		_creatureDetailsWindow->activate();
	}

	void BattleHudWidget::_openAbilityDetails(const AbilitySlotViewModel &p_model)
	{
		if (_abilityDetailsWindow == nullptr || !p_model.abilityId.has_value())
		{
			return;
		}
		std::string text =
			"Cost: " + (p_model.costText.empty() ? std::string("none") : p_model.costText) + "\n" +
			"Range: " + (p_model.rangeText.empty() ? std::string("none") : p_model.rangeText) + "\n\n" +
			(p_model.description.empty() ? std::string("No description available.") : p_model.description) + "\n\nEffects:";
		if (p_model.effects.empty())
		{
			text += " none";
		}
		for (const std::string &effect : p_model.effects)
		{
			text += "\n- " + effect;
		}
		_abilityDetailsWindow->setTitle(p_model.name.empty() ? *p_model.abilityId : p_model.name);
		_abilityDetailsWindow->setText(text);
		_layoutWindow(*_abilityDetailsWindow, 420, 360);
		_abilityDetailsWindow->activate();
	}

	void BattleHudWidget::_openEffectDetails(const StatusViewModel &p_model)
	{
		if (_effectDetailsWindow == nullptr)
		{
			return;
		}
		const std::string text =
			"Stacks: " + std::to_string(p_model.stacks) + "\n" +
			"Duration: " + p_model.duration + "\n\n" +
			(p_model.description.empty() ? std::string("No description available.") : p_model.description);
		_effectDetailsWindow->setTitle(effectName(p_model));
		_effectDetailsWindow->setText(text);
		_layoutWindow(*_effectDetailsWindow, 360, 280);
		_effectDetailsWindow->activate();
	}

	void BattleHudWidget::_layoutWindow(spk::MessageBox &p_window, std::size_t p_maximumWidth, std::size_t p_maximumHeight)
	{
		if (geometry().width() == 0 || geometry().height() == 0)
		{
			return;
		}
		const std::size_t availableWidth = geometry().width() > 20 ? geometry().width() - 20 : geometry().width();
		const std::size_t availableHeight = geometry().height() > 20 ? geometry().height() - 20 : geometry().height();
		const std::size_t width = std::min(availableWidth, p_maximumWidth);
		const std::size_t height = std::min(availableHeight, p_maximumHeight);
		p_window.setGeometry(rect(
			static_cast<int>((geometry().width() - width) / 2),
			static_cast<int>((geometry().height() - height) / 2),
			width,
			height));
	}

	void BattleHudWidget::_layout()
	{
		const std::size_t width = geometry().width();
		const std::size_t height = geometry().height();
		if (width == 0 || height == 0)
		{
			return;
		}
		const std::size_t margin = 10;
		const std::size_t availableHeight = height > margin ? height - margin : height;
		const std::size_t teamWidth = std::clamp(width * 18U / 100U, std::size_t(200), std::size_t(280));
		const std::size_t activeWidth = std::min(width > 2 * margin ? width - 2 * margin : width, std::max(std::size_t(480), width * 55U / 100U));
		const bool hasActionBar = _model.has_value() && _model->actionBar.has_value();
		const bool showAbilities = hasActionBar && _model->phase == BattlePhase::Activation;
		const std::size_t effectRows = _effects.empty() ? 0 : (_effects.size() + 7) / 8;
		const std::size_t activeHeight = std::min(
			availableHeight,
			std::max(std::size_t(48), std::size_t(16) + effectRows * 30 + (hasActionBar ? 30 : 0) + (showAbilities ? 58 : 0) + 28));
		const int playerPanelX = static_cast<int>(margin);
		const int enemyPanelX = static_cast<int>(width > teamWidth + margin ? width - teamWidth - margin : 0);
		const std::size_t playerRosterHeight = std::min(availableHeight, _playerRoster.preferredHeight());
		const std::size_t enemyRosterHeight = std::min(availableHeight, _enemyRoster.preferredHeight());
		_playerRoster.setGeometry(rect(playerPanelX, static_cast<int>(margin), teamWidth, playerRosterHeight));
		_enemyRoster.setGeometry(rect(enemyPanelX, static_cast<int>(margin), teamWidth, enemyRosterHeight));
		_activePanel.setGeometry(rect(static_cast<int>((width - activeWidth) / 2), static_cast<int>(height > activeHeight + margin ? height - activeHeight - margin : 0), activeWidth, activeHeight));
		const std::size_t statusWidth = std::min(width > 20 ? width - 20 : width, std::size_t(360));
		const int statusX = static_cast<int>((width - statusWidth) / 2);
		_statusPanel.setGeometry(rect(statusX, 8, statusWidth, 34));
		_status.setGeometry(rect(statusX + 8, 13, statusWidth > 16 ? statusWidth - 16 : statusWidth, 24));
		_forecast.setGeometry(rect(static_cast<int>(margin), static_cast<int>(std::min(height, margin + playerRosterHeight + 4)), teamWidth, 45));

		const std::size_t panelWidth = _activePanel.geometry().width();
		const std::size_t padding = 8;
		const std::size_t gap = 4;
		std::size_t y = padding;
		const std::size_t effectWidth = panelWidth > padding * 2 + gap * 7 ? (panelWidth - padding * 2 - gap * 7) / 8 : 0;
		for (std::size_t index = 0; index < _effects.size(); ++index)
		{
			const std::size_t row = index / 8;
			const std::size_t column = index % 8;
			_effects[index]->setGeometry(rect(static_cast<int>(padding + column * (effectWidth + gap)), static_cast<int>(y + row * 30), effectWidth, 26));
		}
		y += effectRows * 30;
		if (hasActionBar)
		{
			const std::size_t resourceWidth = panelWidth > padding * 2 + gap * 2 ? (panelWidth - padding * 2 - gap * 2) / 3 : 0;
			_actionPoints->setGeometry(rect(static_cast<int>(padding), static_cast<int>(y), resourceWidth, 24));
			_health->setGeometry(rect(static_cast<int>(padding + resourceWidth + gap), static_cast<int>(y), resourceWidth, 24));
			_movementPoints->setGeometry(rect(static_cast<int>(padding + (resourceWidth + gap) * 2), static_cast<int>(y), resourceWidth, 24));
			y += 30;
		}
		if (showAbilities)
		{
			const std::size_t slotWidth = panelWidth > padding * 2 + gap * 7 ? (panelWidth - padding * 2 - gap * 7) / 8 : 0;
			for (std::size_t index = 0; index < 8; ++index)
			{
				_abilities[index]->setGeometry(rect(static_cast<int>(padding + index * (slotWidth + gap)), static_cast<int>(y), slotWidth, 54));
			}
			y += 58;
		}
		const std::size_t contextWidth = std::min(std::size_t(140), panelWidth);
		_contextButton.setGeometry(rect(static_cast<int>((panelWidth - contextWidth) / 2), static_cast<int>(y), contextWidth, 28));
		_layoutWindow(*_creatureDetailsWindow, 420, 440);
		_layoutWindow(*_abilityDetailsWindow, 420, 360);
		_layoutWindow(*_effectDetailsWindow, 360, 280);
	}

	void BattleHudWidget::_onGeometryChange()
	{
		_layout();
	}

	const std::optional<BattleHudViewModel> &BattleHudWidget::model() const noexcept
	{
		return _model;
	}
}
