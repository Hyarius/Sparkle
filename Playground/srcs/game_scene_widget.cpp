#include "game_scene_widget.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

#include "core/registries.hpp"
#include "logics/actor_path_logic.hpp"
#include "logics/camera_controller_logic.hpp"
#include "logics/chunk_render_logic.hpp"
#include "logics/chunk_synchronization_logic.hpp"
#include "logics/exploration_input_logic.hpp"
#include "structures/game_engine/spk_texture_mesh_render_logic.hpp"
#include "structures/game_engine/spk_texture_mesh_renderer_3d.hpp"
#include "structures/graphics/geometry/spk_primitive_object.hpp"
#include "structures/math/spk_vector3.hpp"
#include "world/generator/procedural_chunk_provider.hpp"
#include "world/generator/procedural_world.hpp"
#include "world/generator/worldgen_params.hpp"
#include "world/voxel_world.hpp"
#include "world/world_navigation.hpp"
#include "world/world_streamer.hpp"

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
		const Registries &p_registries,
		std::uint64_t p_worldSeed) :
		spk::GameEngineWidget(p_name, p_parent),
		_context(p_context),
		_worldSeed(p_worldSeed),
		_overlay(p_name + "/DebugOverlay", this)
	{
		_context.world.explorationActive = true;
		_buildScene(p_registries);
		_configureOverlay();
		_overlay.deactivate();
		activate();
	}

	GameSceneWidget::~GameSceneWidget()
	{
		_context.world.explorationActive = false;
		_context.world.navigation.reset();
		_worldStreamer.reset();
		_context.world.world.reset();
		_proceduralProvider.reset();
		_proceduralWorld.reset();
	}

	void GameSceneWidget::_buildScene(const Registries &p_registries)
	{
		_texture.loadFromFile(std::filesystem::path(PG_RESOURCE_DIR) / "textures" / "voxels.png", {8u, 8u});
		_maskTexture.loadFromFile(std::filesystem::path(PG_RESOURCE_DIR) / "textures" / "mask.png", {4u, 4u});

		spk::GameEngine &engine = gameEngine();
		engine.add<ChunkSynchronizationLogic>();
		engine.add<ChunkRenderLogic>(_texture, &_maskTexture);
		engine.add<spk::TextureMeshRenderLogic>(false);

		_camera = &_cameraEntity.addComponent<spk::Camera3D>();
		_camera->setPerspective(60.0f, 0.1f, 1000.0f);
		_camera->setUp({0.0f, 1.0f, 0.0f});
		_camera->makeMain();
		engine.addEntity(&_cameraEntity);

		_context.world.world = std::make_unique<VoxelWorld>(p_registries.voxels(), &engine);
		const WorldgenParams params = WorldgenParams::load(
			std::filesystem::path(PG_RESOURCE_DIR) / "data" / "worldgen" / "default.json");
		_proceduralWorld = std::make_unique<ProceduralWorld>(ProceduralWorld::generate(params, _worldSeed));
		_proceduralProvider = std::make_unique<ProceduralChunkProvider>(
			*_proceduralWorld, p_registries.biomes(), p_registries.prefabs(), p_registries.voxels(), _worldSeed);
		spk::Vector3Int spawnCell = _proceduralProvider->spawnCell();
		(void)_context.world.world->loadChunk(ChunkCoordinates::fromWorldCell(spawnCell), *_proceduralProvider);
		(void)_context.world.world->loadChunk(
			ChunkCoordinates::fromWorldCell(spawnCell + spk::Vector3Int{0, 2, 0}), *_proceduralProvider);
		_worldStreamer = std::make_unique<WorldStreamer>(
			*_context.world.world, *_proceduralProvider, spk::Vector3Int{2, 1, 2}, 4);
		_worldStreamer->update(spawnCell);
		_streamingFocus = ChunkCoordinates::fromWorldCell(spawnCell);

		const int margin = 48;
		const TraversalBounds navigationBounds{
			{std::max(0, spawnCell.x - margin), 0, std::max(0, spawnCell.z - margin)},
			{std::min(_proceduralWorld->width(), spawnCell.x + margin + 1),
			 params.height.maxHeight + 16,
			 std::min(_proceduralWorld->height(), spawnCell.z + margin + 1)}};
		_context.world.navigation = std::make_unique<WorldNavigation>(
			*_context.world.world,
			navigationBounds,
			static_cast<float>(p_registries.gameRules().maxVerticalTraversalGap));

		const auto standableSpawn = _context.world.navigation->topStandableInColumn(spawnCell.x, spawnCell.z);
		if (!standableSpawn.has_value())
		{
			throw std::runtime_error("generated spawn column has no standable cell");
		}
		spawnCell = *standableSpawn;
		_camera->setTarget(spk::Vector3(spawnCell));
		_camera->setPosition(spk::Vector3(spawnCell) + spk::Vector3{46.0f, 56.0f, -90.0f});

		_playerMesh = std::make_shared<spk::TextureMesh3D>(spk::PrimitiveObject::CreateCube(0.65f));
		_player = &_playerEntity.addComponent<Actor>();
		_player->cell = spawnCell;
		_player->player = true;
		_player->speed = 5.0f;
		auto &playerRenderer = _playerEntity.addComponent<spk::TextureMeshRenderer3D>();
		playerRenderer.setMesh(_playerMesh);
		playerRenderer.setTexture(&_texture);
		playerRenderer.setTint({0.95f, 0.95f, 1.0f, 1.0f});
		engine.addEntity(&_playerEntity);

		auto &hoverRenderer = _hoverEntity.addComponent<spk::TextureMeshRenderer3D>();
		hoverRenderer.setMesh(std::make_shared<spk::TextureMesh3D>());
		hoverRenderer.setTexture(&_maskTexture);
		hoverRenderer.setTint({1.0f, 1.0f, 1.0f, 0.7f});
		hoverRenderer.setTranslucent(true);
		engine.addEntity(&_hoverEntity);

		_pathLogic = &engine.add<ActorPathLogic>(
			_context.events,
			*_context.world.navigation,
			*_context.world.world,
			[this] { return _context.world.explorationActive; });
		_pathLogic->placeAtCell(*_player);
		engine.add<CameraControllerLogic>(_context, *_camera);
		const auto viewportSize = [this] {
			return spk::Vector2(static_cast<float>(geometry().width()), static_cast<float>(geometry().height()));
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

		std::cout << "Streaming generated world from " << _context.world.world->loadedChunkCount()
				  << " initial chunks" << std::endl;
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
	}

	void GameSceneWidget::_onGeometryChange()
	{
		spk::GameEngineWidget::_onGeometryChange();
		if (_camera != nullptr)
		{
			_camera->setViewportSize(static_cast<float>(geometry().width()), static_cast<float>(geometry().height()));
		}
		const std::uint32_t overlayWidth = std::min<std::uint32_t>(geometry().size.x, 360u);
		_overlay.setGeometry(spk::Rect2D({8, 8}, {overlayWidth, 26u * RowCount}));
	}

	void GameSceneWidget::_onUpdate(const spk::UpdateTick &p_tick)
	{
		const long long start = nowNs();
		if (_worldStreamer != nullptr && _player != nullptr && _proceduralWorld != nullptr)
		{
			_worldStreamer->update(_player->cell);
			const ChunkCoordinates focus = ChunkCoordinates::fromWorldCell(_player->cell);
			if (!_streamingFocus.has_value() || focus != *_streamingFocus)
			{
				_streamingFocus = focus;
				const int margin = 48;
				_context.world.navigation->resetBounds({
					{std::max(0, _player->cell.x - margin), 0, std::max(0, _player->cell.z - margin)},
					{std::min(_proceduralWorld->width(), _player->cell.x + margin + 1),
					 64,
					 std::min(_proceduralWorld->height(), _player->cell.z + margin + 1)}});
			}
		}
		spk::GameEngineWidget::_onUpdate(p_tick);
		_updateDurationNs.store(nowNs() - start, std::memory_order_relaxed);
		invalidateRenderUnit();
		_refreshOverlay(p_tick);
	}

	void GameSceneWidget::_onKeyPressedEvent(spk::KeyPressedEvent &p_event)
	{
		if (p_event->key == spk::Keyboard::F7)
		{
			_overlay.isActivated() ? _overlay.deactivate() : _overlay.activate();
			p_event.consume();
			return;
		}
		spk::GameEngineWidget::_onKeyPressedEvent(p_event);
	}

	spk::RenderUnit GameSceneWidget::_buildRenderUnit() const
	{
		const long long start = nowNs();
		spk::RenderUnit unit = spk::GameEngineWidget::_buildRenderUnit();
		_renderDurationNs.store(nowNs() - start, std::memory_order_relaxed);
		_meshCount.store(ChunkRenderLogic::lastMeshCount(), std::memory_order_relaxed);
		_triangleCount.store(ChunkRenderLogic::lastTriangleCount(), std::memory_order_relaxed);
		return unit;
	}

	void GameSceneWidget::_refreshOverlay(const spk::UpdateTick &p_tick)
	{
		if (_camera != nullptr)
			_overlay.setText(CameraPosition, 1, formatVector(_camera->position()));
		if (_player != nullptr)
			_overlay.setText(PlayerCell, 1, formatVector(_player->cell));
		_overlay.setText(
			HoveredCell,
			1,
			_inputLogic != nullptr && _inputLogic->hoveredCell().has_value()
				? formatVector(*_inputLogic->hoveredCell())
				: "-");
		_overlay.setText(
			LoadedChunks,
			1,
			_context.world.world == nullptr ? "0" : std::to_string(_context.world.world->loadedChunkCount()));
		_overlay.setText(Meshes, 1, std::to_string(_meshCount.load(std::memory_order_relaxed)));
		_overlay.setText(Triangles, 1, std::to_string(_triangleCount.load(std::memory_order_relaxed)));
		_overlay.setText(UpdateTime, 1, formatFloat(static_cast<float>(_updateDurationNs.load()) / 1.0e6f, " ms"));
		_overlay.setText(RenderTime, 1, formatFloat(static_cast<float>(_renderDurationNs.load()) / 1.0e6f, " ms"));
		_overlay.setText(DeltaTime, 1, std::to_string(p_tick.deltaTime.milliseconds()) + " ms");
	}
}
