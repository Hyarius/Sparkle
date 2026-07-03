#pragma once

#include "encounters/encounter_types.hpp"
#include "structures/design_pattern/spk_contract_provider.hpp"
#include "structures/math/spk_vector3.hpp"

#include <functional>
#include <memory>
#include <vector>

namespace pg
{
	struct GameContext;
	class Registries;
	class BattleContext;
	class CreatureUnit;

	// First slice of battle entry (battle.md §7): on encounterTriggered it derives a board from the
	// live world under the player, instantiates the Milestone-1 stand-in teams, and fires
	// battleStarted with the owned BattleContext. Keeps the context alive until the next encounter
	// (never freeing it mid-tick — see the note in the .cpp). Real teams/species arrive in step 14.
	class EncounterService
	{
	public:
		using PlayerCellProvider = std::function<spk::Vector3Int()>;

	private:
		GameContext &_context;
		const Registries &_registries;
		PlayerCellProvider _playerCell;
		std::unique_ptr<BattleContext> _battle;
		std::vector<std::unique_ptr<CreatureUnit>> _enemyCreatures;
		spk::ContractProvider<const EncounterSpawn &>::Contract _encounterContract;

		void _onEncounter(const EncounterSpawn &p_spawn);

	public:
		EncounterService(GameContext &p_context, const Registries &p_registries, PlayerCellProvider p_playerCell);
		~EncounterService();

		[[nodiscard]] BattleContext *activeBattle() noexcept;
	};
}
