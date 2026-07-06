#include "core/battle_scene.hpp"

#include "battle/battle_context.hpp"
#include "battle/battle_event.hpp"
#include "battle/battle_input.hpp"
#include "battle/board_overlay_state.hpp"
#include "battle/phases/battle_coordinator.hpp"
#include "board/deployment.hpp"
#include "components/board_overlay_view.hpp"
#include "core/event_center.hpp"
#include "core/registries.hpp"
#include "logics/battle_unit_view_logic.hpp"
#include "logics/tactical_camera_logic.hpp"
#include "ui/battle_banner.hpp"
#include "ui/battle_hud.hpp"
#include "ui/battle_result_screen.hpp"

#include "type/spk_constants.hpp"

#include <array>
#include <cmath>
#include <utility>
#include <vector>

namespace pg
{
	BattleScene::BattleScene(
		BoardOverlayView &p_overlayView,
		BoardOverlayState &p_overlayState,
		BattleInput &p_input,
		TacticalCameraLogic &p_camera3dController,
		spk::Camera3D &p_camera,
		const ICellLookup &p_worldLookup,
		const Registries &p_registries,
		BattleUnitViewLogic &p_unitViews,
		BattleHud &p_hud,
		BattleBanner &p_banner,
		BattleResultScreen &p_resultScreen) :
		_overlayView(p_overlayView),
		_overlayState(p_overlayState),
		_input(p_input),
		_camera3dController(p_camera3dController),
		_camera(p_camera),
		_worldLookup(p_worldLookup),
		_registries(p_registries),
		_unitViews(p_unitViews),
		_hud(p_hud),
		_banner(p_banner),
		_resultScreen(p_resultScreen)
	{
	}

	float BattleScene::_playerYaw(const BattleContext &p_context) const
	{
		const spk::Vector3Int size = p_context.board.terrain().size();
		const std::vector<spk::Vector3Int> &zone = p_context.board.deploymentZones().player;
		if (zone.empty())
		{
			return 225.0f;
		}
		float sumX = 0.0f;
		float sumZ = 0.0f;
		for (const spk::Vector3Int &cell : zone)
		{
			sumX += static_cast<float>(cell.x);
			sumZ += static_cast<float>(cell.z);
		}
		const float directionX = sumX / static_cast<float>(zone.size()) - static_cast<float>(size.x) * 0.5f;
		const float directionZ = sumZ / static_cast<float>(zone.size()) - static_cast<float>(size.z) * 0.5f;
		if (std::abs(directionX) < 0.01f && std::abs(directionZ) < 0.01f)
		{
			return 225.0f;
		}
		return spk::radianToDegree(std::atan2(directionX, directionZ));
	}

	void BattleScene::begin(BattleContext &p_context, BattleCoordinator &p_coordinator)
	{
		_banner.hideResult();
		_resultScreen.bind(p_context);
		_unitViews.begin(p_context);
		_battleEventContract = p_context.events().battleEventOccurred.subscribe([this](const BattleEvent *p_event) {
			const TurnStartedEvent *turnStarted = p_event != nullptr ? p_event->getIf<TurnStartedEvent>() : nullptr;
			if (turnStarted == nullptr || turnStarted->context.caster == nullptr ||
				!turnStarted->context.caster->boardPosition.has_value())
			{
				return;
			}
			_turnFeedbackCell = *turnStarted->context.caster->boardPosition;
			_turnFeedbackSeconds = 1.0f;
			_overlayState.set(OverlayCategory::Hovered, {*_turnFeedbackCell});
		});
		_impressedContract = p_context.events().creatureImpressed.subscribe([this](BattleUnit *p_unit) {
			if (p_unit != nullptr)
			{
				_banner.showMessage(p_unit->displayName() + " was impressed!");
			}
		});
		std::array<AtlasCell, OverlayCategoryCount> masks{};
		const auto &overlayMasks = _registries.gameRules().overlayMasks;
		for (std::size_t index = 0; index < OverlayCategoryCount; ++index)
		{
			const auto cell = overlayMasks.find(std::string(overlayMaskKey(static_cast<OverlayCategory>(index))));
			if (cell != overlayMasks.end())
			{
				masks[index] = AtlasCell{cell->second[0], cell->second[1]};
			}
		}

		_overlayState.clearAll();
		_overlayView.bind(_overlayState, _worldLookup, p_context.board.worldAnchor(), masks);
		_input.bind(p_context, p_coordinator);
		_input.setBusyPredicate([this] {
			return _unitViews.viewBusy();
		});
		_hud.bind(p_context, _input, p_coordinator);

		// Work-item 1: flash the deployment strips at battle entry (placement is auto/instant in M1).
		// The first player interaction clears them (BattleInputController manages the category).
		const DeploymentZones &zones = p_context.board.deploymentZones();
		std::vector<spk::Vector3Int> deployment = zones.player;
		deployment.insert(deployment.end(), zones.enemy.begin(), zones.enemy.end());
		_overlayState.set(OverlayCategory::Deployment, std::move(deployment));

		const spk::Vector3Int size = p_context.board.terrain().size();
		const TacticalCameraLogic::Pose from{_camera.position(), _camera.target()};
		_camera3dController.begin(
			from,
			p_context.board.worldAnchor(),
			spk::Vector2Int{size.x, size.z},
			_playerYaw(p_context));
	}

	void BattleScene::synchronizeUnits()
	{
		_unitViews.synchronize();
	}

	void BattleScene::update(float p_seconds)
	{
		if (_turnFeedbackSeconds <= 0.0f)
		{
			return;
		}
		_turnFeedbackSeconds -= std::max(0.0f, p_seconds);
		if (_turnFeedbackSeconds <= 0.0f && _turnFeedbackCell.has_value())
		{
			const std::vector<spk::Vector3Int> expected{*_turnFeedbackCell};
			if (_overlayState.cells(OverlayCategory::Hovered) == expected)
			{
				_overlayState.clear(OverlayCategory::Hovered);
			}
			_turnFeedbackCell.reset();
		}
	}

	void BattleScene::showResult(BattleSide p_winner, std::function<void()> p_confirmation)
	{
		_input.unbind();
		_hud.unbind();
		_banner.hideResult();
		_resultScreen.show(p_winner, std::move(p_confirmation));
	}

	void BattleScene::end()
	{
		_battleEventContract.resign();
		_impressedContract.resign();
		_turnFeedbackCell.reset();
		_turnFeedbackSeconds = 0.0f;
		_banner.hideResult();
		_resultScreen.unbind();
		_unitViews.end();
		_overlayView.clearBinding();
		_overlayState.clearAll();
		_input.unbind();
		_hud.unbind();
		_camera3dController.end();
	}
}
