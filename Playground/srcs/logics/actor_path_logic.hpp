#pragma once

#include "board/cell_source.hpp"
#include "components/actor.hpp"
#include "core/event_center.hpp"
#include "structures/game_engine/spk_component_logic.hpp"

namespace pg
{
	class VoxelWorld;
	class WorldNavigation;

	class ActorPathLogic : public spk::ComponentLogic<Actor>
	{
	private:
		EventCenter &_events;
		WorldNavigation &_navigation;
		VoxelWorld &_world;
		spk::ContractProvider<Actor *, spk::Vector3Int>::Contract _moveContract;

		[[nodiscard]] float _heightAtCenter(const spk::Vector3Int &p_cell) const;
		[[nodiscard]] float _heightAtEdge(
			const spk::Vector3Int &p_cell,
			VoxelOrientation p_direction) const;
		[[nodiscard]] spk::Vector3 _segmentPosition(
			const spk::Vector3Int &p_from,
			const spk::Vector3Int &p_to,
			float p_progress) const;
		void _applyPosition(Actor &p_actor, const spk::Vector3 &p_footPosition) const;

	public:
		ActorPathLogic(EventCenter &p_events, WorldNavigation &p_navigation, VoxelWorld &p_world);
		void requestMove(Actor &p_actor, const spk::Vector3Int &p_target);
		void placeAtCell(Actor &p_actor);
		void advance(Actor &p_actor, float p_seconds);

	protected:
		void _parseComponentForUpdate(const spk::UpdateTick &p_tick, Actor &p_actor) override;
	};
}
