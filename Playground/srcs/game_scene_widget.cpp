#include "game_scene_widget.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <string>

#include "components/mesh_renderer3d.hpp"
#include "geometry/primitive3d.hpp"
#include "logics/mesh_render_logic.hpp"
#include "structures/math/spk_quaternion.hpp"
#include "structures/math/spk_vector3.hpp"

#ifndef PG_RESOURCE_DIR
#	define PG_RESOURCE_DIR "."
#endif

namespace
{
	constexpr std::size_t StressGridColumns = 40;
	constexpr float StressCubeSpacing = 2.5f;

	enum OverlayRow : std::size_t
	{
		CameraPosition = 0,
		CubeYaw,
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
	GameSceneWidget::GameSceneWidget(const std::string &p_name, spk::Widget *p_parent, GameContext &p_context) :
		spk::GameEngineWidget(p_name, p_parent),
		_modeManager(p_context),
		_overlay(p_name + "/DebugOverlay", this)
	{
		_buildScene();
		_configureOverlay();
		_modeManager.enterExploration();
		activate();
	}

	void GameSceneWidget::_spawnStressCubes(const spk::Vector3 &p_center)
	{
		static constexpr size_t StressCubeCount = 1000;

		static_assert(StressCubeCount % StressGridColumns == 0);

		constexpr std::size_t rows = StressCubeCount / StressGridColumns;

		const float halfColumns = (static_cast<float>(StressGridColumns) - 1.0f) * 0.5f;
		const float halfRows = (static_cast<float>(rows) - 1.0f) * 0.5f;

		spk::GameEngine &engine = gameEngine();

		_cubes.resize(StressCubeCount);

		for (std::size_t i = 0; i < _cubes.size(); ++i)
		{
			const std::size_t column = i % StressGridColumns;
			const std::size_t row = i / StressGridColumns;

			const float x = (static_cast<float>(column) - halfColumns) * StressCubeSpacing;
			const float z = (static_cast<float>(row) - halfRows) * StressCubeSpacing;

			pg::Entity3D *cube = new pg::Entity3D();

			pg::MeshRenderer3D &renderer = cube->addComponent<pg::MeshRenderer3D>();
			renderer.setMesh(&_cubeMesh);
			renderer.setTexture(&_texture);

			cube->transform().setPosition({
				p_center.x + x,
				p_center.y,
				p_center.z + z
			});

			engine.addEntity(cube);

			_cubes[i] = cube;
		}
	}

	void GameSceneWidget::_buildScene()
	{
		const std::filesystem::path texturePath =
			std::filesystem::path(PG_RESOURCE_DIR) / "textures" / "spriteSheet.png";
		_texture.loadFromFile(texturePath, {4u, 3u});

		_cubeMesh = pg::makeCube(1.0f);

		spk::GameEngine &engine = gameEngine();
		engine.add<pg::MeshRenderLogic>();

		_camera = &_cameraEntity.addComponent<pg::Camera3D>();
		_camera->setPerspective(60.0f, 0.1f, 1000.0f);
		_camera->setPosition({0.0f, 5.0f, -10.0f});
		_camera->setUp({0.0f, 1.0f, 0.0f});
		_camera->setTarget({0.0f, 0.0f, 0.0f});
		_camera->makeMain();
		engine.addEntity(&_cameraEntity);

		_spawnStressCubes({0.0f, 0.0f, 0.0f});
	}

	void GameSceneWidget::_configureOverlay()
	{
		_overlay.configureRows(RowCount, 2);
		_overlay.setMaxGlyphSize(16);
		_overlay.setFontOutlineSize(1);

		_overlay.setText(CameraPosition, 0, "Camera position");
		_overlay.setText(CubeYaw, 0, "Cube yaw");
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

		const float deltaSeconds = static_cast<float>(p_tick.deltaTime.milliseconds()) / 1000.0f;
		_cubeYaw += 60.0f * deltaSeconds;
		if (_cubeYaw >= 360.0f)
		{
			_cubeYaw -= 360.0f;
		}
		const spk::Quaternion rotation = spk::Quaternion::fromEuler({25.0f, _cubeYaw, 0.0f});

		for (pg::Entity3D *cube : _cubes)
		{
			cube->transform().setRotation(rotation);
		}

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
		_overlay.setText(CubeYaw, 1, formatFloat(_cubeYaw, " deg"));
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
