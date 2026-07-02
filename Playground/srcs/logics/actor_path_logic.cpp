#include "logics/actor_path_logic.hpp"

#include "board/pathfinder.hpp"
#include "components/transform3d.hpp"
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
		if (p_to.x > p_from.x) return pg::VoxelOrientation::PositiveX;
		if (p_to.x < p_from.x) return pg::VoxelOrientation::NegativeX;
		if (p_to.z > p_from.z) return pg::VoxelOrientation::PositiveZ;
		return pg::VoxelOrientation::NegativeZ;
	}

	[[nodiscard]] pg::VoxelOrientation opposite(pg::VoxelOrientation p_direction)
	{
		switch (p_direction)
		{
		case pg::VoxelOrientation::PositiveX: return pg::VoxelOrientation::NegativeX;
		case pg::VoxelOrientation::NegativeX: return pg::VoxelOrientation::PositiveX;
		case pg::VoxelOrientation::PositiveZ: return pg::VoxelOrientation::NegativeZ;
		case pg::VoxelOrientation::NegativeZ: return pg::VoxelOrientation::PositiveZ;
		}
		return pg::VoxelOrientation::PositiveZ;
	}
}

namespace pg
{
	ActorPathLogic::ActorPathLogic(EventCenter &p_events, WorldNavigation &p_navigation, VoxelWorld &p_world) :
		_events(p_events),
		_navigation(p_navigation),
		_world(p_world),
		_moveContract(_events.actorMoveRequested.subscribe([this](Actor *p_actor, spk::Vector3Int p_target) {
			if (p_actor != nullptr) requestMove(*p_actor, p_target);
		}))
	{
	}

	float ActorPathLogic::_heightAtCenter(const spk::Vector3Int &p_cell) const
	{
		const VoxelCell &cell = _world.cell(p_cell);
		const VoxelDefinition *definition = _world.tryDefinition(cell);
		return static_cast<float>(p_cell.y) +
			   (definition == nullptr ? 0.0f : definition->shape->heights(cell.flip).stationary);
	}

	float ActorPathLogic::_heightAtEdge(
		const spk::Vector3Int &p_cell,
		VoxelOrientation p_direction) const
	{
		const VoxelCell &cell = _world.cell(p_cell);
		const VoxelDefinition *definition = _world.tryDefinition(cell);
		if (definition == nullptr) return static_cast<float>(p_cell.y);
		return static_cast<float>(p_cell.y) + resolveWorldHeights(
			definition->shape->heights(cell.flip), cell.orientation).get(p_direction);
	}

	spk::Vector3 ActorPathLogic::_segmentPosition(
		const spk::Vector3Int &p_from,
		const spk::Vector3Int &p_to,
		float p_progress) const
	{
		const float progress = std::clamp(p_progress, 0.0f, 1.0f);
		const VoxelOrientation direction = directionBetween(p_from, p_to);
		const float centerFrom = _heightAtCenter(p_from);
		const float centerTo = _heightAtCenter(p_to);
		const float boundary = 0.5f * (_heightAtEdge(p_from, direction) + _heightAtEdge(p_to, opposite(direction)));
		const float height = progress <= 0.5f
			? centerFrom + (boundary - centerFrom) * (progress * 2.0f)
			: boundary + (centerTo - boundary) * ((progress - 0.5f) * 2.0f);
		return {
			static_cast<float>(p_from.x) + 0.5f + static_cast<float>(p_to.x - p_from.x) * progress,
			height,
			static_cast<float>(p_from.z) + 0.5f + static_cast<float>(p_to.z - p_from.z) * progress};
	}

	void ActorPathLogic::_applyPosition(Actor &p_actor, const spk::Vector3 &p_footPosition) const
	{
		if (spk::Entity *owner = p_actor.entity(); owner != nullptr)
		{
			if (Transform3D *transform = owner->component<Transform3D>(); transform != nullptr)
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
		_applyPosition(p_actor, {
			static_cast<float>(p_actor.cell.x) + 0.5f,
			_heightAtCenter(p_actor.cell),
			static_cast<float>(p_actor.cell.z) + 0.5f});
	}

	void ActorPathLogic::advance(Actor &p_actor, float p_seconds)
	{
		float remaining = std::max(0.0f, p_seconds) * std::max(0.0f, p_actor.speed);
		while (remaining > 0 && p_actor.segment + 1 < p_actor.path.size())
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
				if (p_actor.player) _events.playerMoved.trigger(p_actor.cell);
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

	void ActorPathLogic::_parseComponentForUpdate(const spk::UpdateTick &p_tick, Actor &p_actor)
	{
		advance(p_actor, static_cast<float>(p_tick.deltaTime.seconds()));
	}
}
