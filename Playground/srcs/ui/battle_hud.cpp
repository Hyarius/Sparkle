#include "ui/battle_hud.hpp"

#include "battle/battle_context.hpp"
#include "battle/battle_input.hpp"
#include "battle/phases/battle_coordinator.hpp"

#include <algorithm>
#include <utility>

namespace
{
	[[nodiscard]] int digitFor(spk::Keyboard::Key p_key)
	{
		if (p_key == spk::Keyboard::Key1 || p_key == spk::Keyboard::Numpad1) return 1;
		if (p_key == spk::Keyboard::Key2 || p_key == spk::Keyboard::Numpad2) return 2;
		if (p_key == spk::Keyboard::Key3 || p_key == spk::Keyboard::Numpad3) return 3;
		if (p_key == spk::Keyboard::Key4 || p_key == spk::Keyboard::Numpad4) return 4;
		if (p_key == spk::Keyboard::Key5 || p_key == spk::Keyboard::Numpad5) return 5;
		if (p_key == spk::Keyboard::Key6 || p_key == spk::Keyboard::Numpad6) return 6;
		if (p_key == spk::Keyboard::Key7 || p_key == spk::Keyboard::Numpad7) return 7;
		if (p_key == spk::Keyboard::Key8 || p_key == spk::Keyboard::Numpad8) return 8;
		return 0;
	}
}

namespace pg
{
	BattleHud::BattleHud(
		const std::string &p_name,
		std::shared_ptr<spk::SpriteSheet> p_icons,
		spk::Widget *p_parent) :
		spk::Widget(p_name, p_parent),
		_playerTeam(p_name + "/PlayerTeam", p_icons, this),
		_enemyTeam(p_name + "/EnemyTeam", p_icons, this),
		_turnOrder(p_name + "/TurnOrder", this),
		_activeUnit(p_name + "/ActiveUnit", p_icons, this),
		_placement(p_name + "/Placement", this),
		_abilityPreview(p_name + "/AbilityPreview", p_icons, this),
		_creatureInfo(p_name + "/CreatureInfo", this)
	{
		_activeUnit.setAbilitySelectionHandler([this](std::size_t p_index) {
			if (_input != nullptr && _input->selectAbility(p_index))
			{
				_activeUnit.refresh(_input->selectedAbility());
			}
		});
		_activeUnit.setEndTurnHandler([this] {
			if (_input != nullptr) (void)_input->endTurn();
		});
		_activeUnit.setAbilityHoverHandler([this](const Ability *p_ability) {
			_abilityPreview.bind(p_ability);
		});
		_playerTeam.setSelectionHandler([this](BattleUnit *p_unit) {
			return _placement.selectUnit(p_unit);
		});
		_placement.setSelectionChangedHandler([this](BattleUnit *p_unit) {
			_playerTeam.setSelected(p_unit);
		});
		auto showInfo = [this](BattleUnit *p_unit) {
			if (_context != nullptr && p_unit != nullptr) _creatureInfo.bind(*_context, *p_unit);
		};
		_playerTeam.setInfoHandler(showInfo);
		_enemyTeam.setInfoHandler(showInfo);
		deactivate();
	}

	void BattleHud::bind(
		BattleContext &p_context, BattleInput &p_input, BattleCoordinator &p_coordinator)
	{
		unbind();
		_context = &p_context;
		_input = &p_input;
		_playerTeam.bind(_context->getUnits(BattleSide::Player), BattleSide::Player);
		_enemyTeam.bind(_context->getUnits(BattleSide::Enemy), BattleSide::Enemy);
		_turnOrder.bind(*_context);
		_placement.bind(p_coordinator.placementPhase());
		_bindActiveUnit(_context->currentTurn.activeUnit);
		_battleEventContract = _context->events().battleEventOccurred.subscribe([this](const BattleEvent *p_event) {
			if (p_event != nullptr)
			{
				if (const TurnStartedEvent *started = p_event->getIf<TurnStartedEvent>(); started != nullptr)
				{
					_bindActiveUnit(started->context.caster);
				}
			}
			_refresh();
		});
		_turnEndedContract = _context->events().battleTurnEnded.subscribe([this](BattleUnit *) {
			_bindActiveUnit(nullptr);
			_refresh();
		});
		activate();
	}

	void BattleHud::unbind()
	{
		_battleEventContract.resign();
		_turnEndedContract.resign();
		_playerTeam.unbind();
		_enemyTeam.unbind();
		_turnOrder.unbind();
		_activeUnit.unbind();
		_placement.unbind();
		_abilityPreview.bind(nullptr);
		_creatureInfo.unbind();
		_context = nullptr;
		_input = nullptr;
		deactivate();
	}

	void BattleHud::_bindActiveUnit(BattleUnit *p_unit)
	{
		_activeUnit.bind(_context, p_unit);
		_activeUnit.refresh(_input != nullptr ? _input->selectedAbility() : nullptr);
	}

	void BattleHud::_refresh()
	{
		_turnOrder.refresh();
		_activeUnit.refresh(_input != nullptr ? _input->selectedAbility() : nullptr);
	}

	void BattleHud::_onGeometryChange()
	{
		const unsigned int width = geometry().width();
		const unsigned int height = geometry().height();
		const unsigned int panelWidth = std::min<unsigned int>(220, width / 5);
		const unsigned int activeHeight = std::min<unsigned int>(150, height / 4);
		const unsigned int columnHeight = height > activeHeight + 20 ? height - activeHeight - 20 : 0;
		_playerTeam.setGeometry(spk::Rect2D(8, 8, panelWidth, columnHeight));
		_enemyTeam.setGeometry(spk::Rect2D(
			static_cast<int>(width > panelWidth + 8 ? width - panelWidth - 8 : 0), 8, panelWidth, columnHeight));

		const unsigned int stripWidth = std::min<unsigned int>(130, width / 7);
		_turnOrder.setGeometry(spk::Rect2D(
			static_cast<int>(panelWidth + 16), 8, stripWidth, columnHeight));

		const unsigned int activeWidth = std::min<unsigned int>(840, width > 32 ? width - 32 : 0);
		_activeUnit.setGeometry(spk::Rect2D(
			static_cast<int>((width - activeWidth) / 2),
			static_cast<int>(height > activeHeight + 8 ? height - activeHeight - 8 : 0),
			activeWidth,
			activeHeight));

		const unsigned int placementWidth = std::min<unsigned int>(680, width > 24 ? width - 24 : 0);
		_placement.setGeometry(spk::Rect2D(
			static_cast<int>((width - placementWidth) / 2),
			static_cast<int>(height > 92 ? height - 92 : 0),
			placementWidth,
			84));
		const unsigned int previewWidth = std::min<unsigned int>(460, width > 24 ? width - 24 : 0);
		_abilityPreview.setGeometry(spk::Rect2D(
			static_cast<int>((width - previewWidth) / 2),
			static_cast<int>(height > activeHeight + 180 ? height - activeHeight - 180 : 8),
			previewWidth,
			160));
		const unsigned int infoWidth = std::min<unsigned int>(520, width > 24 ? width - 24 : 0);
		const unsigned int infoHeight = std::min<unsigned int>(460, height > 24 ? height - 24 : 0);
		_creatureInfo.setGeometry(spk::Rect2D(
			static_cast<int>((width - infoWidth) / 2),
			static_cast<int>((height - infoHeight) / 2), infoWidth, infoHeight));
	}

	void BattleHud::_onKeyPressedEvent(spk::KeyPressedEvent &p_event)
	{
		if (_input == nullptr || !_input->isPlayerInputActive()) return;
		const int digit = digitFor(p_event->key);
		if (digit > 0)
		{
			const std::size_t local = static_cast<std::size_t>(digit - 1);
			if (_activeUnit.abilityBar().slot(local).enabled())
			{
				const std::size_t global = _activeUnit.abilityBar().page() * AbilityBarWidget::PageSize + local;
				(void)_input->selectAbility(global);
				_activeUnit.refresh(_input->selectedAbility());
			}
			p_event.consume();
		}
		else if (p_event->key == spk::Keyboard::Space || p_event->key == spk::Keyboard::Return)
		{
			(void)_input->endTurn();
			p_event.consume();
		}
	}

	bool BattleHud::bound() const noexcept { return _context != nullptr; }
	const TeamColumnWidget &BattleHud::playerTeam() const noexcept { return _playerTeam; }
	const TeamColumnWidget &BattleHud::enemyTeam() const noexcept { return _enemyTeam; }
	const TurnOrderStrip &BattleHud::turnOrder() const noexcept { return _turnOrder; }
	const ActiveUnitPanel &BattleHud::activeUnit() const noexcept { return _activeUnit; }
	const PlacementUI &BattleHud::placement() const noexcept { return _placement; }
	const CreatureInfoWindow &BattleHud::creatureInfo() const noexcept { return _creatureInfo; }
}
