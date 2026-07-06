#include "encounters/encounter_service.hpp"

#include "battle/battle_context.hpp"
#include "board/board_builder.hpp"
#include "core/game_context.hpp"
#include "core/registries.hpp"
#include "creatures/creature_species.hpp"
#include "creatures/apply_progress.hpp"
#include "creatures/creature_unit.hpp"
#include "feats/feat_progress.hpp"
#include "feats/uuid.hpp"
#include "world/voxel_world.hpp"

#include <algorithm>
#include <iostream>

namespace pg
{
	EncounterService::EncounterService(
		GameContext &p_context, const Registries &p_registries, PlayerCellProvider p_playerCell) :
		_context(p_context),
		_registries(p_registries),
		_playerCell(std::move(p_playerCell)),
		_encounterContract(_context.events.encounterTriggered.subscribe([this](const EncounterSpawn &p_spawn) {
			_onEncounter(p_spawn);
		}))
	{
		if (_context.player.empty())
		{
			_context.newGame(_registries);
		}
	}

	EncounterService::~EncounterService() = default;

	BattleContext *EncounterService::activeBattle() noexcept
	{
		return _battle.get();
	}

	void EncounterService::_onEncounter(const EncounterSpawn &p_spawn)
	{
		VoxelWorld *world = _context.world.world.get();
		if (world == nullptr)
		{
			return;
		}

		const std::size_t enemyCount = p_spawn.enemyTeam.size();
		const std::size_t playerCount = _context.player.teamSize();
		if (enemyCount == 0 || playerCount == 0)
		{
			std::cerr << "EncounterService: encounter needs at least one creature on each side" << std::endl;
			return;
		}
		const spk::Vector3Int center = _playerCell();

		try
		{
			BoardData board = BoardBuilder::fromWorld(
				*world,
				center,
				p_spawn.boardSize,
				std::max(1, _registries.gameRules().deploymentStripDepth),
				std::max(playerCount, enemyCount),
				VoxelOrientation::PositiveZ,
				static_cast<float>(_registries.gameRules().maxVerticalTraversalGap));

			// Replaces the previous (finished) battle. This runs during exploration — never inside a
			// battle tick — so destroying the old context here is safe.
			_battle.reset();
			_enemyCreatures.clear();
			_battle = std::make_unique<BattleContext>(_context.events, std::move(board));
			_battle->placementStyle = p_spawn.placementStyle;
			_battle->allowsTaming = p_spawn.allowsTaming;
			_battle->returnWorldCell = center;

			for (const std::unique_ptr<CreatureUnit> &creature : _context.player.team)
			{
				if (creature != nullptr)
				{
					(void)_battle->addUnit(creature.get(), BattleSide::Player);
				}
			}
			for (const EncounterTeamMember &member : p_spawn.enemyTeam)
			{
				const CreatureSpecies &species = _registries.creatures().get(member.speciesId);
				auto creature = std::make_unique<CreatureUnit>(species);
				for (const std::string &nodeId : member.completedNodeUuids)
				{
					const std::optional<spk::UUID> nodeUuid = uuidFromString(nodeId);
					if (!nodeUuid.has_value() || !completeNodeDirect(*creature, *nodeUuid))
					{
						throw std::runtime_error(
							"invalid or exhausted completed node '" + nodeId +
							"' for species '" + member.speciesId + "'");
					}
				}
				applyProgress(*creature);
				CreatureUnit *source = creature.get();
				_enemyCreatures.push_back(std::move(creature));
				BattleUnit &battleUnit = _battle->addUnit(source, BattleSide::Enemy);
				const std::string aiId = member.aiId.empty() ? "aggressive-melee" : member.aiId;
				battleUnit.aiBehaviour = &_registries.ai().get(aiId);
			}

			_context.events.battleStarted.trigger(_battle.get());
		} catch (const std::exception &exception)
		{
			std::cerr << "EncounterService: could not start battle: " << exception.what() << std::endl;
			_battle.reset();
			_enemyCreatures.clear();
		}
	}
}
