#include "logics/actor_path_logic.hpp"

#include "board/pathfinder.hpp"
#include "structures/game_engine/spk_transform_3d.hpp"
#include "voxel/voxel_traversal.hpp"
#include "world/voxel_world.hpp"
#include "world/world_navigation.hpp"

#include <algorithm>
#include <cmath>

namespace
{
	[[nodiscard]] pg::VoxelOrientation directionBetween(
		const spk::Vector3Int &p_from,
		const spk::Vector3Int &p_to)
	{
		if (p_to.x > p_from.x)
		{
			return pg::VoxelOrientation::PositiveX;
		}
		if (p_to.x < p_from.x)
		{
			return pg::VoxelOrientation::NegativeX;
		}
		if (p_to.z > p_from.z)
		{
			return pg::VoxelOrientation::PositiveZ;
		}
		return pg::VoxelOrientation::NegativeZ;
	}

}

namespace pg
{
	ActorPathLogic::ActorPathLogic(
		EventCenter &p_events,
		WorldNavigation &p_navigation,
		VoxelWorld &p_world,
		std::function<bool()> p_enabled) :
		_events(p_events),
		_navigation(p_navigation),
		_world(p_world),
		_enabled(std::move(p_enabled)),
		_moveContract(_events.actorMoveRequested.subscribe([this](Actor *p_actor, spk::Vector3Int p_target) {
			if (p_actor != nullptr)
			{
				requestMove(*p_actor, p_target);
			}
		}))
	{
	}

	float ActorPathLogic::_heightAtCenter(const spk::Vector3Int &p_cell) const
	{
		return walkHeightAtCenter(WorldCellSource(_world), p_cell);
	}

	spk::Vector3 ActorPathLogic::_segmentPosition(
		const spk::Vector3Int &p_from,
		const spk::Vector3Int &p_to,
		float p_progress) const
	{
		return interpolateWalkSegment(WorldCellSource(_world), p_from, p_to, p_progress);
	}

	void ActorPathLogic::_applyPosition(Actor &p_actor, const spk::Vector3 &p_footPosition) const
	{
		if (spk::Entity *owner = p_actor.entity(); owner != nullptr)
		{
			if (spk::Transform3D *transform = owner->component<spk::Transform3D>(); transform != nullptr)
			{
				transform->setPosition(p_footPosition + spk::Vector3{0, 0.35f, 0});
			}
		}
	}

	void ActorPathLogic::requestMove(Actor &p_actor, const spk::Vector3Int &p_target)
	{
		const auto path = Pathfinder::findPath(_navigation.graph(), p_actor.cell, p_target);
		if (!path.has_value())
		{
			p_actor.path.clear();
			p_actor.segment = 0;
			p_actor.segmentProgress = 0;
			_events.invalidTarget.trigger(p_target);
			return;
		}
		p_actor.path = *path;
		p_actor.segment = 0;
		p_actor.segmentProgress = 0;
	}

	void ActorPathLogic::placeAtCell(Actor &p_actor)
	{
		_applyPosition(p_actor, {static_cast<float>(p_actor.cell.x) + 0.5f, _heightAtCenter(p_actor.cell), static_cast<float>(p_actor.cell.z) + 0.5f});
	}

	void ActorPathLogic::advance(Actor &p_actor, float p_seconds)
	{
		float remaining = std::max(0.0f, p_seconds) * std::max(0.0f, p_actor.speed);
		while (remaining > 0 && _enabled() && p_actor.segment + 1 < p_actor.path.size())
		{
			const spk::Vector3Int from = p_actor.path[p_actor.segment];
			const spk::Vector3Int to = p_actor.path[p_actor.segment + 1];
			const float segmentLength = std::max(1.0f, _segmentPosition(from, to, 0).distance(_segmentPosition(from, to, 1)));
			const float available = (1.0f - p_actor.segmentProgress) * segmentLength;
			const float consumed = std::min(remaining, available);
			p_actor.segmentProgress += consumed / segmentLength;
			remaining -= consumed;
			p_actor.facing = directionBetween(from, to);
			_applyPosition(p_actor, _segmentPosition(from, to, p_actor.segmentProgress));
			if (p_actor.segmentProgress >= 1.0f - 0.0001f)
			{
				p_actor.cell = to;
				++p_actor.segment;
				p_actor.segmentProgress = 0;
				if (p_actor.player)
				{
					_events.playerMoved.trigger(p_actor.cell);
				}
			}
		}
		if (p_actor.segment + 1 >= p_actor.path.size())
		{
			p_actor.path.clear();
			p_actor.segment = 0;
			p_actor.segmentProgress = 0;
			placeAtCell(p_actor);
		}
	}

	void ActorPathLogic::_parseComponentForUpdate(const spk::UpdateContext &p_tick, Actor &p_actor)
	{
		if (_enabled())
		{
			advance(p_actor, static_cast<float>(p_tick.deltaTime.seconds()));
		}
	}
}
