#pragma once

#include "components/actor.hpp"
#include "core/game_context.hpp"
#include "structures/game_engine/spk_camera_3d.hpp"
#include "structures/game_engine/spk_component_logic.hpp"
#include "structures/game_engine/spk_texture_mesh_renderer_3d.hpp"
#include "structures/voxel/spk_voxel_shape.hpp"

#include <functional>
#include <optional>

namespace pg
{
	class VoxelWorld;
	class WorldNavigation;

	class ExplorationInputLogic : public spk::ComponentLogic<Actor>
	{
	public:
		using ViewportSize = std::function<spk::Vector2()>;
	private:
		GameContext &_context;
		VoxelWorld &_world;
		WorldNavigation &_navigation;
		spk::Camera3D &_camera;
		spk::TextureMeshRenderer3D &_hoverRenderer;
		ViewportSize _viewportSize;
		spk::AtlasCell _hoveredMask;
		spk::AtlasCell _invalidMask;
		std::optional<spk::Vector3Int> _hovered;
		float _invalidSeconds = 0;
		spk::ContractProvider<spk::Vector3Int>::Contract _invalidContract;

		void _pick(const spk::Vector2Int &p_mouse);
		void _rebuildHover();

	public:
		ExplorationInputLogic(
			GameContext &p_context,
			VoxelWorld &p_world,
			WorldNavigation &p_navigation,
			spk::Camera3D &p_camera,
			spk::TextureMeshRenderer3D &p_hoverRenderer,
			ViewportSize p_viewportSize,
			spk::AtlasCell p_hoveredMask,
			spk::AtlasCell p_invalidMask);
		[[nodiscard]] const std::optional<spk::Vector3Int> &hoveredCell() const noexcept;

	protected:
		void _parseComponentForUpdate(const spk::UpdateContext &p_tick, Actor &p_actor) override;
		void _parseComponentForMouseMovedEvent(spk::MouseMovedEvent &p_event, Actor &p_actor) override;
		void _parseComponentForMouseButtonPressedEvent(spk::MouseButtonPressedEvent &p_event, Actor &p_actor) override;
	};
}
