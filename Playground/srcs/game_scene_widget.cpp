#include "game_scene_widget.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

#include "battle/battle_input.hpp"
#include "core/battle_mode.hpp"
#include "core/battle_scene.hpp"
#include "core/registries.hpp"
#include "encounters/encounter_emitter.hpp"
#include "encounters/encounter_service.hpp"
#include "logics/actor_path_logic.hpp"
#include "logics/battle_unit_view_logic.hpp"
#include "logics/board_overlay_logic.hpp"
#include "logics/camera_controller_logic.hpp"
#include "logics/chunk_render_logic.hpp"
#include "logics/chunk_synchronization_logic.hpp"
#include "logics/exploration_input_logic.hpp"
#include "logics/tactical_camera_logic.hpp"
#include "structures/game_engine/spk_texture_mesh_render_logic.hpp"
#include "structures/game_engine/spk_texture_mesh_renderer_3d.hpp"
#include "structures/graphics/geometry/spk_primitive_object.hpp"
#include "structures/math/spk_vector3.hpp"
#include "world/voxel_world.hpp"
#include "world/world_navigation.hpp"

#ifndef PG_RESOURCE_DIR
#	define PG_RESOURCE_DIR "."
#endif

namespace
{
	enum OverlayRow : std::size_t
	{
		CameraPosition = 0,
		PlayerCell,
		HoveredCell,
		LoadedChunks,
		Meshes,
		Triangles,
		UpdateTime,
		RenderTime,
		DeltaTime,
		Encounter,
		RowCount
	};

	[[nodiscard]] long long nowNs()
	{
		return std::chrono::duration_cast<std::chrono::nanoseconds>(
				   std::chrono::steady_clock::now().time_since_epoch())
			.count();
	}

	[[nodiscard]] std::string formatVector(const spk::Vector3 &p_value)
	{
		char buffer[80];
		std::snprintf(buffer, sizeof(buffer), "(%.2f, %.2f, %.2f)", p_value.x, p_value.y, p_value.z);
		return buffer;
	}

	[[nodiscard]] std::string formatVector(const spk::Vector3Int &p_value)
	{
		return "(" + std::to_string(p_value.x) + ", " + std::to_string(p_value.y) + ", " +
			   std::to_string(p_value.z) + ")";
	}

	[[nodiscard]] std::string formatFloat(float p_value, const char *p_suffix = "")
	{
		char buffer[64];
		std::snprintf(buffer, sizeof(buffer), "%.2f%s", p_value, p_suffix);
		return buffer;
	}

}

namespace pg
{
	GameSceneWidget::GameSceneWidget(
		const std::string &p_name,
		spk::Widget *p_parent,
		GameContext &p_context,
		const Registries &p_registries) :
		spk::GameEngineWidget(p_name, p_parent),
		_modeManager(p_context),
		_context(p_context),
		_overlay(p_name + "/DebugOverlay", this),
		_battleBanner(p_name + "/BattleBanner", this),
		_encounterContract(_context.events.encounterTriggered.subscribe([this](const EncounterSpawn &p_spawn) {
			_overlay.setText(Encounter, 1, encounterSummary(p_spawn));
		}))
	{
		_buildScene(p_registries);
		_configureOverlay();
		_modeManager.enterExploration();
		activate();
	}

	GameSceneWidget::~GameSceneWidget()
	{
		// Detach presentation before tearing down so a shutdown mid-battle can't dangle: the
		// ModeManager destructs last and may call BattleMode::exit -> BattleScene::end.
		if (pg::BattleMode *battleMode = _modeManager.battleMode(); battleMode != nullptr)
		{
			battleMode->setScene(nullptr);
		}
		if (_battleScene != nullptr)
		{
			_battleScene->end();
		}
		_encounterService.reset();
		_battleScene.reset();
		_context.world.encounterEmitter.reset();
		_context.world.navigation.reset();
		_context.world.world.reset();
		_context.world.activeMap = nullptr;
		_context.world.activeBiome = nullptr;
	}

	void GameSceneWidget::_buildScene(const Registries &p_registries)
	{
		const std::filesystem::path texturePath =
			std::filesystem::path(PG_RESOURCE_DIR) / "textures" / "voxels.png";
		_texture.loadFromFile(texturePath, {8u, 8u});
		_maskTexture.loadFromFile(
			std::filesystem::path(PG_RESOURCE_DIR) / "textures" / "mask.png", {4u, 4u});

		_registries = &p_registries;

		spk::GameEngine &engine = gameEngine();
		engine.add<pg::ChunkSynchronizationLogic>();
		engine.add<pg::ChunkRenderLogic>(_texture, &_maskTexture);
		engine.add<spk::TextureMeshRenderLogic>(false);
		_battleUnitViews = &engine.add<pg::BattleUnitViewLogic>(engine, &_texture, &p_registries.models());
		// Overlay draws last so its translucent masks blend over terrain and units.
		engine.add<pg::BoardOverlayLogic>(_maskTexture);

		_camera = &_cameraEntity.addComponent<spk::Camera3D>();
		_camera->setPerspective(60.0f, 0.1f, 1000.0f);
		_camera->setPosition({78.0f, 58.0f, -58.0f});
		_camera->setUp({0.0f, 1.0f, 0.0f});
		_camera->setTarget({32.0f, 2.0f, 32.0f});
		_camera->makeMain();
		engine.addEntity(&_cameraEntity);

		_context.world.activeMap = &p_registries.maps().get("m1-testground");
		_context.world.activeBiome = &p_registries.biomes().get(_context.world.activeMap->biome);
		_context.world.world = std::make_unique<VoxelWorld>(p_registries.voxels(), &engine);
		_context.world.world->loadFromMap(*_context.world.activeMap);
		_context.world.navigation = std::make_unique<WorldNavigation>(
			*_context.world.world,
			TraversalBounds{{0, 0, 0}, _context.world.activeMap->size()},
			static_cast<float>(p_registries.gameRules().maxVerticalTraversalGap));
		_context.world.encounterEmitter = std::make_unique<EncounterEmitter>(
			_context,
			spk::Vector2Int{
				p_registries.gameRules().defaultBoardSize[0],
				p_registries.gameRules().defaultBoardSize[1]});

		const MapMarker *spawnMarker = _context.world.activeMap->marker("playerSpawn");
		if (spawnMarker == nullptr)
		{
			throw std::runtime_error("m1-testground is missing playerSpawn marker");
		}
		const auto spawnCell = _context.world.navigation->topStandableInColumn(
			spawnMarker->at.x, spawnMarker->at.z);
		if (!spawnCell.has_value())
		{
			throw std::runtime_error("playerSpawn column has no standable cell");
		}

		_playerMesh = std::make_shared<spk::TextureMesh3D>(spk::PrimitiveObject::CreateCube(0.65f));
		_player = &_playerEntity.addComponent<Actor>();
		_player->cell = *spawnCell;
		_player->player = true;
		_player->speed = 5.0f;
		spk::TextureMeshRenderer3D &playerRenderer = _playerEntity.addComponent<spk::TextureMeshRenderer3D>();
		playerRenderer.setMesh(_playerMesh);
		playerRenderer.setTexture(&_texture);
		playerRenderer.setTint({0.95f, 0.95f, 1.0f, 1.0f});
		engine.addEntity(&_playerEntity);

		spk::TextureMeshRenderer3D &hoverRenderer = _hoverEntity.addComponent<spk::TextureMeshRenderer3D>();
		hoverRenderer.setMesh(std::make_shared<spk::TextureMesh3D>());
		hoverRenderer.setTexture(&_maskTexture);
		hoverRenderer.setTint({1.0f, 1.0f, 1.0f, 0.7f});
		hoverRenderer.setTranslucent(true);
		engine.addEntity(&_hoverEntity);

		_overlayView = &_overlayEntity.addComponent<pg::BoardOverlayView>();
		engine.addEntity(&_overlayEntity);

		ActorPathLogic &pathLogic = engine.add<ActorPathLogic>(
			_context.events,
			*_context.world.navigation,
			*_context.world.world,
			[this] {
				return _context.world.explorationActive;
			});
		pathLogic.placeAtCell(*_player);
		engine.add<CameraControllerLogic>(_context, *_camera);
		const auto viewportSize = [this]() {
			return spk::Vector2(
				static_cast<float>(geometry().width()),
				static_cast<float>(geometry().height()));
		};
		const auto hovered = p_registries.gameRules().overlayMasks.at("hovered");
		const auto invalid = p_registries.gameRules().overlayMasks.at("invalid");
		_inputLogic = &engine.add<ExplorationInputLogic>(
			_context,
			*_context.world.world,
			*_context.world.navigation,
			*_camera,
			hoverRenderer,
			viewportSize,
			AtlasCell{hovered[0], hovered[1]},
			AtlasCell{invalid[0], invalid[1]});

		_battleInput = &engine.add<pg::BattleInput>(*_camera, _overlayState, viewportSize);
		_tacticalCamera = &engine.add<pg::TacticalCameraLogic>(*_camera);

		_battleScene = std::make_unique<BattleScene>(
			*_overlayView,
			_overlayState,
			*_battleInput,
			*_tacticalCamera,
			*_camera,
			*_context.world.world,
			p_registries,
			*_battleUnitViews,
			_battleBanner);
		if (pg::BattleMode *battleMode = _modeManager.battleMode(); battleMode != nullptr)
		{
			battleMode->setScene(_battleScene.get());
		}
		_encounterService = std::make_unique<EncounterService>(
			_context, p_registries, [this]() {
				return _player->cell;
			});

		std::cout << "Loaded map 'm1-testground' as "
				  << _context.world.world->loadedChunkCount() << " chunks" << std::endl;
	}

	void GameSceneWidget::_configureOverlay()
	{
		_overlay.configureRows(RowCount, 2);
		_overlay.setMaxGlyphSize(16);
		_overlay.setFontOutlineSize(1);

		_overlay.setText(CameraPosition, 0, "Camera position");
		_overlay.setText(PlayerCell, 0, "Player cell");
		_overlay.setText(HoveredCell, 0, "Hovered cell");
		_overlay.setText(LoadedChunks, 0, "Loaded chunks");
		_overlay.setText(Meshes, 0, "Meshes rendered");
		_overlay.setText(Triangles, 0, "Triangles rendered");
		_overlay.setText(UpdateTime, 0, "Update duration");
		_overlay.setText(RenderTime, 0, "Render duration");
		_overlay.setText(DeltaTime, 0, "Frame delta");
		_overlay.setText(Encounter, 0, "Last encounter");
		_overlay.setText(Encounter, 1, "-");
	}

	void GameSceneWidget::_onGeometryChange()
	{
		spk::GameEngineWidget::_onGeometryChange();

		if (_camera != nullptr)
		{
			_camera->setViewportSize(
				static_cast<float>(geometry().width()),
				static_cast<float>(geometry().height()));
		}
		if (_player != nullptr)
		{
			_overlay.setText(PlayerCell, 1, formatVector(_player->cell));
		}
		if (_inputLogic != nullptr && _inputLogic->hoveredCell().has_value())
		{
			_overlay.setText(HoveredCell, 1, formatVector(*_inputLogic->hoveredCell()));
		}
		else
		{
			_overlay.setText(HoveredCell, 1, "-");
		}

		const std::uint32_t overlayWidth = std::min<std::uint32_t>(geometry().size.x, 360u);
		_overlay.setGeometry(spk::Rect2D({8, 8}, {overlayWidth, 26u * RowCount}));

		const std::uint32_t bannerWidth = std::min<std::uint32_t>(geometry().size.x, 420u);
		const std::uint32_t bannerHeight = std::min<std::uint32_t>(geometry().size.y, 140u);
		_battleBanner.setGeometry(spk::Rect2D({static_cast<int>((geometry().size.x - bannerWidth) / 2), static_cast<int>((geometry().size.y - bannerHeight) / 2)}, {bannerWidth, bannerHeight}));
	}

	void GameSceneWidget::_onUpdate(const spk::UpdateTick &p_tick)
	{
		const long long start = nowNs();
		_modeManager.update(p_tick);
		spk::GameEngineWidget::_onUpdate(p_tick);

		_updateDurationNs.store(nowNs() - start, std::memory_order_relaxed);

		invalidateRenderUnit();
		_refreshOverlay(p_tick);
	}

	spk::RenderUnit GameSceneWidget::_buildRenderUnit() const
	{
		const long long start = nowNs();
		spk::RenderUnit unit = spk::GameEngineWidget::_buildRenderUnit();
		_renderDurationNs.store(nowNs() - start, std::memory_order_relaxed);

		_meshCount.store(pg::ChunkRenderLogic::lastMeshCount(), std::memory_order_relaxed);
		_triangleCount.store(pg::ChunkRenderLogic::lastTriangleCount(), std::memory_order_relaxed);
		return unit;
	}

	void GameSceneWidget::_refreshOverlay(const spk::UpdateTick &p_tick)
	{
		if (_camera != nullptr)
		{
			_overlay.setText(CameraPosition, 1, formatVector(_camera->position()));
		}
		_overlay.setText(
			LoadedChunks,
			1,
			_context.world.world == nullptr ? "0" : std::to_string(_context.world.world->loadedChunkCount()));
		_overlay.setText(Meshes, 1, std::to_string(_meshCount.load(std::memory_order_relaxed)));
		_overlay.setText(Triangles, 1, std::to_string(_triangleCount.load(std::memory_order_relaxed)));
		_overlay.setText(
			UpdateTime,
			1,
			formatFloat(static_cast<float>(_updateDurationNs.load(std::memory_order_relaxed)) / 1.0e6f, " ms"));
		_overlay.setText(
			RenderTime,
			1,
			formatFloat(static_cast<float>(_renderDurationNs.load(std::memory_order_relaxed)) / 1.0e6f, " ms"));
		_overlay.setText(DeltaTime, 1, std::to_string(p_tick.deltaTime.milliseconds()) + " ms");
	}
}
