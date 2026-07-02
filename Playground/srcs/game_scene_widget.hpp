#pragma once

#include <atomic>
#include <cstddef>
#include <string>

#include "components/camera3d.hpp"
#include "components/entity3d.hpp"
#include "core/mode_manager.hpp"
#include "geometry/mesh3d.hpp"
#include "voxel/voxel_grid.hpp"
#include "structures/graphics/texture/spk_sprite_sheet.hpp"
#include "structures/widget/spk_debug_overlay.hpp"
#include "structures/widget/spk_game_engine_widget.hpp"

namespace pg
{
	class Registries;
	class VoxelRegistry;

	// Step 5 scene: a hand-stamped voxel showcase proving meshing, neighbour culling,
	// orientation/flip transforms, atlas UVs, and the existing lit depth-tested renderer.
	class GameSceneWidget : public spk::GameEngineWidget
	{
	private:
		ModeManager _modeManager;

		spk::SpriteSheet _texture;
		VoxelGrid _showcaseGrid;
		Mesh3D _showcaseMesh;
		pg::Entity3D _showcaseEntity;

		pg::Entity3D _cameraEntity;
		pg::Camera3D *_camera = nullptr;

		spk::DebugOverlay _overlay;

		float _meshingDurationMs = 0.0f;

		mutable std::atomic<long long> _renderDurationNs{0};
		std::atomic<long long> _updateDurationNs{0};
		mutable std::atomic<std::size_t> _meshCount{0};
		mutable std::atomic<std::size_t> _triangleCount{0};

		void _buildScene(const VoxelRegistry &p_voxels);
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
	};
}
