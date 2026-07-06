#pragma once

#include "structures/design_pattern/spk_contract_provider.hpp"
#include "structures/game_engine/spk_camera_3d.hpp"
#include "structures/math/spk_vector3.hpp"

#include <functional>
#include <optional>

namespace pg
{
	class BattleContext;
	class BattleCoordinator;
	class BoardOverlayView;
	struct BoardOverlayState;
	class BattleInput;
	class TacticalCameraLogic;
	class ICellLookup;
	class Registries;
	class BattleUnitViewLogic;
	class BattleBanner;
	class BattleResultScreen;
	class BattleHud;
	class BattleUnit;
	struct BattleEvent;
	enum class BattleSide;

	// Presentation wiring for a battle: binds the overlay view/input to the live BattleContext,
	// resolves category mask atlas cells from game-rules, and kicks off the tactical camera blend.
	// Owned by GameSceneWidget (where the engine lives); driven by BattleMode's enter/exit so the
	// battle backend itself stays headless (battle.md, rendering-cameras.md §3-4).
	class BattleScene
	{
	private:
		BoardOverlayView &_overlayView;
		BoardOverlayState &_overlayState;
		BattleInput &_input;
		TacticalCameraLogic &_camera3dController;
		spk::Camera3D &_camera;
		const ICellLookup &_worldLookup;
		const Registries &_registries;
		BattleUnitViewLogic &_unitViews;
		BattleHud &_hud;
		BattleBanner &_banner;
		BattleResultScreen &_resultScreen;
		spk::ContractProvider<const BattleEvent *>::Contract _battleEventContract;
		spk::ContractProvider<BattleUnit *>::Contract _impressedContract;
		std::optional<spk::Vector3Int> _turnFeedbackCell;
		float _turnFeedbackSeconds = 0.0f;

		[[nodiscard]] float _playerYaw(const BattleContext &p_context) const;

	public:
		BattleScene(
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
			BattleResultScreen &p_resultScreen);

		void begin(BattleContext &p_context, BattleCoordinator &p_coordinator);
		void synchronizeUnits();
		void update(float p_seconds);
		void showResult(BattleSide p_winner, std::function<void()> p_confirmation);
		void end();
	};
}
