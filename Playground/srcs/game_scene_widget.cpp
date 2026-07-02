#include "game_scene_widget.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <string>

#include "components/mesh_renderer3d.hpp"
#include "core/registries.hpp"
#include "logics/mesh_render_logic.hpp"
#include "structures/math/spk_vector3.hpp"
#include "voxel/voxel_mesher.hpp"
#include "world/showcase.hpp"

#ifndef PG_RESOURCE_DIR
#	define PG_RESOURCE_DIR "."
#endif

namespace
{
	enum OverlayRow : std::size_t
	{
		CameraPosition = 0,
		MeshingTime,
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
		_overlay(p_name + "/DebugOverlay", this)
	{
		_buildScene(p_registries.voxels());
		_configureOverlay();
		_modeManager.enterExploration();
		activate();
	}

	void GameSceneWidget::_buildScene(const VoxelRegistry &p_voxels)
	{
		const std::filesystem::path texturePath =
			std::filesystem::path(PG_RESOURCE_DIR) / "textures" / "voxels.png";
		_texture.loadFromFile(texturePath, {8u, 8u});

		_showcaseGrid = buildShowcaseGrid(p_voxels);
		const auto meshingStart = std::chrono::steady_clock::now();
		_showcaseMesh = VoxelMesher::buildRenderMesh(_showcaseGrid, p_voxels);
		_meshingDurationMs = std::chrono::duration<float, std::milli>(
			std::chrono::steady_clock::now() - meshingStart)
			.count();
		std::printf(
			"Showcase voxel mesh: %zu vertices, %zu triangles in %.3f ms\n",
			_showcaseMesh.vertices().size(),
			_showcaseMesh.indexes().size() / 3,
			_meshingDurationMs);

		spk::GameEngine &engine = gameEngine();
		engine.add<pg::MeshRenderLogic>();

		_camera = &_cameraEntity.addComponent<pg::Camera3D>();
		_camera->setPerspective(60.0f, 0.1f, 1000.0f);
		_camera->setPosition({15.0f, 10.0f, -10.0f});
		_camera->setUp({0.0f, 1.0f, 0.0f});
		_camera->setTarget({6.0f, 1.25f, 6.0f});
		_camera->makeMain();
		engine.addEntity(&_cameraEntity);

		pg::MeshRenderer3D &renderer = _showcaseEntity.addComponent<pg::MeshRenderer3D>();
		renderer.setMesh(&_showcaseMesh);
		renderer.setTexture(&_texture);
		renderer.setTint({1.0f, 1.0f, 1.0f, 1.0f});
		renderer.setTranslucent(false);
		engine.addEntity(&_showcaseEntity);
	}

	void GameSceneWidget::_configureOverlay()
	{
		_overlay.configureRows(RowCount, 2);
		_overlay.setMaxGlyphSize(16);
		_overlay.setFontOutlineSize(1);

		_overlay.setText(CameraPosition, 0, "Camera position");
		_overlay.setText(MeshingTime, 0, "Voxel meshing");
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

		_meshCount.store(pg::MeshRenderLogic::lastMeshCount(), std::memory_order_relaxed);
		_triangleCount.store(pg::MeshRenderLogic::lastTriangleCount(), std::memory_order_relaxed);
		return unit;
	}

	void GameSceneWidget::_refreshOverlay(const spk::UpdateTick &p_tick)
	{
		if (_camera != nullptr)
		{
			_overlay.setText(CameraPosition, 1, formatVector(_camera->position()));
		}
		_overlay.setText(MeshingTime, 1, formatFloat(_meshingDurationMs, " ms"));
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
