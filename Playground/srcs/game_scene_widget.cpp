#include "game_scene_widget.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <string>

#include "components/mesh_renderer3d.hpp"
#include "core/registries.hpp"
#include "logics/chunk_render_logic.hpp"
#include "logics/chunk_synchronization_logic.hpp"
#include "structures/math/spk_vector3.hpp"
#include "world/voxel_world.hpp"

#ifndef PG_RESOURCE_DIR
#	define PG_RESOURCE_DIR "."
#endif

namespace
{
	enum OverlayRow : std::size_t
	{
		CameraPosition = 0,
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
		_overlay(p_name + "/DebugOverlay", this)
	{
		_buildScene(p_registries);
		_configureOverlay();
		_modeManager.enterExploration();
		activate();
	}

	GameSceneWidget::~GameSceneWidget()
	{
		_context.world.world.reset();
		_context.world.activeMap = nullptr;
	}

	void GameSceneWidget::_buildScene(const Registries &p_registries)
	{
		const std::filesystem::path texturePath =
			std::filesystem::path(PG_RESOURCE_DIR) / "textures" / "voxels.png";
		_texture.loadFromFile(texturePath, {8u, 8u});
		_maskTexture.loadFromFile(
			std::filesystem::path(PG_RESOURCE_DIR) / "textures" / "mask.png", {4u, 4u});

		spk::GameEngine &engine = gameEngine();
		engine.add<pg::ChunkSynchronizationLogic>();
		engine.add<pg::ChunkRenderLogic>(_texture, &_maskTexture);

		_camera = &_cameraEntity.addComponent<pg::Camera3D>();
		_camera->setPerspective(60.0f, 0.1f, 1000.0f);
		_camera->setPosition({78.0f, 58.0f, -58.0f});
		_camera->setUp({0.0f, 1.0f, 0.0f});
		_camera->setTarget({32.0f, 2.0f, 32.0f});
		_camera->makeMain();
		engine.addEntity(&_cameraEntity);

		_context.world.activeMap = &p_registries.maps().get("m1-testground");
		_context.world.world = std::make_unique<VoxelWorld>(p_registries.voxels(), &engine);
		_context.world.world->loadFromMap(*_context.world.activeMap);
		std::cout << "Loaded map 'm1-testground' as "
				  << _context.world.world->loadedChunkCount() << " chunks" << std::endl;
	}

	void GameSceneWidget::_configureOverlay()
	{
		_overlay.configureRows(RowCount, 2);
		_overlay.setMaxGlyphSize(16);
		_overlay.setFontOutlineSize(1);

		_overlay.setText(CameraPosition, 0, "Camera position");
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
			_camera->setViewportSize(
				static_cast<float>(geometry().width()),
				static_cast<float>(geometry().height()));
		}

		const std::uint32_t overlayWidth = std::min<std::uint32_t>(geometry().size.x, 360u);
		_overlay.setGeometry(spk::Rect2D({8, 8}, {overlayWidth, 26u * RowCount}));
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
