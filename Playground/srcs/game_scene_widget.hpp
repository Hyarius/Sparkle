#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "components/actor.hpp"
#include "core/game_context.hpp"
#include "structures/game_engine/spk_camera_3d.hpp"
#include "structures/game_engine/spk_entity_3d.hpp"
#include "structures/graphics/geometry/spk_texture_mesh_3d.hpp"
#include "structures/graphics/texture/spk_sprite_sheet.hpp"
#include "structures/widget/spk_debug_overlay.hpp"
#include "structures/widget/spk_game_engine_widget.hpp"
#include "world/chunk_coordinates.hpp"

namespace spk
{
	class VoxelChunkStreamer;
}

namespace pg
{
	class Registries;
	class ExplorationInputLogic;
	class ActorPathLogic;
	class CameraControllerLogic;
	class PlanChunkProvider;
	class VoxelWorld;
	class WorldNavigation;
	struct WorldPlan;

	struct GameSceneConstructionOptions
	{
		std::uint64_t worldSeed = 1;
		bool writeWorldMapPreview = true;
		// Optional diagnostics/failure-injection checkpoint. It runs after the initial
		// chunks and navigation exist but before either object is published to GameContext.
		std::function<void(const VoxelWorld &, const WorldNavigation &)> afterInitialWorldReady;
	};

	class GameSceneWidget : public spk::GameEngineWidget
	{
	private:
		GameContext &_context;
		bool _previousExplorationActive = false;
		std::uint64_t _worldSeed = 1;
		std::shared_ptr<const WorldPlan> _worldPlan;
		std::unique_ptr<PlanChunkProvider> _terrainProvider;
		// Scene construction is transactional: these objects remain widget-owned until
		// every potentially-throwing setup step has completed. Their declaration order
		// makes navigation release before the world during constructor unwinding.
		std::unique_ptr<VoxelWorld> _stagedWorld;
		std::unique_ptr<WorldNavigation> _stagedNavigation;
		std::optional<ChunkCoordinates> _streamingFocus;

		spk::SpriteSheet _texture;
		spk::SpriteSheet _maskTexture;
		std::shared_ptr<spk::TextureMesh3D> _playerMesh;
		spk::Entity3D _playerEntity;
		spk::Entity3D _hoverEntity;
		Actor *_player = nullptr;
		spk::VoxelChunkStreamer *_streamer = nullptr;
		ExplorationInputLogic *_inputLogic = nullptr;
		ActorPathLogic *_pathLogic = nullptr;
		CameraControllerLogic *_cameraLogic = nullptr;

		// Door/exit-pad cells mapped to their teleport destination (from the world
		// plan's portals). Stepping on a key cell queues the teleport; it executes
		// after the engine update so it never mutates the actor mid-advance.
		std::unordered_map<spk::Vector3Int, spk::Vector3Int> _portalTargets;
		std::optional<spk::Vector3Int> _pendingTeleport;
		spk::ContractProvider<spk::Vector3Int>::Contract _playerMovedContract;

		spk::Entity3D _cameraEntity;
		spk::Camera3D *_camera = nullptr;
		spk::DebugOverlay _overlay;

		mutable std::atomic<long long> _renderDurationNs{0};
		std::atomic<long long> _updateDurationNs{0};

		// Profiler probe names currently mirrored as overlay rows (after the fixed rows),
		// in snapshot order. Grows the first time the voxel logic registers its probes.
		std::vector<std::string> _profilerRowNames;

		void _buildScene(const Registries &p_registries, const GameSceneConstructionOptions &p_options);
		void _executeTeleport(const spk::Vector3Int &p_target);
		void _configureOverlay();
		[[nodiscard]] std::size_t _profilerSectionRowCount() const;
		void _applyOverlayGeometry();
		void _syncProfilerRows(const std::vector<std::string> &p_names);
		void _refreshOverlay(const spk::UpdateContext &p_tick);

	protected:
		void _onGeometryChange() override;
		void _onUpdate(const spk::UpdateContext &p_tick) override;
		void _onKeyPressedEvent(spk::KeyPressedEvent &p_event) override;
		[[nodiscard]] spk::RenderUnit _buildRenderUnit() const override;

	public:
		GameSceneWidget(
			const std::string &p_name,
			spk::Widget *p_parent,
			GameContext &p_context,
			const Registries &p_registries,
			std::uint64_t p_worldSeed = 1);
		GameSceneWidget(
			const std::string &p_name,
			spk::Widget *p_parent,
			GameContext &p_context,
			const Registries &p_registries,
			const GameSceneConstructionOptions &p_options);
		~GameSceneWidget() override;
	};
}
