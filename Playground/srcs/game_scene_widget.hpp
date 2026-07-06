#pragma once

#include <atomic>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

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
#include "ui/battle_hud.hpp"
#include "ui/battle_result_screen.hpp"
#include "ui/exploration_hud.hpp"
#include "world/chunk_coordinates.hpp"

namespace pg
{
	class Registries;
	class BattleContext;
	class VoxelRegistry;
	class ExplorationInputLogic;
	class BattleInput;
	class TacticalCameraLogic;
	class BattleScene;
	class EncounterService;
	class FeatBoardService;
	class TamingService;
	class TrainerSightLogic;
	class BattleUnitViewLogic;
	class ActorPathLogic;
	class PortalService;
	class ProceduralWorld;
	class ProceduralChunkProvider;
	class WorldStreamer;

	// Step 7 exploration scene: chunked terrain, click-to-move actor, hover mask and orbit camera.
	class GameSceneWidget : public spk::GameEngineWidget
	{
	private:
		ModeManager _modeManager;
		GameContext &_context;
		const Registries *_registries = nullptr;
		bool _generatedWorld = true;
		std::uint64_t _worldSeed = 1;
		std::unique_ptr<ProceduralWorld> _proceduralWorld;
		std::unique_ptr<ProceduralChunkProvider> _proceduralProvider;
		std::unique_ptr<WorldStreamer> _worldStreamer;
		std::optional<ChunkCoordinates> _streamingFocus;

		spk::SpriteSheet _texture;
		spk::SpriteSheet _maskTexture;
		std::shared_ptr<spk::TextureMesh3D> _playerMesh;
		spk::Entity3D _playerEntity;
		spk::Entity3D _hoverEntity;
		pg::Actor *_player = nullptr;
		pg::ExplorationInputLogic *_inputLogic = nullptr;
		pg::ActorPathLogic *_pathLogic = nullptr;
		std::vector<std::unique_ptr<spk::Entity3D>> _trainerEntities;
		std::unique_ptr<TrainerSightLogic> _trainerSightLogic;

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
		std::unique_ptr<FeatBoardService> _featBoardService;
		std::unique_ptr<TamingService> _tamingService;
		std::unique_ptr<EncounterService> _encounterService;
		std::unique_ptr<PortalService> _portalService;

		std::shared_ptr<spk::SpriteSheet> _uiSprites;
		BattleHud _battleHud;
		ExplorationHud _explorationHud;
		spk::DebugOverlay _overlay;
		BattleBanner _battleBanner;
		BattleResultScreen _battleResultScreen;
		spk::ContractProvider<const EncounterSpawn &>::Contract _encounterContract;

		mutable std::atomic<long long> _renderDurationNs{0};
		std::atomic<long long> _updateDurationNs{0};
		mutable std::atomic<std::size_t> _meshCount{0};
		mutable std::atomic<std::size_t> _triangleCount{0};

		void _buildScene(const Registries &p_registries);
		void _configureOverlay();
		void _refreshOverlay(const spk::UpdateTick &p_tick);
		void _rebuildMapActors();
		void _requestQuit();

	protected:
		void _onGeometryChange() override;
		void _onUpdate(const spk::UpdateTick &p_tick) override;
		void _onKeyPressedEvent(spk::KeyPressedEvent &p_event) override;
		[[nodiscard]] spk::RenderUnit _buildRenderUnit() const override;

	public:
		GameSceneWidget(
			const std::string &p_name,
			spk::Widget *p_parent,
			GameContext &p_context,
			const Registries &p_registries,
			bool p_generatedWorld = true,
			std::uint64_t p_worldSeed = 1);
		~GameSceneWidget() override;
	};
}
