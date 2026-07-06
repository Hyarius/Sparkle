#include "ui/exploration_hud.hpp"

#include "core/game_context.hpp"
#include "creatures/creature_unit.hpp"
#include "encounters/biome.hpp"
#include "world/map_definition.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <utility>

namespace pg
{
	ExplorationHud::ExplorationHud(
		const std::string &p_name,
		std::shared_ptr<spk::SpriteSheet> p_icons,
		spk::Widget *p_parent) :
		spk::Widget(p_name, p_parent),
		_timePanel(p_name + "/TimePanel", this),
		_timeLabel(p_name + "/Time", &_timePanel),
		_zonePanel(p_name + "/ZonePanel", this),
		_zoneLabel(p_name + "/Zone", &_zonePanel),
		_saveButton(p_name + "/Save", "Save", this),
		_quitButton(p_name + "/Quit", "Quit", this),
		_partyPanel(p_name + "/Party", this),
		_partyTitle(p_name + "/PartyTitle", &_partyPanel),
		_promptPanel(p_name + "/PromptPanel", this),
		_promptLabel(p_name + "/Prompt", &_promptPanel)
	{
		_timeLabel.setText("Day 1 - 12:00");
		_timeLabel.setTextSize(spk::Font::Size(14, 1));
		_zoneLabel.setTextSize(spk::Font::Size(18, 1));
		_partyTitle.setText("Party");
		_partyTitle.setTextSize(spk::Font::Size(16, 1));
		_promptLabel.setTextSize(spk::Font::Size(16, 1));
		for (std::size_t index = 0; index < _partyCards.size(); ++index)
		{
			_partyCards[index] = std::make_unique<CreatureCardWidget>(
				p_name + "/Party/Card" + std::to_string(index + 1), p_icons, &_partyPanel);
			_partyCards[index]->deactivate();
		}
		_promptPanel.deactivate();
		deactivate();
	}

	void ExplorationHud::bind(GameContext &p_context, std::function<void()> p_quit)
	{
		unbind();
		_context = &p_context;
		_quit = std::move(p_quit);
		_saveContract = _saveButton.subscribeToClick([] {
			std::cout << "Save is available in step 32" << std::endl;
		});
		_quitContract = _quitButton.subscribeToClick([this] {
			if (_quit)
			{
				_quit();
			}
		});
		_worldContract = _context->events.worldChanged.subscribe([this] {
			_refreshZone();
		});
		_partyContract = _context->events.partyChanged.subscribe([this] {
			_refreshParty();
		});
		_promptContract = _context->events.interactionPromptChanged.subscribe(
			[this](std::string p_prompt) {
				_setPrompt(std::move(p_prompt));
			});
		_modeContract = _context->events.explorationModeChanged.subscribe([this](bool p_active) {
			if (p_active)
			{
				activate();
			}
			else
			{
				deactivate();
			}
		});
		_refreshZone();
		_refreshParty();
		_setPrompt({});
		if (_context->world.explorationActive)
		{
			activate();
		}
	}

	void ExplorationHud::unbind()
	{
		_saveContract.resign();
		_quitContract.resign();
		_worldContract.resign();
		_partyContract.resign();
		_promptContract.resign();
		_modeContract.resign();
		for (auto &card : _partyCards)
		{
			card->unbind();
			card->deactivate();
		}
		_context = nullptr;
		_quit = {};
		_partyCount = 0;
		deactivate();
	}

	void ExplorationHud::_refreshZone()
	{
		_zoneText.clear();
		if (_context != nullptr && _context->world.activeMap != nullptr)
		{
			_zoneText = _context->world.activeMap->id;
			if (_context->world.activeBiome != nullptr)
			{
				_zoneText += " - " + _context->world.activeBiome->displayName;
			}
		}
		else if (_context != nullptr && _context->world.activeBiome != nullptr)
		{
			_zoneText = "Generated World - " + _context->world.activeBiome->displayName;
		}
		_zoneLabel.setText(_zoneText.empty() ? "Unknown zone" : _zoneText);
	}

	void ExplorationHud::_refreshParty()
	{
		for (auto &card : _partyCards)
		{
			card->unbind();
			card->deactivate();
		}
		_partyCount = 0;
		if (_context == nullptr)
		{
			return;
		}
		for (const std::unique_ptr<CreatureUnit> &creature : _context->player.team)
		{
			if (creature == nullptr)
			{
				continue;
			}
			_partyCards[_partyCount]->bind(creature.get());
			_partyCards[_partyCount]->activate();
			if (++_partyCount == _partyCards.size())
			{
				break;
			}
		}
	}

	void ExplorationHud::_setPrompt(std::string p_prompt)
	{
		_promptText = std::move(p_prompt);
		_promptLabel.setText(_promptText);
		if (_promptText.empty())
		{
			_promptPanel.deactivate();
		}
		else
		{
			_promptPanel.activate();
		}
	}

	void ExplorationHud::_onGeometryChange()
	{
		const unsigned int width = geometry().width();
		const unsigned int height = geometry().height();
		_timePanel.setGeometry(spk::Rect2D(8, 8, 150, 38));
		_timeLabel.setGeometry(spk::Rect2D(4, 2, 142, 34));
		const unsigned int zoneWidth = std::min<unsigned int>(420, width > 16 ? width - 16 : 0);
		_zonePanel.setGeometry(spk::Rect2D(static_cast<int>((width - zoneWidth) / 2), 8, zoneWidth, 42));
		_zoneLabel.setGeometry(spk::Rect2D(4, 2, zoneWidth > 8 ? zoneWidth - 8 : 0, 38));
		_saveButton.setGeometry(spk::Rect2D(static_cast<int>(width > 176 ? width - 176 : 0), 8, 78, 38));
		_quitButton.setGeometry(spk::Rect2D(static_cast<int>(width > 90 ? width - 90 : 0), 8, 78, 38));
		const unsigned int partyWidth = std::min<unsigned int>(220, width / 4);
		const unsigned int partyHeight = std::min<unsigned int>(480, height > 70 ? height - 70 : 0);
		_partyPanel.setGeometry(spk::Rect2D(8, 58, partyWidth, partyHeight));
		_partyTitle.setGeometry(spk::Rect2D(4, 2, partyWidth > 8 ? partyWidth - 8 : 0, 28));
		const unsigned int available = partyHeight > 32 ? partyHeight - 32 : 0;
		const unsigned int cardHeight = available / static_cast<unsigned int>(_partyCards.size());
		for (std::size_t index = 0; index < _partyCards.size(); ++index)
		{
			_partyCards[index]->setGeometry(spk::Rect2D(4, static_cast<int>(32 + index * cardHeight), partyWidth > 8 ? partyWidth - 8 : 0, cardHeight > 3 ? cardHeight - 3 : cardHeight));
		}
		const unsigned int promptWidth = std::min<unsigned int>(280, width > 16 ? width - 16 : 0);
		_promptPanel.setGeometry(spk::Rect2D(static_cast<int>((width - promptWidth) / 2), static_cast<int>(height > 80 ? height - 80 : 0), promptWidth, 48));
		_promptLabel.setGeometry(spk::Rect2D(4, 2, promptWidth > 8 ? promptWidth - 8 : 0, 44));
	}

	bool ExplorationHud::bound() const noexcept
	{
		return _context != nullptr;
	}
	const std::string &ExplorationHud::zoneText() const noexcept
	{
		return _zoneText;
	}
	const std::string &ExplorationHud::promptText() const noexcept
	{
		return _promptText;
	}
	std::size_t ExplorationHud::partyCount() const noexcept
	{
		return _partyCount;
	}
	const CreatureCardWidget &ExplorationHud::partyCard(std::size_t p_index) const
	{
		if (p_index >= _partyCards.size())
		{
			throw std::out_of_range("party card index out of range");
		}
		return *_partyCards[p_index];
	}
}
