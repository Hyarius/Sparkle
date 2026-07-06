#include "logics/trainer_sight_logic.hpp"

#include "board/pathfinder.hpp"
#include "components/actor.hpp"
#include "core/event_center.hpp"
#include "core/game_context.hpp"
#include "encounters/encounter_table.hpp"
#include "encounters/encounter_types.hpp"
#include "world/trainer.hpp"
#include "world/voxel_world.hpp"
#include "world/world_navigation.hpp"
#include "world/world_raycaster.hpp"

#include <cmath>
#include <algorithm>
#include <utility>

namespace
{
	[[nodiscard]] std::pair<int, int> forwardAndLateral(
		const spk::Vector3Int &p_delta, pg::VoxelOrientation p_facing)
	{
		switch (p_facing)
		{
		case pg::VoxelOrientation::PositiveX: return {p_delta.x, std::abs(p_delta.z)};
		case pg::VoxelOrientation::NegativeX: return {-p_delta.x, std::abs(p_delta.z)};
		case pg::VoxelOrientation::PositiveZ: return {p_delta.z, std::abs(p_delta.x)};
		case pg::VoxelOrientation::NegativeZ: return {-p_delta.z, std::abs(p_delta.x)};
		}
		return {0, 0};
	}
}

namespace pg
{
	TrainerSightLogic::TrainerSightLogic(
		GameContext &p_context,
		const VoxelWorld &p_world,
		WorldNavigation &p_navigation,
		spk::Vector2Int p_defaultBoardSize,
		ApproachFunction p_approach) :
		_context(p_context),
		_world(p_world),
		_navigation(p_navigation),
		_defaultBoardSize(p_defaultBoardSize),
		_approach(std::move(p_approach)),
		_moveContract(_context.events.playerMoved.subscribe([this](spk::Vector3Int p_cell) {
			_onPlayerMoved(p_cell);
		})),
		_resolvedContract(_context.events.battleResolved.subscribe(
			[this](BattleContext *, BattleSide p_winner) {
				if (_activeTrainer != nullptr)
				{
					if (p_winner == BattleSide::Player)
					{
						_context.clearedTrainers.insert(_activeTrainer->clearedFlag);
					}
					_activeTrainer->encounterPending = false;
					_activeTrainer = nullptr;
				}
			}))
	{
	}

	void TrainerSightLogic::addTrainer(Trainer &p_trainer)
	{
		_trainers.push_back(&p_trainer);
	}

	void TrainerSightLogic::clearTrainers() noexcept
	{
		_trainers.clear();
		_activeTrainer = nullptr;
	}

	bool TrainerSightLogic::canSee(
		const VoxelWorld &p_world,
		const spk::Vector3Int &p_trainerCell,
		VoxelOrientation p_facing,
		int p_sightRange,
		const spk::Vector3Int &p_playerCell)
	{
		if (p_sightRange <= 0)
		{
			return false;
		}
		const spk::Vector3Int delta = p_playerCell - p_trainerCell;
		const auto [forward, lateral] = forwardAndLateral(delta, p_facing);
		// Trainer sight is deliberately a strict cardinal line, not a widening cone.
		if (forward <= 0 || forward > p_sightRange || lateral != 0)
		{
			return false;
		}

		const spk::Vector3 origin{
			static_cast<float>(p_trainerCell.x) + 0.5f,
			static_cast<float>(p_trainerCell.y) + 1.5f,
			static_cast<float>(p_trainerCell.z) + 0.5f};
		const spk::Vector3 target{
			static_cast<float>(p_playerCell.x) + 0.5f,
			static_cast<float>(p_playerCell.y) + 1.5f,
			static_cast<float>(p_playerCell.z) + 0.5f};
		const spk::Vector3 direction = target - origin;
		const float distance = origin.distance(target);
		return !WorldRaycaster::raycast(
			p_world, WorldRay{.origin = origin, .direction = direction}, std::max(0.0f, distance - 0.01f))
			.has_value();
	}

	void TrainerSightLogic::_onPlayerMoved(const spk::Vector3Int &p_playerCell)
	{
		if (!_context.world.explorationActive || _activeTrainer != nullptr)
		{
			return;
		}
		for (Trainer *trainer : _trainers)
		{
			if (trainer == nullptr || trainer->actor == nullptr || trainer->encounterTable == nullptr ||
				trainer->encounterPending || _context.clearedTrainers.contains(trainer->clearedFlag) ||
				!canSee(_world, trainer->actor->cell, trainer->facing, trainer->sightRange, p_playerCell))
			{
				continue;
			}

			const EncounterTier *tier = trainer->encounterTable->tierForBadges(
				static_cast<int>(_context.clearedGyms.size()));
			if (tier == nullptr || tier->weightedTeams.empty())
			{
				continue;
			}
			const WeightedEncounterTeam &team = tier->weightedTeams.front();
			if (const auto path = Pathfinder::findPath(
					_navigation.graph(), trainer->actor->cell, p_playerCell);
				path.has_value() && path->size() >= 2)
			{
				const spk::Vector3Int adjacent = (*path)[path->size() - 2];
				if (_approach)
				{
					_approach(*trainer->actor, adjacent);
				}
				else
				{
					trainer->actor->cell = adjacent;
				}
			}

			trainer->encounterPending = true;
			_activeTrainer = trainer;
			EncounterSpawn spawn{
				.displayName = team.displayName,
				.enemyTeam = team.team,
				.allowsTaming = false,
				.boardSize = team.boardSize.value_or(
					trainer->boardSize.x > 0 && trainer->boardSize.y > 0
						? trainer->boardSize
						: _defaultBoardSize),
				.placementStyle = PlacementStyle::ByLine};
			_context.events.encounterTriggered.trigger(spawn);
			return;
		}
	}
}
