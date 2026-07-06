#pragma once

#include "battle/battle_event.hpp"
#include "battle/battle_side.hpp"
#include "structures/design_pattern/spk_contract_provider.hpp"

#include <span>

namespace pg
{
	class BattleContext;
	class BattleUnit;
	class CreatureUnit;
	struct EventCenter;

	class FeatBoardService
	{
	private:
		EventCenter &_events;
		spk::ContractProvider<BattleContext *, BattleSide>::Contract _battleResolvedContract;

		void _onBattleResolved(BattleContext *p_context, BattleSide p_winner);

	public:
		explicit FeatBoardService(EventCenter &p_events);

		[[nodiscard]] int registerFightEvents(
			CreatureUnit &p_unit,
			std::span<const BattleEvent *const> p_events,
			const BattleUnit *p_subject = nullptr) const;
	};
}
