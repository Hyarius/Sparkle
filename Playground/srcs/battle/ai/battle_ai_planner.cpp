#include "battle/ai/battle_ai_planner.hpp"

#include "abilities/ability_definition.hpp"
#include "battle/battle_session.hpp"
#include "battle/battle_snapshot.hpp"
#include "battle/effects/effect_resolver.hpp"
#include "core/registries.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <tuple>

namespace pg
{
	namespace
	{
		[[nodiscard]] bool isActive(const BattleUnitSnapshot &p_unit) noexcept
		{
			return p_unit.placed && p_unit.cell.has_value() && p_unit.health > 0 &&
				   p_unit.removalReason == RemovalReason::None;
		}

		[[nodiscard]] const BattleUnitSnapshot *findUnit(const BattleSnapshot &p_snapshot, BattleUnitId p_id) noexcept
		{
			const auto found = std::ranges::find(p_snapshot.units, p_id, &BattleUnitSnapshot::id);
			return found == p_snapshot.units.end() ? nullptr : &*found;
		}

		[[nodiscard]] std::uint64_t absDifference(int p_left, int p_right) noexcept
		{
			const std::int64_t difference = static_cast<std::int64_t>(p_left) - static_cast<std::int64_t>(p_right);
			return static_cast<std::uint64_t>(difference < 0 ? -difference : difference);
		}

		[[nodiscard]] std::uint64_t manhattanXZ(const BoardCell &p_left, const BoardCell &p_right) noexcept
		{
			return absDifference(p_left.x, p_right.x) + absDifference(p_left.z, p_right.z);
		}

		[[nodiscard]] bool rosterThenIdBefore(const BattleUnitSnapshot &p_left, const BattleUnitSnapshot &p_right) noexcept
		{
			return std::tuple{p_left.rosterOrder, p_left.id.value()} < std::tuple{p_right.rosterOrder, p_right.id.value()};
		}

		[[nodiscard]] std::uint64_t healthPermille(const BattleUnitSnapshot &p_unit)
		{
			const std::int64_t maximum = std::max<std::int64_t>(1, p_unit.maxHealth);
			const std::int64_t health = std::max<std::int64_t>(0, p_unit.health);
			if (health > std::numeric_limits<std::int64_t>::max() / 1000)
			{
				throw std::overflow_error("AI health ratio multiplication overflow");
			}
			return static_cast<std::uint64_t>((health * 1000) / maximum);
		}

		[[nodiscard]] std::optional<BattleUnitId> selectUnit(
			const BattleSnapshot &p_snapshot,
			const BattleUnitSnapshot &p_actor,
			AIUnitSelector p_selector)
		{
			if (p_selector == AIUnitSelector::Self)
			{
				return p_actor.id;
			}

			const bool wantsAlly = p_selector == AIUnitSelector::NearestAlly ||
								   p_selector == AIUnitSelector::LowestHealthAlly || p_selector == AIUnitSelector::HighestHealthAlly;
			const bool nearest = p_selector == AIUnitSelector::NearestAlly || p_selector == AIUnitSelector::NearestEnemy;
			const bool highest = p_selector == AIUnitSelector::HighestHealthAlly || p_selector == AIUnitSelector::HighestHealthEnemy;
			const BoardCell actorCell = *p_actor.cell;
			const BattleUnitSnapshot *best = nullptr;

			for (const BattleUnitSnapshot &candidate : p_snapshot.units)
			{
				if (!isActive(candidate) || candidate.id == p_actor.id || (candidate.side == p_actor.side) != wantsAlly)
				{
					continue;
				}
				if (best == nullptr)
				{
					best = &candidate;
					continue;
				}

				bool replace = false;
				if (nearest)
				{
					const std::uint64_t candidateDistance = manhattanXZ(actorCell, *candidate.cell);
					const std::uint64_t bestDistance = manhattanXZ(actorCell, *best->cell);
					replace = candidateDistance < bestDistance ||
							  (candidateDistance == bestDistance && rosterThenIdBefore(candidate, *best));
				}
				else
				{
					const std::uint64_t candidateRatio = healthPermille(candidate);
					const std::uint64_t bestRatio = healthPermille(*best);
					replace = highest
								  ? candidateRatio > bestRatio || (candidateRatio == bestRatio && rosterThenIdBefore(candidate, *best))
								  : candidateRatio < bestRatio || (candidateRatio == bestRatio && rosterThenIdBefore(candidate, *best));
				}
				if (replace)
				{
					best = &candidate;
				}
			}
			return best == nullptr ? std::nullopt : std::optional<BattleUnitId>(best->id);
		}

		[[nodiscard]] bool compareInclusive(std::uint64_t p_actual, AIInclusiveComparison p_comparison, std::uint64_t p_expected) noexcept
		{
			return p_comparison == AIInclusiveComparison::AtMost ? p_actual <= p_expected : p_actual >= p_expected;
		}

		[[nodiscard]] bool ownsAbility(const BattleUnitSnapshot &p_unit, std::string_view p_abilityId)
		{
			return std::ranges::find(p_unit.abilityIds, p_abilityId) != p_unit.abilityIds.end();
		}

		[[nodiscard]] bool hasStatus(
			const BattleUnitSnapshot &p_unit,
			const AIStatusReference &p_reference,
			const Registries &p_registries)
		{
			const auto matches = [&](const BattleStatusSnapshot &status) {
				return std::visit(
					[&](const auto &reference) {
						using Reference = std::decay_t<decltype(reference)>;
						if constexpr (std::is_same_v<Reference, AIStatusIdReference>)
						{
							return status.definitionId == reference.statusId;
						}
						else
						{
							const StatusDefinition *definition = p_registries.statuses().tryGet(status.definitionId);
							return definition != nullptr && std::ranges::find(definition->tags, reference.tag) != definition->tags.end();
						}
					},
					p_reference);
			};
			return std::ranges::any_of(p_unit.passiveStatuses, matches) || std::ranges::any_of(p_unit.transientStatuses, matches);
		}

		[[nodiscard]] bool conditionMatches(
			const AIConditionSpec &p_condition,
			const BattleSnapshot &p_snapshot,
			const BattleUnitSnapshot &p_actor,
			const Registries &p_registries)
		{
			return std::visit(
				[&](const auto &condition) {
					using Condition = std::decay_t<decltype(condition)>;
					if constexpr (std::is_same_v<Condition, AIAlwaysCondition>)
					{
						return true;
					}
					else if constexpr (std::is_same_v<Condition, AIHealthRatioCondition>)
					{
						const std::optional<BattleUnitId> selected = selectUnit(p_snapshot, p_actor, condition.selector);
						const BattleUnitSnapshot *target = selected ? findUnit(p_snapshot, *selected) : nullptr;
						return target != nullptr && compareInclusive(healthPermille(*target), condition.comparison, condition.permille);
					}
					else if constexpr (std::is_same_v<Condition, AIResourceCondition>)
					{
						const int current = condition.resource == BattleResource::ActionPoints ? p_actor.actionPoints : p_actor.movementPoints;
						return current >= condition.amount;
					}
					else if constexpr (std::is_same_v<Condition, AIDistanceCondition>)
					{
						const std::optional<BattleUnitId> selected = selectUnit(p_snapshot, p_actor, condition.selector);
						const BattleUnitSnapshot *target = selected ? findUnit(p_snapshot, *selected) : nullptr;
						return target != nullptr && compareInclusive(manhattanXZ(*p_actor.cell, *target->cell), condition.comparison, condition.cells);
					}
					else if constexpr (std::is_same_v<Condition, AIStatusCondition>)
					{
						const std::optional<BattleUnitId> selected = selectUnit(p_snapshot, p_actor, condition.selector);
						const BattleUnitSnapshot *target = selected ? findUnit(p_snapshot, *selected) : nullptr;
						return target != nullptr && hasStatus(*target, condition.reference, p_registries) == condition.present;
					}
					else
					{
						const AbilityDefinition *ability = p_registries.abilities().tryGet(condition.abilityId);
						return ability != nullptr && ownsAbility(p_actor, condition.abilityId) &&
							   p_actor.actionPoints >= ability->cost.actionPoints && p_actor.movementPoints >= ability->cost.movementPoints;
					}
				},
				p_condition);
		}

		[[nodiscard]] std::vector<CastPlan> legalAnchors(const BattleSession &p_session, BattleUnitId p_actor, std::string_view p_abilityId)
		{
			std::vector<CastPlan> result;
			for (AbilityAnchorPreview preview : p_session.abilityAnchors(p_actor, p_abilityId))
			{
				if (preview.legal && preview.plan.has_value())
				{
					result.push_back(std::move(*preview.plan));
				}
			}
			return result;
		}

		[[nodiscard]] std::optional<BoardCell> chooseAnchor(
			const AIAnchorSpec &p_anchor,
			const BattleSession &p_session,
			const BattleSnapshot &p_snapshot,
			const BattleUnitSnapshot &p_actor,
			std::string_view p_abilityId)
		{
			return std::visit(
				[&](const auto &anchor) -> std::optional<BoardCell> {
					using Anchor = std::decay_t<decltype(anchor)>;
					if constexpr (std::is_same_v<Anchor, AIUnitAnchor>)
					{
						const std::optional<BattleUnitId> selected = selectUnit(p_snapshot, p_actor, anchor.selector);
						const BattleUnitSnapshot *target = selected ? findUnit(p_snapshot, *selected) : nullptr;
						if (target == nullptr)
						{
							return std::nullopt;
						}
						auto plan = p_session.planCast(p_actor.id, p_abilityId, *target->cell);
						return plan ? std::optional<BoardCell>(plan->anchor) : std::nullopt;
					}
					else if constexpr (std::is_same_v<Anchor, AISelfCellAnchor>)
					{
						auto plan = p_session.planCast(p_actor.id, p_abilityId, *p_actor.cell);
						return plan ? std::optional<BoardCell>(plan->anchor) : std::nullopt;
					}
					else if constexpr (std::is_same_v<Anchor, AIBestAreaAnchor>)
					{
						const std::vector<CastPlan> plans = legalAnchors(p_session, p_actor.id, p_abilityId);
						const CastPlan *best = nullptr;
						std::size_t bestPreferred = 0;
						std::size_t bestOther = 0;
						const BattleSide preferredSide = anchor.preferred == AIBestAreaPreference::Enemies ? opposingSide(p_actor.side) : p_actor.side;
						for (const CastPlan &plan : plans)
						{
							std::size_t preferred = 0;
							std::size_t other = 0;
							for (const BattleUnitId id : plan.affectedUnits)
							{
								const BattleUnitSnapshot *affected = findUnit(p_snapshot, id);
								if (affected == nullptr || !isActive(*affected))
								{
									continue;
								}
								if (affected->side == preferredSide)
								{
									++preferred;
								}
								else
								{
									++other;
								}
							}
							if (best == nullptr || preferred > bestPreferred ||
								(preferred == bestPreferred && (other < bestOther ||
																(other == bestOther && BoardCellLess{}(plan.anchor, best->anchor)))))
							{
								best = &plan;
								bestPreferred = preferred;
								bestOther = other;
							}
						}
						return best == nullptr ? std::nullopt : std::optional<BoardCell>(best->anchor);
					}
					else
					{
						const std::optional<BattleUnitId> selected = selectUnit(p_snapshot, p_actor, anchor.selector);
						const BattleUnitSnapshot *target = selected ? findUnit(p_snapshot, *selected) : nullptr;
						if (target == nullptr)
						{
							return std::nullopt;
						}
						const std::vector<CastPlan> plans = legalAnchors(p_session, p_actor.id, p_abilityId);
						const CastPlan *best = nullptr;
						for (const CastPlan &plan : plans)
						{
							if (best == nullptr || manhattanXZ(plan.anchor, *target->cell) < manhattanXZ(best->anchor, *target->cell) ||
								(manhattanXZ(plan.anchor, *target->cell) == manhattanXZ(best->anchor, *target->cell) && BoardCellLess{}(plan.anchor, best->anchor)))
							{
								best = &plan;
							}
						}
						return best == nullptr ? std::nullopt : std::optional<BoardCell>(best->anchor);
					}
				},
				p_anchor);
		}

		[[nodiscard]] std::optional<BattleCommand> decide(
			const AIDecisionSpec &p_decision,
			const BattleSession &p_session,
			const BattleSnapshot &p_snapshot,
			const BattleUnitSnapshot &p_actor)
		{
			return std::visit(
				[&](const auto &decision) -> std::optional<BattleCommand> {
					using Decision = std::decay_t<decltype(decision)>;
					if constexpr (std::is_same_v<Decision, AICastAbilityDecision>)
					{
						const AbilityDefinition *ability = p_session.registries().abilities().tryGet(decision.abilityId);
						if (ability == nullptr || !ownsAbility(p_actor, decision.abilityId) ||
							p_actor.actionPoints < ability->cost.actionPoints || p_actor.movementPoints < ability->cost.movementPoints ||
							!EffectResolver::supportsAll(*ability))
						{
							return std::nullopt;
						}
						const std::optional<BoardCell> anchor = chooseAnchor(decision.anchor, p_session, p_snapshot, p_actor, decision.abilityId);
						return anchor ? std::optional<BattleCommand>(CastAbilityCommand{p_actor.id, decision.abilityId, *anchor}) : std::nullopt;
					}
					else if constexpr (std::is_same_v<Decision, AIMoveTowardDecision> || std::is_same_v<Decision, AIMoveAwayDecision>)
					{
						const std::optional<BattleUnitId> selected = selectUnit(p_snapshot, p_actor, decision.selector);
						const BattleUnitSnapshot *target = selected ? findUnit(p_snapshot, *selected) : nullptr;
						if (target == nullptr)
						{
							return std::nullopt;
						}
						const std::uint64_t currentMp = static_cast<std::uint64_t>(std::max(0, p_actor.movementPoints));
						const std::uint64_t budget = decision.maximumMovementPoints == 0
														 ? currentMp
														 : std::min(currentMp, static_cast<std::uint64_t>(decision.maximumMovementPoints));
						const std::uint64_t originDistance = manhattanXZ(*p_actor.cell, *target->cell);
						const bool toward = std::is_same_v<Decision, AIMoveTowardDecision>;
						const MovePlan *best = nullptr;
						for (const MovePlan &plan : p_session.legalMoves(p_actor.id))
						{
							if (plan.destination == plan.origin || plan.totalMovementPointCost > budget)
							{
								continue;
							}
							const std::uint64_t distance = manhattanXZ(plan.destination, *target->cell);
							if ((toward && distance >= originDistance) || (!toward && distance <= originDistance))
							{
								continue;
							}
							if (best == nullptr)
							{
								best = &plan;
								continue;
							}
							const std::uint64_t bestDistance = manhattanXZ(best->destination, *target->cell);
							const bool betterObjective = toward ? distance < bestDistance : distance > bestDistance;
							if (betterObjective ||
								(distance == bestDistance && (plan.totalMovementPointCost < best->totalMovementPointCost ||
															  (plan.totalMovementPointCost == best->totalMovementPointCost &&
															   (plan.steps.size() < best->steps.size() ||
																(plan.steps.size() == best->steps.size() && BoardCellLess{}(plan.destination, best->destination)))))))
							{
								best = &plan;
							}
						}
						return best == nullptr ? std::nullopt : std::optional<BattleCommand>(MoveCommand{p_actor.id, best->destination});
					}
					else
					{
						return BattleCommand(EndTurnCommand{p_actor.id, EndTurnRequestCause::Explicit});
					}
				},
				p_decision);
		}
	}

	AIPlanResult BattleAIPlanner::chooseNextCommand(
		const BattleSession &p_session,
		BattleUnitId p_unit,
		const AIBehaviourDefinition &p_behaviour) const
	{
		if (p_behaviour.id.empty())
		{
			return AIPlanFailure::MissingBehaviour;
		}
		const BattleSnapshot snapshot = p_session.snapshot();
		if (snapshot.phase != BattlePhase::Activation || !snapshot.activeUnit.has_value() || *snapshot.activeUnit != p_unit)
		{
			return AIPlanFailure::WrongPhase;
		}
		const BattleUnitSnapshot *actor = findUnit(snapshot, p_unit);
		if (actor == nullptr || !isActive(*actor))
		{
			return AIPlanFailure::UnitNotActive;
		}

		std::vector<AIRuleTrace> trace;
		for (const AIRuleDefinition &rule : p_behaviour.rules)
		{
			AIRuleTrace entry;
			entry.ruleId = rule.id;
			entry.conditionsMatched = true;
			for (const AIConditionSpec &condition : rule.conditions)
			{
				if (!conditionMatches(condition, snapshot, *actor, p_session.registries()))
				{
					entry.conditionsMatched = false;
					entry.detailCode = "conditionFalse";
					break;
				}
			}
			if (!entry.conditionsMatched)
			{
				trace.push_back(std::move(entry));
				continue;
			}
			const std::optional<BattleCommand> command = decide(rule.decision, p_session, snapshot, *actor);
			if (!command.has_value())
			{
				entry.detailCode = "noLegalCommand";
				trace.push_back(std::move(entry));
				continue;
			}
			entry.decisionProducedLegalCommand = true;
			trace.push_back(entry);
			return AIPlannedCommand{p_unit, p_behaviour.id, rule.id, *command, std::move(trace)};
		}
		return AIPlanFailure::NoRuleProducedCommand;
	}
}
