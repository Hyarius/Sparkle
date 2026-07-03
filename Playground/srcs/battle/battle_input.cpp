#include "battle/battle_input.hpp"

#include "abilities/ability.hpp"
#include "battle/battle_context.hpp"
#include "battle/battle_unit.hpp"
#include "battle/phases/battle_coordinator.hpp"
#include "battle/rules/battle_action_validator.hpp"
#include "board/board_raycaster.hpp"
#include "board/pathfinder.hpp"
#include "rendering/mouse_picker.hpp"

#include <algorithm>
#include <array>
#include <utility>

namespace pg
{
	BattleInputController::BattleInputController(const BattleContext &p_context, BoardOverlayState &p_overlay) :
		_context(p_context),
		_overlay(p_overlay)
	{
	}

	BattleUnit *BattleInputController::_activeUnit() const
	{
		return _context.currentTurn.activeUnit;
	}

	void BattleInputController::_refresh()
	{
		static constexpr std::array transient = {
			OverlayCategory::Deployment,
			OverlayCategory::MoveRange,
			OverlayCategory::Path,
			OverlayCategory::AbilityRange,
			OverlayCategory::AreaOfEffect,
			OverlayCategory::LosBlocked,
			OverlayCategory::Hovered,
			OverlayCategory::Invalid};
		for (OverlayCategory category : transient)
		{
			_overlay.cells(category).clear();
		}
		_reachable.clear();
		_validTargets.clear();

		BattleUnit *active = _activeUnit();
		if (active != nullptr && active->boardPosition.has_value())
		{
			if (_hovered.has_value())
			{
				_overlay.cells(OverlayCategory::Hovered) = {*_hovered};
			}
			if (_mode == Mode::Move)
			{
				_reachable = BattleActionValidator::getReachableCells(_context, *active);
				std::vector<spk::Vector3Int> &range = _overlay.cells(OverlayCategory::MoveRange);
				for (const auto &[cell, cost] : _reachable)
				{
					if (cost > 0.0f)
					{
						range.push_back(cell);
					}
				}
				if (_hovered.has_value())
				{
					const auto reached = _reachable.find(*_hovered);
					if (reached != _reachable.end() && reached->second > 0.0f)
					{
						const auto path = Pathfinder::findPath(
							_context.board.navigation().graph(),
							*active->boardPosition,
							*_hovered,
							static_cast<float>(active->attributes.mp.current()));
						if (path.has_value())
						{
							_overlay.cells(OverlayCategory::Path) = *path;
						}
					}
					else if (*_hovered != *active->boardPosition)
					{
						_overlay.cells(OverlayCategory::Invalid) = {*_hovered};
					}
				}
			}
			else if (_ability != nullptr)
			{
				const RangeShading shading = BattleActionValidator::getRangeShading(_context, *active, *_ability);
				_overlay.cells(OverlayCategory::AbilityRange) = shading.visible;
				_overlay.cells(OverlayCategory::LosBlocked) = shading.blocked;
				_validTargets = BattleActionValidator::getValidTargets(_context, *active, *_ability);
				if (_hovered.has_value())
				{
					if (std::ranges::find(_validTargets, *_hovered) != _validTargets.end())
					{
						_overlay.cells(OverlayCategory::AreaOfEffect) =
							BattleActionValidator::getAreaCells(_context, *active, *_ability, *_hovered);
					}
					else
					{
						_overlay.cells(OverlayCategory::Invalid) = {*_hovered};
					}
				}
			}
		}
		_overlay.touch();
	}

	void BattleInputController::reset()
	{
		_mode = Mode::Move;
		_ability = nullptr;
		_hovered.reset();
		_refresh();
	}

	void BattleInputController::enterMoveMode()
	{
		_mode = Mode::Move;
		_ability = nullptr;
		_refresh();
	}

	bool BattleInputController::enterAbilityMode(std::size_t p_index)
	{
		BattleUnit *active = _activeUnit();
		if (active == nullptr || p_index >= active->source().abilities.size())
		{
			return false;
		}
		_ability = active->source().abilities[p_index];
		if (_ability == nullptr)
		{
			return false;
		}
		_mode = Mode::Ability;
		_refresh();
		return true;
	}

	void BattleInputController::handleDigit(int p_digit)
	{
		if (p_digit >= 1)
		{
			(void)enterAbilityMode(static_cast<std::size_t>(p_digit - 1));
		}
	}

	void BattleInputController::escape()
	{
		enterMoveMode();
	}

	void BattleInputController::hover(const std::optional<spk::Vector3Int> &p_cell)
	{
		_hovered = p_cell;
		_refresh();
	}

	std::unique_ptr<BattleAction> BattleInputController::click(const std::optional<spk::Vector3Int> &p_cell)
	{
		hover(p_cell);
		BattleUnit *active = _activeUnit();
		if (active == nullptr || !active->boardPosition.has_value() || !p_cell.has_value())
		{
			return nullptr;
		}
		if (_mode == Mode::Move)
		{
			const auto reached = _reachable.find(*p_cell);
			if (reached != _reachable.end() && reached->second > 0.0f)
			{
				return std::make_unique<MoveAction>(*active, *p_cell, static_cast<int>(reached->second));
			}
		}
		else if (_ability != nullptr && std::ranges::find(_validTargets, *p_cell) != _validTargets.end())
		{
			return std::make_unique<AbilityAction>(*active, *_ability, std::vector<spk::Vector3Int>{*p_cell});
		}
		return nullptr;
	}

	std::unique_ptr<BattleAction> BattleInputController::endTurn()
	{
		BattleUnit *active = _activeUnit();
		return active == nullptr ? nullptr : std::make_unique<EndTurnAction>(*active);
	}

	BattleInput::BattleInput(spk::Camera3D &p_camera, BoardOverlayState &p_overlay, ViewportSize p_viewportSize) :
		_camera(p_camera),
		_overlay(p_overlay),
		_viewportSize(std::move(p_viewportSize))
	{
	}

	void BattleInput::bind(BattleContext &p_context, BattleCoordinator &p_coordinator)
	{
		_context = &p_context;
		_coordinator = &p_coordinator;
		_controller = std::make_unique<BattleInputController>(p_context, _overlay);
	}

	void BattleInput::unbind()
	{
		_context = nullptr;
		_coordinator = nullptr;
		_controller.reset();
		_busy = [] {
			return false;
		};
	}

	void BattleInput::setBusyPredicate(std::function<bool()> p_busy)
	{
		_busy = p_busy ? std::move(p_busy) : std::function<bool()>([] {
			return false;
		});
	}

	bool BattleInput::_isPlayerInputActive() const
	{
		return _context != nullptr && _coordinator != nullptr && _controller != nullptr &&
			   _coordinator->currentPhaseName() == "PlayerTurn" && !_busy();
	}

	std::optional<spk::Vector3Int> BattleInput::_pick(const spk::Vector2Int &p_mouse) const
	{
		const WorldRay ray = MousePicker::screenToRay(_camera, _viewportSize(), spk::Vector2(p_mouse));
		return BoardRaycaster::resolveMouseCell(_context->board, ray);
	}

	void BattleInput::_submit(std::unique_ptr<BattleAction> p_action)
	{
		if (_coordinator != nullptr && p_action != nullptr)
		{
			(void)_coordinator->playerTurnPhase().submitAction(std::move(p_action));
		}
	}

	void BattleInput::_parseComponentForMouseMovedEvent(spk::MouseMovedEvent &p_event, Actor &p_actor)
	{
		if (!p_actor.player || !_isPlayerInputActive())
		{
			return;
		}
		_controller->hover(_pick(p_event->position));
	}

	void BattleInput::_parseComponentForMouseButtonPressedEvent(spk::MouseButtonPressedEvent &p_event, Actor &p_actor)
	{
		if (!p_actor.player || !_isPlayerInputActive() || p_event->button != spk::Mouse::Left)
		{
			return;
		}
		_submit(_controller->click(_pick(p_event.device().position)));
	}

	void BattleInput::_parseComponentForKeyPressedEvent(spk::KeyPressedEvent &p_event, Actor &p_actor)
	{
		if (!p_actor.player || !_isPlayerInputActive())
		{
			return;
		}
		if (p_event->key == spk::Keyboard::Escape)
		{
			_controller->escape();
		}
		else if (p_event->key == spk::Keyboard::Key1 || p_event->key == spk::Keyboard::Numpad1)
		{
			_controller->handleDigit(1);
		}
		else if (p_event->key == spk::Keyboard::Space || p_event->key == spk::Keyboard::Return)
		{
			_submit(_controller->endTurn());
		}
	}
}
