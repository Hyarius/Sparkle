#include "game_scene_widget.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <numbers>
#include <stdexcept>
#include <string>
#include <unordered_set>

#include "core/registries.hpp"
#include "logics/actor_path_logic.hpp"
#include "logics/camera_controller_logic.hpp"
#include "logics/day_time_management_logic.hpp"
#include "logics/exploration_input_logic.hpp"
#include "structures/game_engine/rendering/spk_scene_lighting_render_feature.hpp"
#include "structures/game_engine/spk_texture_mesh_render_logic.hpp"
#include "structures/game_engine/spk_texture_mesh_renderer_3d.hpp"
#include "structures/graphics/geometry/spk_primitive_object.hpp"
#include "structures/math/spk_vector3.hpp"
#include "structures/system/spk_profiler.hpp"
#include "structures/voxel/spk_voxel_chunk.hpp"
#include "structures/voxel/spk_voxel_chunk_render_logic.hpp"
#include "structures/voxel/spk_voxel_chunk_streamer.hpp"
#include "structures/voxel/spk_voxel_chunk_streamer_logic.hpp"
#include "structures/voxel/spk_voxel_chunk_transparent_render_logic.hpp"
#include "structures/voxel/spk_voxel_fluid_simulation_logic.hpp"
#include "structures/voxel/spk_voxel_fluid_simulator.hpp"
#include "structures/voxel/spk_voxel_grid.hpp"
#include "structures/voxel/spk_voxel_map.hpp"
#include "structures/voxel/spk_voxel_shape.hpp"
#include "world/generator/plan_chunk_provider.hpp"
#include "world/generator/world_map_image.hpp"
#include "world/generator/world_plan.hpp"
#include "world/voxel_world.hpp"
#include "world/world_navigation.hpp"

#ifndef PG_RESOURCE_DIR
#	define PG_RESOURCE_DIR "."
#endif

namespace
{
	// Chunks kept active around the player, per horizontal axis (inclusive radius). Mirrors
	// the historical world-streamer radius.
	constexpr spk::Vector3Int StreamViewRange{8, 1, 8};

	enum OverlayRow : std::size_t
	{
		CameraPosition = 0,
		PlayerCell,
		HoveredCell,
		LoadedChunks,
		UpdateTime,
		RenderTime,
		DeltaTime,
		TimeOfDay,
		SelectedLights,
		RowCount
	};

	// Profiler rows use four columns so each statistic is its own label.
	enum ProfilerColumn : std::size_t
	{
		ProbeName = 0,
		ProbeMax,
		ProbeAvg,
		ProbeMin,
		ProfilerColumnCount
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

	[[nodiscard]] double toMilliseconds(const spk::Duration &p_duration)
	{
		return static_cast<double>(p_duration.nanoseconds()) / 1.0e6;
	}

	// Bare value; the profiler table's header row names each column (Max / Average / Min).
	[[nodiscard]] std::string formatProbeValue(const spk::Duration &p_duration)
	{
		char buffer[32];
		std::snprintf(buffer, sizeof(buffer), "%.2f ms", toMilliseconds(p_duration));
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
		GameSceneWidget(
			p_name,
			p_parent,
			p_context,
			p_registries,
			GameSceneConstructionOptions{.worldSeed = p_worldSeed})
	{
	}

	GameSceneWidget::GameSceneWidget(
		const std::string &p_name,
		spk::Widget *p_parent,
		GameContext &p_context,
		const Registries &p_registries,
		const GameSceneConstructionOptions &p_options) :
		spk::GameEngineWidget(p_name, p_parent),
		_context(p_context),
		_previousExplorationActive(p_context.world.explorationActive),
		_worldSeed(p_options.worldSeed),
		_overlay(p_name + "/DebugOverlay", this)
	{
		_buildScene(p_registries, p_options);
		_configureOverlay();
		_overlay.deactivate();
		activate();

		// Commit only after the last potentially-throwing operation. Until this point,
		// member unwinding destroys navigation and the map while the base engine is alive.
		_context.world.navigation.reset();
		_context.world.world.reset();
		_context.world.world = std::move(_stagedWorld);
		_context.world.navigation = std::move(_stagedNavigation);
		_context.world.explorationActive = true;
	}

	GameSceneWidget::~GameSceneWidget()
	{
		for (auto &[position, entity] : _voxelLightEntities)
		{
			(void)position;
			gameEngine().removeEntity(entity.get());
		}
		_voxelLightEntities.clear();
		_context.world.explorationActive = _previousExplorationActive;
		_context.world.navigation.reset();
		// Destroys the VoxelWorld (and its spk::VoxelMap), unregistering every chunk entity
		// from the engine while the engine is still alive.
		_context.world.world.reset();
		_terrainProvider.reset();
	}

	void GameSceneWidget::_buildScene(
		const Registries &p_registries,
		const GameSceneConstructionOptions &p_options)
	{
		_texture.loadFromFile(std::filesystem::path(PG_RESOURCE_DIR) / "textures" / "voxels.png", {8u, 8u});
		_maskTexture.loadFromFile(std::filesystem::path(PG_RESOURCE_DIR) / "textures" / "mask.png", {4u, 4u});

		spk::GameEngine &engine = gameEngine();
		// Stream and bake first, then submit all opaque chunk and actor geometry. The
		// lower-priority transparent chunk pass runs last so water blends over the player
		// without preventing the player from writing its depth first.
		engine.add<spk::VoxelChunkStreamerLogic>();
		// Fluid automaton (priority 50, after streaming, before baking): makes worldgen-placed
		// water sources spread and fall; its map edits re-bake chunks like any other cell change.
		engine.add<spk::VoxelFluidSimulationLogic>();
		engine.add<spk::VoxelChunkRenderLogic>(_texture);
		engine.add<spk::TextureMeshRenderLogic>();
		engine.add<spk::VoxelChunkTransparentRenderLogic>(_texture);

		_camera = &_cameraEntity.addComponent<spk::Camera3D>();
		_camera->setPerspective(60.0f, 0.1f, 1000.0f);
		_camera->setUp({0.0f, 1.0f, 0.0f});
		_camera->makeMain();
		engine.addEntity(&_cameraEntity);

		_sun = &_sunEntity.addComponent<spk::Light3D>();
		(void)_sun->directional();
		_sun->intensity() = 1.35f;
		_sun->color() = {1.0f, 0.96f, 0.84f, 1.0f};
		// Shadow drawing/sampling is not wired through geometry yet. Keep the
		// authored intent off until that path is complete, avoiding empty cascade
		// construction during the daylight preview.
		_sun->castsShadows() = true;
		_sun->selectionPriority() = 1000;
		_sunEntity.transform().setRotation(spk::Quaternion::lookAt({0.0f, 0.0f, 0.0f}, {-0.35f, -0.85f, -0.40f}));
		engine.addEntity(&_sunEntity);
		_lighting = engine.renderPipeline().findFeature<spk::SceneLightingRenderFeature>();
		if (_lighting == nullptr)
		{
			throw std::runtime_error("Scene lighting feature is unavailable");
		}
		_dayTimeLogic = &engine.add<DayTimeManagementLogic>(_sunEntity, *_lighting);

		// Once-per-seed world skeleton: generate, report, and dump the preview map at the
		// Playground root so the produced world can be inspected outside the game.
		WorldGenConfig worldConfig;
		worldConfig.masterSeed = _worldSeed;
		_worldPlan = std::make_shared<const WorldPlan>(generateWorldPlan(
			worldConfig,
			planBiomesFrom(p_registries.biomes()),
			p_registries.placementRules(),
			p_registries.prefabs(),
			p_registries.interiors()));
		std::cout << _worldPlan->report();

		// Door cells teleport into the composed interiors, exit pads teleport back out.
		for (const PlanPortal &portal : _worldPlan->portals)
		{
			_portalTargets.emplace(portal.from, portal.to);
		}
		_playerMovedContract = _context.events.playerMoved.subscribe([this](spk::Vector3Int p_cell) {
			const auto found = _portalTargets.find(p_cell);
			if (found != _portalTargets.end())
			{
				_pendingTeleport = found->second;
			}
		});
		_terrainProvider = std::make_unique<PlanChunkProvider>(p_registries, *_worldPlan);
		_stagedWorld = std::make_unique<VoxelWorld>(
			p_registries.voxels(),
			[provider = _terrainProvider.get()](spk::VoxelChunk &p_chunk) {
				provider->fill(p_chunk);
			},
			&engine);

		spk::Vector3Int spawnCell = _terrainProvider->spawnCell();
		const spk::Vector3Int spawnChunk = spk::VoxelChunk::coordinatesFromWorldCell(spawnCell);

		// Warm up the spawn neighbourhood so the navigation graph has terrain to build on
		// before the streamer takes over on the first tick.
		for (int y = -StreamViewRange.y; y <= StreamViewRange.y; ++y)
		{
			for (int z = -StreamViewRange.z; z <= StreamViewRange.z; ++z)
			{
				for (int x = -StreamViewRange.x; x <= StreamViewRange.x; ++x)
				{
					_stagedWorld->loadChunk(spawnChunk + spk::Vector3Int{x, y, z});
				}
			}
		}

		const int margin = 48;
		const TraversalBounds navigationBounds{
			{spawnCell.x - margin, 0, spawnCell.z - margin},
			{spawnCell.x + margin + 1, _terrainProvider->maxHeight() + 16, spawnCell.z + margin + 1}};
		_stagedNavigation = std::make_unique<WorldNavigation>(
			*_stagedWorld,
			navigationBounds,
			static_cast<float>(p_registries.gameRules().maxVerticalTraversalGap));
		if (p_options.afterInitialWorldReady)
		{
			p_options.afterInitialWorldReady(*_stagedWorld, *_stagedNavigation);
		}

		const auto standableSpawn = _stagedNavigation->topStandableInColumn(spawnCell.x, spawnCell.z);
		if (!standableSpawn.has_value())
		{
			throw std::runtime_error("generated spawn column has no standable cell");
		}
		spawnCell = *standableSpawn;
		// A deliberately obvious point-light fixture for checking local attenuation.
		const spk::Vector3Int glowingVoxelCell = spawnCell + spk::Vector3Int{3, 1, 0};
		_stagedWorld->setCell(glowingVoxelCell, {p_registries.voxels().runtimeId("glowing-voxel")});
		const auto addGlowingFixture = [&](const std::string &id, const spk::Vector3Int &cell) {
			_stagedWorld->setCell(cell, {p_registries.voxels().runtimeId(id)});
		};
		addGlowingFixture("glowing-blue-voxel", glowingVoxelCell + spk::Vector3Int{0, 0, 5});
		addGlowingFixture("glowing-green-voxel", glowingVoxelCell + spk::Vector3Int{0, 0, -5});
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
		_streamer = &_playerEntity.addComponent<spk::VoxelChunkStreamer>(_stagedWorld->map());
		_streamer->setViewRange(StreamViewRange);
		_streamer->setOriginPosition(spawnChunk);
		// The fluid simulator rides the player entity like the streamer: the update loop
		// re-centers it on the player cell. The only Playground policy is the traversal
		// mapping (passable voxels are replaceable by fluid, solid voxels are support);
		// fluid cells themselves are classified by the simulation.
		_fluidSimulator = &_playerEntity.addComponent<spk::VoxelFluidSimulator>(
			_stagedWorld->map(),
			spk::VoxelFluidCellRules{
				.canReplace =
					[&registry = _stagedWorld->registry()](const spk::VoxelCell &p_cell) {
						return registry.definition(p_cell.id).data.traversal == VoxelTraversal::Passable;
					},
				.providesSupport =
					[&registry = _stagedWorld->registry()](const spk::VoxelCell &p_cell) {
						return registry.definition(p_cell.id).data.traversal == VoxelTraversal::Solid;
					}});
		_fluidSimulator->setSimulationCenter(_player->cell);
		engine.addEntity(&_playerEntity);

		auto &hoverRenderer = _hoverEntity.addComponent<spk::TextureMeshRenderer3D>();
		hoverRenderer.setMesh(std::make_shared<spk::TextureMesh3D>());
		hoverRenderer.setTexture(&_maskTexture);
		hoverRenderer.setTint({1.0f, 1.0f, 1.0f, 0.7f});
		hoverRenderer.setTranslucent(true);
		engine.addEntity(&_hoverEntity);

		_pathLogic = &engine.add<ActorPathLogic>(
			_context.events,
			*_stagedNavigation,
			*_stagedWorld,
			[this] {
				return _context.world.explorationActive;
			});
		_pathLogic->placeAtCell(*_player);
		_cameraLogic = &engine.add<CameraControllerLogic>(_context, *_camera);
		const auto viewportSize = [this] {
			return spk::Vector2(static_cast<float>(geometry().width()), static_cast<float>(geometry().height()));
		};
		const auto hovered = p_registries.gameRules().overlayMasks.at("hovered");
		const auto invalid = p_registries.gameRules().overlayMasks.at("invalid");
		_inputLogic = &engine.add<ExplorationInputLogic>(
			_context,
			*_stagedWorld,
			*_stagedNavigation,
			*_camera,
			hoverRenderer,
			viewportSize,
			spk::AtlasCell{hovered[0], hovered[1]},
			spk::AtlasCell{invalid[0], invalid[1]});

		_streamingFocus = spawnChunk;

		if (p_options.writeWorldMapPreview)
		{
			const std::filesystem::path mapPath =
				std::filesystem::path(PG_RESOURCE_DIR).parent_path() / "world_map.png";
			if (writeWorldMapPng(*_worldPlan, mapPath))
			{
				std::cout << "world map written to " << mapPath.generic_string() << std::endl;
			}
			else
			{
				std::cerr << "failed to write world map to " << mapPath.generic_string() << std::endl;
			}
		}

		std::cout << "Streaming generated world from " << _stagedWorld->loadedChunkCount()
				  << " initial chunks" << std::endl;
	}

	void GameSceneWidget::_configureOverlay()
	{
		_overlay.configureRows(RowCount + _profilerSectionRowCount(), 2);
		_overlay.setMaxGlyphSize(16);
		_overlay.setFontOutlineSize(1);
		_overlay.setText(CameraPosition, 0, "Camera position");
		_overlay.setText(PlayerCell, 0, "Player cell");
		_overlay.setText(HoveredCell, 0, "Hovered cell");
		_overlay.setText(LoadedChunks, 0, "Loaded chunks");
		_overlay.setText(UpdateTime, 0, "Update duration");
		_overlay.setText(RenderTime, 0, "Render duration");
		_overlay.setText(DeltaTime, 0, "Frame delta");
		_overlay.setText(TimeOfDay, 0, "Time of day");
		_overlay.setText(SelectedLights, 0, "Selected lights (D/P/S)");

		if (_profilerRowNames.empty() == true)
		{
			return;
		}

		// Header row: column 0 stays blank, the value columns are titled.
		_overlay.setRowColumns(RowCount, ProfilerColumnCount);
		_overlay.setText(RowCount, ProbeMax, "Max");
		_overlay.setText(RowCount, ProbeAvg, "Average");
		_overlay.setText(RowCount, ProbeMin, "Min");
		for (std::size_t index = 0; index < _profilerRowNames.size(); ++index)
		{
			const std::size_t row = RowCount + 1 + index;
			_overlay.setRowColumns(row, ProfilerColumnCount);
			_overlay.setText(row, ProbeName, _profilerRowNames[index]);
		}
	}

	std::size_t GameSceneWidget::_profilerSectionRowCount() const
	{
		// One header row plus one row per probe, or nothing while no probe exists yet.
		return _profilerRowNames.empty() ? 0 : (1 + _profilerRowNames.size());
	}

	void GameSceneWidget::_applyOverlayGeometry()
	{
		const std::size_t totalRows = RowCount + _profilerSectionRowCount();
		// Wider when the profiler table is present, since it carries three extra value columns.
		const std::uint32_t targetWidth = _profilerRowNames.empty() ? 360u : 560u;
		const std::uint32_t overlayWidth = std::min<std::uint32_t>(geometry().size.x, targetWidth);
		_overlay.setGeometry(spk::Rect2D({8, 8}, {overlayWidth, 26u * static_cast<std::uint32_t>(totalRows)}));
	}

	void GameSceneWidget::_syncProfilerRows(const std::vector<std::string> &p_names)
	{
		if (p_names == _profilerRowNames)
		{
			return;
		}
		_profilerRowNames = p_names;
		// configureRows() rebuilds every label, so the fixed rows are re-applied too.
		_configureOverlay();
		_applyOverlayGeometry();
	}

	void GameSceneWidget::_onGeometryChange()
	{
		spk::GameEngineWidget::_onGeometryChange();
		if (_camera != nullptr)
		{
			_camera->setViewportSize(static_cast<float>(geometry().width()), static_cast<float>(geometry().height()));
		}
		_applyOverlayGeometry();
	}

	void GameSceneWidget::_onUpdate(const spk::UpdateContext &p_tick)
	{
		const long long start = nowNs();
		if (_player != nullptr)
		{
			const spk::Vector3Int focus = spk::VoxelChunk::coordinatesFromWorldCell(_player->cell);
			if (_streamer != nullptr)
			{
				_streamer->setOriginPosition(focus);
			}
			if (_fluidSimulator != nullptr)
			{
				_fluidSimulator->setSimulationCenter(_player->cell);
			}
			if (_terrainProvider != nullptr && (!_streamingFocus.has_value() || focus != *_streamingFocus))
			{
				_streamingFocus = focus;
				const int margin = 48;
				_context.world.navigation->resetBounds({{_player->cell.x - margin, 0, _player->cell.z - margin}, {_player->cell.x + margin + 1, 64, _player->cell.z + margin + 1}});
			}
		}
		spk::GameEngineWidget::_onUpdate(p_tick);
		_syncVoxelLights();
		if (_pendingTeleport.has_value())
		{
			const spk::Vector3Int target = *_pendingTeleport;
			_pendingTeleport.reset();
			_executeTeleport(target);
		}
		_updateDurationNs.store(nowNs() - start, std::memory_order_relaxed);
		invalidateRenderUnit();
		_refreshOverlay(p_tick);
	}

	void GameSceneWidget::_syncVoxelLights()
	{
		if (_context.world.world == nullptr || _context.world.world->revision() == _voxelLightRevision)
		{
			return;
		}
		_voxelLightRevision = _context.world.world->revision();
		std::unordered_set<spk::Vector3Int> required;
		spk::GameEngine &engine = gameEngine();
		_context.world.world->map().forEachLoadedChunk([&](const spk::VoxelChunk &chunk) {
			const spk::VoxelGrid &grid = chunk.grid();
			for (int y = 0; y < grid.size().y; ++y)
			{
				for (int z = 0; z < grid.size().z; ++z)
				{
					for (int x = 0; x < grid.size().x; ++x)
					{
						const spk::VoxelCell &cell = grid.cell(x, y, z);
						if (cell.isEmpty())
						{
							continue;
						}
						const VoxelDefinition *definition = _context.world.world->registry().tryDefinition(cell.id);
						if (definition == nullptr || !definition->light.has_value())
						{
							continue;
						}
						const spk::Vector3Int worldCell = chunk.worldOrigin() + spk::Vector3Int{x, y, z};
						required.insert(worldCell);
						if (_voxelLightEntities.contains(worldCell))
						{
							continue;
						}
						auto entity = std::make_unique<spk::Entity3D>();
						entity->transform().setPosition(spk::Vector3(worldCell) + spk::Vector3{0.5f, 0.5f, 0.5f});
						auto &light = entity->addComponent<spk::Light3D>();
						const VoxelLightDefinition &authored = *definition->light;
						switch (authored.type)
						{
						case VoxelLightType::Directional:
							(void)light.directional();
							break;
						case VoxelLightType::Point:
							(void)light.point();
							light.point().range = authored.reach;
							break;
						case VoxelLightType::Spot:
							(void)light.spot();
							light.spot().range = authored.reach;
							light.spot().innerHalfAngleDegrees = authored.innerHalfAngleDegrees;
							light.spot().outerHalfAngleDegrees = authored.outerHalfAngleDegrees;
							break;
						}
						light.color() = authored.color;
						light.intensity() = authored.power;
						light.selectionPriority() = 100;
						engine.addEntity(entity.get());
						_voxelLightEntities.emplace(worldCell, std::move(entity));
					}
				}
			}
		});
		for (auto iterator = _voxelLightEntities.begin(); iterator != _voxelLightEntities.end();)
		{
			if (required.contains(iterator->first))
			{
				++iterator;
				continue;
			}
			engine.removeEntity(iterator->second.get());
			iterator = _voxelLightEntities.erase(iterator);
		}
	}

	void GameSceneWidget::_executeTeleport(const spk::Vector3Int &p_target)
	{
		if (_player == nullptr || _context.world.world == nullptr)
		{
			return;
		}
		// Warm the destination neighbourhood so walk heights and the navigation graph
		// have terrain the instant the player lands (mirrors the spawn warm-up).
		const spk::Vector3Int targetChunk = spk::VoxelChunk::coordinatesFromWorldCell(p_target);
		for (int y = -StreamViewRange.y; y <= StreamViewRange.y; ++y)
		{
			for (int z = -StreamViewRange.z; z <= StreamViewRange.z; ++z)
			{
				for (int x = -StreamViewRange.x; x <= StreamViewRange.x; ++x)
				{
					_context.world.world->loadChunk(targetChunk + spk::Vector3Int{x, y, z});
				}
			}
		}

		const spk::Vector3 cameraDelta = spk::Vector3(p_target - _player->cell);
		_player->cell = p_target;
		_player->path.clear();
		_player->segment = 0;
		_player->segmentProgress = 0.0f;
		if (_pathLogic != nullptr)
		{
			_pathLogic->placeAtCell(*_player);
		}
		if (_streamer != nullptr)
		{
			_streamer->setOriginPosition(targetChunk);
		}
		// Forces the streaming-focus branch of the next update to re-center the
		// navigation bounds around the arrival cell.
		_streamingFocus.reset();
		if (_cameraLogic != nullptr)
		{
			_cameraLogic->teleportBy(cameraDelta);
		}
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
		return unit;
	}

	void GameSceneWidget::_refreshOverlay(const spk::UpdateContext &p_tick)
	{
		if (_camera != nullptr)
		{
			_overlay.setText(CameraPosition, 1, formatVector(_camera->position()));
		}
		if (_player != nullptr)
		{
			_overlay.setText(PlayerCell, 1, formatVector(_player->cell));
		}
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
		_overlay.setText(UpdateTime, 1, formatFloat(static_cast<float>(_updateDurationNs.load()) / 1.0e6f, " ms"));
		_overlay.setText(RenderTime, 1, formatFloat(static_cast<float>(_renderDurationNs.load()) / 1.0e6f, " ms"));
		_overlay.setText(DeltaTime, 1, std::to_string(p_tick.deltaTime.milliseconds()) + " ms");
		_overlay.setText(TimeOfDay, 1, _dayTimeLogic == nullptr ? "-" : formatFloat(_dayTimeLogic->timeOfDayHours(), " h"));
		if (_lighting != nullptr)
		{
			const spk::SceneLightingDiagnostics &diagnostics = _lighting->diagnostics();
			_overlay.setText(SelectedLights, 1, std::to_string(diagnostics.selectedDirectional) + " / " + std::to_string(diagnostics.selectedPoint) + " / " + std::to_string(diagnostics.selectedSpot));
		}

		if (p_tick.profiler != nullptr)
		{
			const std::vector<spk::Profiler::Snapshot> snapshots = p_tick.profiler->snapshot();
			std::vector<std::string> names;
			names.reserve(snapshots.size());
			for (const spk::Profiler::Snapshot &snapshot : snapshots)
			{
				names.push_back(snapshot.name);
			}
			_syncProfilerRows(names);
			for (std::size_t index = 0; index < snapshots.size(); ++index)
			{
				const spk::Profiler::Snapshot &snapshot = snapshots[index];
				const std::size_t row = RowCount + 1 + index;
				_overlay.setText(row, ProbeMax, formatProbeValue(snapshot.maximum));
				_overlay.setText(row, ProbeAvg, formatProbeValue(snapshot.average));
				_overlay.setText(row, ProbeMin, formatProbeValue(snapshot.minimum));
			}
		}
	}
}
