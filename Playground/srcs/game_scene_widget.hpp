#pragma once

#include <atomic>
#include <cstddef>
#include <string>

#include "components/camera3d.hpp"
#include "components/entity3d.hpp"
#include "core/mode_manager.hpp"
#include "structures/graphics/geometry/spk_texture_mesh_2d.hpp"
#include "structures/graphics/texture/spk_sprite_sheet.hpp"
#include "structures/widget/spk_debug_overlay.hpp"
#include "structures/widget/spk_game_engine_widget.hpp"

namespace pg
{
	// Step 1 scene: a single textured cube rendered in perspective 3D through a Camera3D,
	// rotating each frame, with a DebugOverlay. Proves the 3D render path (Transform3D /
	// Camera3D / MeshRenderer3D + MeshRenderLogic + MeshRenderCommand + mesh shader).
	class GameSceneWidget : public spk::GameEngineWidget
	{
	private:
		ModeManager _modeManager;

		spk::SpriteSheet _texture;
		spk::TextureMesh2D _cubeMesh;

		pg::Entity3D _cameraEntity;
		std::vector<pg::Entity3D *> _cubes;
		pg::Camera3D *_camera = nullptr;

		spk::DebugOverlay _overlay;

		float _cubeYaw = 0.0f;

		mutable std::atomic<long long> _renderDurationNs{0};
		std::atomic<long long> _updateDurationNs{0};
		mutable std::atomic<std::size_t> _meshCount{0};
		mutable std::atomic<std::size_t> _triangleCount{0};

		void _buildScene();
		void _spawnStressCubes(const spk::Vector3 &p_center);
		void _configureOverlay();
		void _refreshOverlay(const spk::UpdateTick &p_tick);

	protected:
		void _onGeometryChange() override;
		void _onUpdate(const spk::UpdateTick &p_tick) override;
		[[nodiscard]] spk::RenderUnit _buildRenderUnit() const override;

	public:
		GameSceneWidget(const std::string &p_name, spk::Widget *p_parent, GameContext &p_context);
	};
}
