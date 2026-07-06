#pragma once

#include "battle/battle_side.hpp"
#include "structures/design_pattern/spk_contract_provider.hpp"
#include "structures/math/spk_vector2.hpp"
#include "structures/math/spk_vector3.hpp"
#include "voxel/voxel_enums.hpp"

#include <functional>
#include <vector>

namespace pg
{
	class Actor;
	class BattleContext;
	struct GameContext;
	class Trainer;
	class VoxelWorld;
	class WorldNavigation;

	class TrainerSightLogic
	{
	public:
		using ApproachFunction = std::function<void(Actor &, const spk::Vector3Int &)>;

	private:
		GameContext &_context;
		const VoxelWorld &_world;
		WorldNavigation &_navigation;
		spk::Vector2Int _defaultBoardSize;
		ApproachFunction _approach;
		std::vector<Trainer *> _trainers;
		Trainer *_activeTrainer = nullptr;
		spk::ContractProvider<spk::Vector3Int>::Contract _moveContract;
		spk::ContractProvider<BattleContext *, BattleSide>::Contract _resolvedContract;

		void _onPlayerMoved(const spk::Vector3Int &p_playerCell);

	public:
		TrainerSightLogic(
			GameContext &p_context,
			const VoxelWorld &p_world,
			WorldNavigation &p_navigation,
			spk::Vector2Int p_defaultBoardSize,
			ApproachFunction p_approach = {});

		void addTrainer(Trainer &p_trainer);
		void clearTrainers() noexcept;

		[[nodiscard]] static bool canSee(
			const VoxelWorld &p_world,
			const spk::Vector3Int &p_trainerCell,
			VoxelOrientation p_facing,
			int p_sightRange,
			const spk::Vector3Int &p_playerCell);
	};
}
