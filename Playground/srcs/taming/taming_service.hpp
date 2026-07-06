#pragma once

#include "battle/battle_side.hpp"
#include "structures/design_pattern/spk_contract_provider.hpp"

namespace pg
{
	class BattleContext;
	struct BattleEvent;
	struct GameContext;

	class TamingService
	{
	private:
		GameContext &_game;
		BattleContext *_activeBattle = nullptr;
		spk::ContractProvider<BattleContext *>::Contract _battleStartedContract;
		spk::ContractProvider<const BattleEvent *>::Contract _battleEventContract;
		spk::ContractProvider<BattleContext *, BattleSide>::Contract _battleResolvedContract;

		void _onBattleEvent(const BattleEvent *p_event);
		void _onBattleResolved(BattleContext *p_context, BattleSide p_winner);

	public:
		explicit TamingService(GameContext &p_game);
	};
}
