#include "battle/presentation/battle_presentation_runtime.hpp"

#include "battle/presentation/battle_board_picker.hpp"
#include "battle/presentation/battle_hud_view_model_builder.hpp"
#include "battle/presentation/battle_interaction_controller.hpp"
#include "battle/presentation/battle_object_presenter.hpp"
#include "battle/presentation/battle_overlay_model.hpp"
#include "battle/presentation/battle_overlay_presenter.hpp"
#include "battle/presentation/battle_unit_presenter.hpp"
#include "battle/presentation/tactical_camera_controller.hpp"
#include "core/game_rules.hpp"
#include "structures/game_engine/spk_camera_3d.hpp"
#include "structures/game_engine/spk_game_engine.hpp"
#include "structures/system/event/spk_events.hpp"
#include "widgets/battle/battle_hud_widget.hpp"

#include <cmath>
#include <utility>

namespace pg
{
	BattlePresentationRuntime::BattlePresentationRuntime(
		spk::GameEngine &p_engine,
		spk::Camera3D &p_camera,
		BattleSession &p_commands,
		BattleSnapshotProvider p_snapshots,
		BattlePresentationCallbacks p_callbacks,
		std::function<spk::Vector2()> p_viewportSize,
		const spk::Texture &p_maskTexture,
		const spk::Texture &p_unitTexture,
		const GameRules &p_rules,
		BattleHudWidget &p_hud) :
		_engine(p_engine),
		_camera(p_camera),
		_commands(p_commands),
		_snapshots(std::move(p_snapshots)),
		_callbacks(std::move(p_callbacks)),
		_viewportSize(std::move(p_viewportSize)),
		_maskTexture(p_maskTexture),
		_unitTexture(p_unitTexture),
		_rules(p_rules),
		_hud(p_hud),
		_lastSnapshot(_snapshots())
	{
	}

	void BattlePresentationRuntime::_refreshHud()
	{
		if (_entered && _interaction != nullptr)
		{
			_hud.render(BattleHudViewModelBuilder::build(_lastSnapshot, _commands.registries(), *_interaction));
		}
	}

	BattlePresentationRuntime::~BattlePresentationRuntime()
	{
		detach();
	}

	void BattlePresentationRuntime::enter(const BattlePresentationBoardBinding &p_boardPresentation)
	{
		if (_entered)
		{
			return;
		}
		if (!p_boardPresentation.bounds.finite())
		{
			throw std::invalid_argument("battle presentation requires finite board bounds");
		}
		_binding = &p_boardPresentation;
		_presenter = std::make_unique<BattleOverlayPresenter>(_engine, _maskTexture, _rules);
		_unitPresenter = std::make_unique<BattleUnitPresenter>();
		_objectPresenter = std::make_unique<BattleObjectPresenter>();
		_overlay = std::make_unique<BattleOverlayModelBuilder>();
		_picker = std::make_unique<BattleBoardPicker>();
		_cameraController = std::make_unique<TacticalCameraController>();
		_interaction = std::make_unique<BattleInteractionController>(
			_commands,
			_snapshots,
			[this] {
				if (_callbacks.presentationStateChanged)
				{
					_callbacks.presentationStateChanged();
				}
				_refreshHud();
				_refreshOverlay();
			});
		_presenter->attach();
		_cameraController->enter(_binding->bounds, _camera);
		_entered = true;
		_lastSnapshot = _snapshots();
		BattleEventSequence next{1};
		if (!_commands.archivedBatches().empty())
		{
			next = _commands.archivedBatches().back().events.onePastLast;
		}
		_unitPresenter->attach(_engine, *_binding, _commands.registries(), _unitTexture, _lastSnapshot, next);
		_objectPresenter->attach(_engine, *_binding, _unitTexture, _lastSnapshot, next);
		_interaction->syncFromSnapshot(_lastSnapshot);
		_hud.bind(*_interaction);
		_refreshHud();
		_refreshOverlay();
	}

	void BattlePresentationRuntime::onCommittedBatch(const CommittedBattleBatch &p_batch)
	{
		if (!_entered)
		{
			return;
		}
		const std::vector<BattleEvent> events = _commands.copyEvents(p_batch.events);
		_unitPresenter->consume(events, p_batch.after);
		_objectPresenter->consume(events, p_batch.after);
		_lastSnapshot = p_batch.after;
		_interaction->syncFromSnapshot(_lastSnapshot);
		_refreshHud();
		_refreshOverlay();
	}

	void BattlePresentationRuntime::_refreshOverlay()
	{
		if (!_entered || _binding == nullptr)
		{
			return;
		}
		if (_overlay->update(_interaction->overlayCandidates()))
		{
			_presenter->present(_overlay->model(), _binding->board, _binding->cells);
		}
	}

	std::optional<BoardCell> BattlePresentationRuntime::_pick(const spk::Vector2Int &p_mouse) const
	{
		if (!_entered || _binding == nullptr)
		{
			return std::nullopt;
		}
		const spk::Vector2 viewport = _viewportSize();
		if (viewport.x <= 0.0f || viewport.y <= 0.0f)
		{
			return std::nullopt;
		}
		const spk::Ray3D ray = _camera.rayFromViewport(viewport, spk::Vector2(p_mouse));
		const std::optional<BattlePick> pick = _picker->pick(_binding->board, _binding->cells, ray);
		return pick.has_value() ? std::optional<BoardCell>(pick->boardCell) : std::nullopt;
	}

	bool BattlePresentationRuntime::_handled(InputHandling p_result)
	{
		if (p_result.submission.has_value() && _callbacks.commandCompleted)
		{
			_callbacks.commandCompleted(*p_result.submission);
		}
		_refreshHud();
		_refreshOverlay();
		return p_result.disposition != InputHandlingDisposition::Ignored;
	}

	void BattlePresentationRuntime::update(const spk::UpdateContext &p_tick)
	{
		if (!_entered)
		{
			return;
		}
		_cameraController->update(p_tick, _camera);
		_unitPresenter->update(p_tick);
		_objectPresenter->update(p_tick);
		_interaction->update(static_cast<float>(p_tick.deltaTime.seconds()));
		const BattleSnapshot snapshot = _snapshots();
		if (!(snapshot == _lastSnapshot))
		{
			_lastSnapshot = snapshot;
			_interaction->syncFromSnapshot(snapshot);
			_refreshHud();
		}
		_refreshOverlay();
	}

	void BattlePresentationRuntime::beginExit()
	{
		if (!_entered)
		{
			return;
		}
		_callbacks = {};
		_hud.unbind();
		onMouseLeave();
		if (_cameraController)
		{
			_cameraController->beginExit(_camera.target());
		}
		if (_presenter)
		{
			_presenter->detach();
		}
		if (_unitPresenter)
		{
			_unitPresenter->fastForward();
			_unitPresenter->detach();
		}
		if (_objectPresenter)
		{
			_objectPresenter->fastForward();
			_objectPresenter->detach();
		}
		_entered = false;
	}

	void BattlePresentationRuntime::detach() noexcept
	{
		beginExit();
		_interaction.reset();
		_overlay.reset();
		_picker.reset();
		_presenter.reset();
		_unitPresenter.reset();
		_objectPresenter.reset();
		_cameraController.reset();
		_binding = nullptr;
	}

	bool BattlePresentationRuntime::onMouseMoved(const spk::MouseMovedEvent &p_event)
	{
		if (!_entered)
		{
			return false;
		}
		if (_rightPressed)
		{
			const spk::Vector2Int difference = p_event->position - _rightPressPosition;
			if (!_rightDragging && std::abs(difference.x) + std::abs(difference.y) > 3)
			{
				_rightDragging = true;
			}
			if (_rightDragging)
			{
				_cameraController->onRightDrag(p_event->delta);
			}
			return true;
		}
		_interaction->hover(_pick(p_event->position));
		_refreshHud();
		_refreshOverlay();
		return true;
	}

	bool BattlePresentationRuntime::onMousePressed(const spk::MouseButtonPressedEvent &p_event)
	{
		if (!_entered)
		{
			return false;
		}
		if (p_event->button == spk::Mouse::Right)
		{
			_rightPressed = true;
			_rightDragging = false;
			_rightPressPosition = p_event.device().position;
			return true;
		}
		if (p_event->button != spk::Mouse::Left)
		{
			return false;
		}
		const std::optional<BoardCell> picked = _pick(p_event.device().position);
		_interaction->hover(picked);
		return picked.has_value() ? _handled(_interaction->clickBoardCell(*picked)) : false;
	}

	bool BattlePresentationRuntime::onMouseReleased(const spk::MouseButtonReleasedEvent &p_event)
	{
		if (!_entered || p_event->button != spk::Mouse::Right)
		{
			return false;
		}
		const bool cancel = _rightPressed && !_rightDragging;
		_rightPressed = false;
		_rightDragging = false;
		return cancel ? _handled(_interaction->cancel()) : true;
	}

	bool BattlePresentationRuntime::onMouseWheel(const spk::MouseWheelScrolledEvent &p_event)
	{
		if (!_entered)
		{
			return false;
		}
		_cameraController->onWheel(p_event->delta.y);
		return true;
	}

	bool BattlePresentationRuntime::onKeyPressed(const spk::KeyPressedEvent &p_event)
	{
		if (!_entered || p_event->isRepeated)
		{
			return false;
		}
		switch (p_event->key)
		{
		case spk::Keyboard::M:
			if (std::holds_alternative<SelectingMovement>(_interaction->state()))
			{
				return _handled(_interaction->cancel());
			}
			return _handled(_interaction->selectMove());
		case spk::Keyboard::Escape:
			return _handled(_interaction->cancel());
		case spk::Keyboard::Return:
			return _handled(_interaction->confirmDeployment());
		case spk::Keyboard::Space:
			return _handled(_interaction->endTurn());
		default:
			break;
		}
		if (p_event->key >= spk::Keyboard::Key1 && p_event->key <= spk::Keyboard::Key8)
		{
			if (std::holds_alternative<PlacementSelecting>(_interaction->state()))
			{
				return _handled(_interaction->selectPlacementRosterSlot(
					static_cast<std::size_t>(p_event->key - spk::Keyboard::Key1)));
			}
			return _handled(_interaction->selectAbility(static_cast<std::size_t>(p_event->key - spk::Keyboard::Key1)));
		}
		return false;
	}

	void BattlePresentationRuntime::onMouseLeave() noexcept
	{
		_rightPressed = false;
		_rightDragging = false;
		if (_interaction)
		{
			_interaction->hover(std::nullopt);
		}
	}

	bool BattlePresentationRuntime::entered() const noexcept
	{
		return _entered;
	}

	const BattleInteractionController *BattlePresentationRuntime::interaction() const noexcept
	{
		return _interaction.get();
	}
}
