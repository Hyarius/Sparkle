#pragma once

#include "components/actor.hpp"
#include "components/mesh_renderer3d.hpp"
#include "core/game_context.hpp"
#include "structures/game_engine/spk_component_logic.hpp"
#include "voxel/voxel_shape.hpp"

#include <functional>
#include <optional>

namespace pg
{
	class Camera3D;
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
		Camera3D &_camera;
		MeshRenderer3D &_hoverRenderer;
		ViewportSize _viewportSize;
		AtlasCell _hoveredMask;
		AtlasCell _invalidMask;
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
			Camera3D &p_camera,
			MeshRenderer3D &p_hoverRenderer,
			ViewportSize p_viewportSize,
			AtlasCell p_hoveredMask,
			AtlasCell p_invalidMask);
		[[nodiscard]] const std::optional<spk::Vector3Int> &hoveredCell() const noexcept;

	protected:
		void _parseComponentForUpdate(const spk::UpdateTick &p_tick, Actor &p_actor) override;
		void _parseComponentForMouseMovedEvent(spk::MouseMovedEvent &p_event, Actor &p_actor) override;
		void _parseComponentForMouseButtonPressedEvent(spk::MouseButtonPressedEvent &p_event, Actor &p_actor) override;
	};
}
