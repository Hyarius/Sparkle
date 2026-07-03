#pragma once

#include <atomic>
#include <cstddef>
#include <memory>
#include <string>

#include "battle/board_overlay_state.hpp"
#include "components/actor.hpp"
#include "components/board_overlay_view.hpp"
#include "core/mode_manager.hpp"
#include "encounters/encounter_types.hpp"
#include "structures/game_engine/spk_camera_3d.hpp"
#include "structures/game_engine/spk_entity_3d.hpp"
#include "structures/graphics/geometry/spk_texture_mesh_3d.hpp"
#include "structures/graphics/texture/spk_sprite_sheet.hpp"
#include "structures/widget/spk_debug_overlay.hpp"
#include "structures/widget/spk_game_engine_widget.hpp"
#include "ui/battle_banner.hpp"

namespace pg
{
	class Registries;
	class VoxelRegistry;
	class ExplorationInputLogic;
	class BattleInput;
	class TacticalCameraLogic;
	class BattleScene;
	class EncounterService;
	class BattleUnitViewLogic;

	// Step 7 exploration scene: chunked terrain, click-to-move actor, hover mask and orbit camera.
	class GameSceneWidget : public spk::GameEngineWidget
	{
	private:
		ModeManager _modeManager;
		GameContext &_context;
		const Registries *_registries = nullptr;

		spk::SpriteSheet _texture;
		spk::SpriteSheet _maskTexture;
		std::shared_ptr<spk::TextureMesh3D> _playerMesh;
		spk::Entity3D _playerEntity;
		spk::Entity3D _hoverEntity;
		pg::Actor *_player = nullptr;
		pg::ExplorationInputLogic *_inputLogic = nullptr;

		spk::Entity3D _cameraEntity;
		spk::Camera3D *_camera = nullptr;

		// Battle presentation (built once; inert until a battle begins).
		BoardOverlayState _overlayState;
		spk::Entity3D _overlayEntity;
		pg::BoardOverlayView *_overlayView = nullptr;
		pg::BattleInput *_battleInput = nullptr;
		pg::TacticalCameraLogic *_tacticalCamera = nullptr;
		pg::BattleUnitViewLogic *_battleUnitViews = nullptr;
		std::unique_ptr<BattleScene> _battleScene;
		std::unique_ptr<EncounterService> _encounterService;

		spk::DebugOverlay _overlay;
		BattleBanner _battleBanner;
		spk::ContractProvider<const EncounterSpawn &>::Contract _encounterContract;

		mutable std::atomic<long long> _renderDurationNs{0};
		std::atomic<long long> _updateDurationNs{0};
		mutable std::atomic<std::size_t> _meshCount{0};
		mutable std::atomic<std::size_t> _triangleCount{0};

		void _buildScene(const Registries &p_registries);
		void _configureOverlay();
		void _refreshOverlay(const spk::UpdateTick &p_tick);

	protected:
		void _onGeometryChange() override;
		void _onUpdate(const spk::UpdateTick &p_tick) override;
		[[nodiscard]] spk::RenderUnit _buildRenderUnit() const override;

	public:
		GameSceneWidget(
			const std::string &p_name,
			spk::Widget *p_parent,
			GameContext &p_context,
			const Registries &p_registries);
		~GameSceneWidget() override;
	};
}
