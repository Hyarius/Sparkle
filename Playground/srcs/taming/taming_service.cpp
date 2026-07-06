#include "taming/taming_service.hpp"

#include "battle/battle_context.hpp"
#include "core/event_center.hpp"
#include "core/game_context.hpp"
#include "creatures/apply_progress.hpp"
#include "creatures/creature_species.hpp"
#include "creatures/creature_unit.hpp"
#include "feats/feat_board.hpp"
#include "feats/feat_progress.hpp"
#include "taming/wild_battle_unit.hpp"

#include <iostream>
#include <memory>

namespace pg
{
	TamingService::TamingService(GameContext &p_game) :
		_game(p_game),
		_battleStartedContract(_game.events.battleStarted.subscribe([this](BattleContext *p_context) {
			_activeBattle = p_context;
		})),
		_battleEventContract(_game.events.battleEventOccurred.subscribe([this](const BattleEvent *p_event) {
			_onBattleEvent(p_event);
		})),
		_battleResolvedContract(_game.events.battleResolved.subscribe(
			[this](BattleContext *p_context, BattleSide p_winner) {
				_onBattleResolved(p_context, p_winner);
			}))
	{
	}

	void TamingService::_onBattleEvent(const BattleEvent *p_event)
	{
		if (_activeBattle == nullptr || !_activeBattle->allowsTaming || p_event == nullptr)
		{
			return;
		}

		const BattleEvent *events[] = {p_event};
		for (BattleUnit *unit : _activeBattle->getUnits(BattleSide::Enemy))
		{
			auto *wild = dynamic_cast<WildBattleUnit *>(unit);
			if (wild == nullptr || wild->tamingProgress.isImpressed() ||
				wild->tamingProgress.hasFailed() || wild->isDefeated())
			{
				continue;
			}

			wild->tamingProgress.evaluateEvents(events);
			if (wild->tamingProgress.isImpressed())
			{
				wild->hasLeftBattle = true;
				(void)_activeBattle->removeUnit(*wild);
				_game.events.creatureImpressed.trigger(wild);
				std::cout << wild->displayName() << " was impressed!" << std::endl;
			}
		}
	}

	void TamingService::_onBattleResolved(BattleContext *p_context, BattleSide p_winner)
	{
		if (p_context == nullptr || p_context != _activeBattle)
		{
			return;
		}

		if (p_winner == BattleSide::Player && p_context->allowsTaming)
		{
			for (BattleUnit *unit : p_context->getUnits(BattleSide::Enemy))
			{
				auto *wild = dynamic_cast<WildBattleUnit *>(unit);
				if (wild == nullptr || !wild->tamingProgress.isImpressed() || wild->source() == nullptr ||
					wild->source()->species == nullptr)
				{
					continue;
				}

				auto recruit = std::make_unique<CreatureUnit>(*wild->source()->species);
				const FeatBoard *board = recruit->species->featBoard;
				if (board != nullptr)
				{
					for (const FeatNodeProgress &sourceProgress : wild->source()->featBoardProgress.nodes)
					{
						const FeatNode *node = board->tryNode(sourceProgress.nodeUuid);
						if (node != nullptr && sourceProgress.completionCount > 0)
						{
							recruit->featBoardProgress.getOrCreateProgress(*node).completionCount =
								sourceProgress.completionCount;
						}
					}
				}
				applyProgress(*recruit);
				CreatureUnit &added = _game.player.addCreatureToTeamOrStorage(std::move(recruit));
				_game.events.creatureRecruited.trigger(&added);
			}
		}
		_activeBattle = nullptr;
	}
}
