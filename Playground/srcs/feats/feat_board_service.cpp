#include "feats/feat_board_service.hpp"

#include "battle/battle_context.hpp"
#include "battle/battle_unit.hpp"
#include "core/event_center.hpp"
#include "creatures/apply_progress.hpp"
#include "creatures/creature_species.hpp"
#include "creatures/creature_unit.hpp"
#include "feats/feat_board.hpp"
#include "feats/feat_progress.hpp"

#include <algorithm>
#include <iostream>
#include <optional>
#include <vector>

namespace pg
{
	FeatBoardService::FeatBoardService(EventCenter &p_events) :
		_events(p_events),
		_battleResolvedContract(_events.battleResolved.subscribe(
			[this](BattleContext *p_context, BattleSide p_winner) {
				_onBattleResolved(p_context, p_winner);
			}))
	{
	}

	int FeatBoardService::registerFightEvents(
		CreatureUnit &p_unit,
		std::span<const BattleEvent *const> p_events,
		const BattleUnit *p_subject) const
	{
		if (p_unit.species == nullptr || p_unit.species->featBoard == nullptr)
		{
			return 0;
		}

		const FeatBoard &board = *p_unit.species->featBoard;
		const BattleUnit *subject = p_subject;
		if (subject == nullptr)
		{
			for (const BattleEvent *event : p_events)
			{
				if (event == nullptr)
				{
					continue;
				}
				const BattleEventContext &context = battleEventContext(*event);
				if (context.caster != nullptr && context.caster->source() == &p_unit)
				{
					subject = context.caster;
					break;
				}
				if (context.target != nullptr && context.target->source() == &p_unit)
				{
					subject = context.target;
					break;
				}
			}
		}
		std::vector<spk::UUID> evaluatedNodes;
		int completions = 0;
		bool completedNode = false;

		p_unit.featBoardProgress.resetTransientRequirementProgress();
		do
		{
			completedNode = false;
			for (const FeatNode &node : board.nodes)
			{
				if (std::ranges::find(evaluatedNodes, node.uuid) != evaluatedNodes.end() ||
					!board.isReachable(node, p_unit))
				{
					continue;
				}

				evaluatedNodes.push_back(node.uuid);
				FeatNodeProgress &nodeProgress = p_unit.featBoardProgress.getOrCreateProgress(node);
				for (FeatRequirementProgress &requirement : nodeProgress.requirements)
				{
					if (requirement.condition != nullptr)
					{
						requirement.advancement = requirement.condition->evaluateEvents(
							p_events, requirement.advancement, subject);
					}
				}

				if (nodeProgress.isCompleted())
				{
					nodeProgress.complete();
					++completions;
					completedNode = true;
					// Form rewards affect reachability, so replay rewards before the next pass.
					applyProgress(p_unit);
				}
			}
		} while (completedNode);

		p_unit.featBoardProgress.resetTransientRequirementProgress();
		if (completions > 0)
		{
			applyProgress(p_unit);
		}
		return completions;
	}

	void FeatBoardService::_onBattleResolved(BattleContext *p_context, BattleSide p_winner)
	{
		if (p_context == nullptr)
		{
			return;
		}

		for (BattleUnit *battleUnit : p_context->getUnits(BattleSide::Player))
		{
			if (battleUnit == nullptr || battleUnit->source() == nullptr)
			{
				continue;
			}

			std::vector<const BattleEvent *> unitEvents;
			bool hasBattleWon = false;
			for (const BattleEvent &event : p_context->log.events())
			{
				const BattleEventContext &context = battleEventContext(event);
				if (context.caster == battleUnit ||
					(context.target == battleUnit && context.target != context.caster))
				{
					unitEvents.push_back(&event);
					hasBattleWon = hasBattleWon || event.getIf<BattleWonEvent>() != nullptr;
				}
			}

			std::optional<BattleEvent> syntheticBattleWon;
			if (p_winner == BattleSide::Player && !hasBattleWon)
			{
				syntheticBattleWon.emplace(BattleWonEvent{
					.context = {.turnIndex = p_context->currentTurn.turnIndex, .caster = battleUnit},
					.side = BattleSide::Player,
					.unitSurvived = !battleUnit->isDefeated()});
				unitEvents.push_back(&*syntheticBattleWon);
			}

			CreatureUnit &creature = *battleUnit->source();
			const nlohmann::json progressBefore = creature.featBoardProgress.toJson();
			const int completions = registerFightEvents(creature, unitEvents, battleUnit);
			if (completions > 0 || creature.featBoardProgress.toJson() != progressBefore)
			{
				_events.featProgressUpdated.trigger(&creature, completions);
				if (completions > 0)
				{
					std::cout << creature.displayName() << ": " << completions
							  << (completions == 1 ? " feat completed" : " feats completed") << std::endl;
				}
			}
		}
	}
}
