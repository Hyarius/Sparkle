#include "encounters/encounter_service.hpp"

#include "abilities/ability.hpp"
#include "battle/battle_context.hpp"
#include "board/board_builder.hpp"
#include "core/game_context.hpp"
#include "core/registries.hpp"
#include "world/voxel_world.hpp"

#include <algorithm>
#include <iostream>

namespace
{
	// Milestone-1 placeholder combatants (D26). Real creature units come from species JSON + the
	// player team in step 14; here they exist only to make the battle loop playable end to end.
	[[nodiscard]] pg::Attributes playerStandIn()
	{
		return {.health = 42, .ap = 6, .mp = 4, .attack = 5, .armor = 2, .magic = 1, .resistance = 1, .stamina = 3.0f, .staminaRate = 1.0f};
	}
	[[nodiscard]] pg::Attributes enemyStandIn()
	{
		return {.health = 22, .ap = 6, .mp = 3, .attack = 4, .armor = 1, .magic = 1, .resistance = 1, .stamina = 6.0f, .staminaRate = 1.0f};
	}
}

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

		const std::size_t enemyCount = std::max<std::size_t>(1, p_spawn.enemyTeam.size());
		const std::size_t playerCount = 2;
		const Ability &tackle = _registries.abilities().get("tackle");
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
			_battle = std::make_unique<BattleContext>(_context.events, std::move(board));
			_battle->placementStyle = p_spawn.placementStyle;
			_battle->allowsTaming = p_spawn.allowsTaming;
			_battle->returnWorldCell = center;

			for (std::size_t index = 0; index < playerCount; ++index)
			{
				(void)_battle->addUnit(
					{"Ally " + std::to_string(index + 1), playerStandIn(), {&tackle}}, BattleSide::Player);
			}
			for (std::size_t index = 0; index < enemyCount; ++index)
			{
				const std::string name = index < p_spawn.enemyTeam.size() && !p_spawn.enemyTeam[index].speciesId.empty()
											 ? p_spawn.enemyTeam[index].speciesId
											 : "Wild " + std::to_string(index + 1);
				(void)_battle->addUnit({name, enemyStandIn(), {&tackle}}, BattleSide::Enemy);
			}

			_context.events.battleStarted.trigger(_battle.get());
		} catch (const std::exception &exception)
		{
			std::cerr << "EncounterService: could not start battle: " << exception.what() << std::endl;
			_battle.reset();
		}
	}
}
