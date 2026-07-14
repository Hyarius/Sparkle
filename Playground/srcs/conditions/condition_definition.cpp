#include "conditions/condition_definition.hpp"

#include <algorithm>
#include <variant>

namespace pg
{
	namespace
	{
		void collectFrom(
			const std::vector<std::string> &p_ids,
			const DefinitionSource &p_source,
			std::vector<ConditionReference> &p_target)
		{
			for (const std::string &id : p_ids)
			{
				p_target.push_back(ConditionReference{id, p_source});
			}
		}

		void collectFrom(const ConditionSpec &p_condition, ConditionReferences &p_references)
		{
			const DefinitionSource &source = p_condition.source;

			std::visit(
				[&source, &p_references](const auto &p_payload) {
					using Payload = std::decay_t<decltype(p_payload)>;

					if constexpr (
						std::is_same_v<Payload, DamageConditionSpec> || std::is_same_v<Payload, HealingConditionSpec>)
					{
						collectFrom(p_payload.sourceAbilities, source, p_references.abilities);
					}
					else if constexpr (std::is_same_v<Payload, AbilityCastConditionSpec>)
					{
						collectFrom(p_payload.abilities, source, p_references.abilities);
					}
					else if constexpr (std::is_same_v<Payload, StatusConditionSpec>)
					{
						collectFrom(p_payload.statuses, source, p_references.statuses);
					}
					else if constexpr (std::is_same_v<Payload, EventAbsenceConditionSpec>)
					{
						std::visit(
							[&source, &p_references](const auto &p_predicate) {
								using Predicate = std::decay_t<decltype(p_predicate)>;

								if constexpr (std::is_same_v<Predicate, DamageAbsencePredicate>)
								{
									collectFrom(p_predicate.sourceAbilities, source, p_references.abilities);
								}
								else if constexpr (std::is_same_v<Predicate, AbilityCastAbsencePredicate>)
								{
									collectFrom(p_predicate.abilities, source, p_references.abilities);
								}
								else if constexpr (std::is_same_v<Predicate, StatusAbsencePredicate>)
								{
									collectFrom(p_predicate.statuses, source, p_references.statuses);
								}
							},
							p_payload.predicate);
					}
					else if constexpr (
						std::is_same_v<Payload, AllOfConditionSpec> || std::is_same_v<Payload, AnyOfConditionSpec>)
					{
						for (const ConditionSpec &child : p_payload.children)
						{
							collectFrom(child, p_references);
						}
					}
				},
				p_condition.payload);
		}
	}

	const std::map<std::string, ConditionUnitSet> &conditionUnitSetNames()
	{
		static const std::map<std::string, ConditionUnitSet> names{
			{"any", ConditionUnitSet::Any},
			{"opponentTeam", ConditionUnitSet::OpponentTeam},
			{"subject", ConditionUnitSet::Subject},
			{"subjectAllies", ConditionUnitSet::SubjectAllies},
			{"subjectTeam", ConditionUnitSet::SubjectTeam}};
		return names;
	}

	const std::map<std::string, ConditionEventRole> &conditionEventRoleNames()
	{
		static const std::map<std::string, ConditionEventRole> names{
			{"either", ConditionEventRole::Either},
			{"source", ConditionEventRole::Source},
			{"target", ConditionEventRole::Target}};
		return names;
	}

	const std::map<std::string, ConditionWindow> &conditionWindowNames()
	{
		static const std::map<std::string, ConditionWindow> names{
			{"ability", ConditionWindow::Ability},
			{"fight", ConditionWindow::Fight},
			{"game", ConditionWindow::Game},
			{"turn", ConditionWindow::Turn}};
		return names;
	}

	const std::map<std::string, WindowCountMode> &windowCountModeNames()
	{
		static const std::map<std::string, WindowCountMode> names{
			{"consecutive", WindowCountMode::Consecutive},
			{"cumulative", WindowCountMode::Cumulative}};
		return names;
	}

	const std::map<std::string, ConditionAggregation> &conditionAggregationNames()
	{
		static const std::map<std::string, ConditionAggregation> names{
			{"count", ConditionAggregation::Count},
			{"maximum", ConditionAggregation::Maximum},
			{"minimum", ConditionAggregation::Minimum},
			{"sum", ConditionAggregation::Sum}};
		return names;
	}

	const std::map<std::string, ConditionComparison> &conditionComparisonNames()
	{
		static const std::map<std::string, ConditionComparison> names{
			{"atLeast", ConditionComparison::AtLeast},
			{"atMost", ConditionComparison::AtMost},
			{"between", ConditionComparison::Between},
			{"equal", ConditionComparison::Equal},
			{"greaterThan", ConditionComparison::GreaterThan},
			{"lessThan", ConditionComparison::LessThan}};
		return names;
	}

	const std::map<std::string, DamageKindFilter> &damageKindFilterNames()
	{
		static const std::map<std::string, DamageKindFilter> names{
			{"any", DamageKindFilter::Any},
			{"magical", DamageKindFilter::Magical},
			{"physical", DamageKindFilter::Physical}};
		return names;
	}

	const std::map<std::string, DamageComponent> &damageComponentNames()
	{
		static const std::map<std::string, DamageComponent> names{
			{"health", DamageComponent::Health},
			{"shield", DamageComponent::Shield},
			{"total", DamageComponent::Total}};
		return names;
	}

	const std::map<std::string, PresenceFilter> &presenceFilterNames()
	{
		static const std::map<std::string, PresenceFilter> names{
			{"any", PresenceFilter::Any},
			{"forbidden", PresenceFilter::Forbidden},
			{"required", PresenceFilter::Required}};
		return names;
	}

	const std::map<std::string, StatusConditionAction> &statusConditionActionNames()
	{
		static const std::map<std::string, StatusConditionAction> names{
			{"applied", StatusConditionAction::Applied},
			{"removed", StatusConditionAction::Removed}};
		return names;
	}

	const std::map<std::string, ShieldConditionAction> &shieldConditionActionNames()
	{
		static const std::map<std::string, ShieldConditionAction> names{
			{"absorbed", ShieldConditionAction::Absorbed},
			{"applied", ShieldConditionAction::Applied},
			{"broken", ShieldConditionAction::Broken}};
		return names;
	}

	const std::map<std::string, MovementFilter> &movementFilterNames()
	{
		static const std::map<std::string, MovementFilter> names{
			{"any", MovementFilter::Any},
			{"displacement", MovementFilter::Displacement},
			{"teleport", MovementFilter::Teleport},
			{"voluntary", MovementFilter::Voluntary}};
		return names;
	}

	const std::map<std::string, MovementDirection> &movementDirectionNames()
	{
		static const std::map<std::string, MovementDirection> names{
			{"any", MovementDirection::Any},
			{"awayFromSource", MovementDirection::AwayFromSource},
			{"towardSource", MovementDirection::TowardSource}};
		return names;
	}

	const std::map<std::string, ResourceConditionAction> &resourceConditionActionNames()
	{
		static const std::map<std::string, ResourceConditionAction> names{
			{"gained", ResourceConditionAction::Gained},
			{"lost", ResourceConditionAction::Lost},
			{"nextActivationPenalty", ResourceConditionAction::NextActivationPenalty},
			{"spent", ResourceConditionAction::Spent}};
		return names;
	}

	const std::map<std::string, ResourceChangeReason> &resourceChangeReasonNames()
	{
		static const std::map<std::string, ResourceChangeReason> names{
			{"abilityCost", ResourceChangeReason::AbilityCost},
			{"activationRefill", ResourceChangeReason::ActivationRefill},
			{"effect", ResourceChangeReason::Effect},
			{"effectiveMaximumClamp", ResourceChangeReason::EffectiveMaximumClamp},
			{"movementCost", ResourceChangeReason::MovementCost},
			{"nextActivationPenaltyConsumption", ResourceChangeReason::NextActivationPenaltyConsumption}};
		return names;
	}

	const std::map<std::string, PositionSample> &positionSampleNames()
	{
		static const std::map<std::string, PositionSample> names{
			{"afterMove", PositionSample::AfterMove},
			{"throughoutWindow", PositionSample::ThroughoutWindow},
			{"turnEnd", PositionSample::TurnEnd},
			{"turnStart", PositionSample::TurnStart}};
		return names;
	}

	const std::map<std::string, UnitRemovalReasonFilter> &unitRemovalReasonFilterNames()
	{
		static const std::map<std::string, UnitRemovalReasonFilter> names{
			{"any", UnitRemovalReasonFilter::Any},
			{"defeated", UnitRemovalReasonFilter::Defeated},
			{"impressed", UnitRemovalReasonFilter::Impressed}};
		return names;
	}

	const std::map<std::string, RelativeBattleOutcome> &relativeBattleOutcomeNames()
	{
		static const std::map<std::string, RelativeBattleOutcome> names{
			{"completed", RelativeBattleOutcome::Completed},
			{"draw", RelativeBattleOutcome::Draw},
			{"opponentTeamVictory", RelativeBattleOutcome::OpponentTeamVictory},
			{"subjectTeamVictory", RelativeBattleOutcome::SubjectTeamVictory}};
		return names;
	}

	const std::map<std::string, SurvivorState> &survivorStateNames()
	{
		static const std::map<std::string, SurvivorState> names{
			{"active", SurvivorState::Active},
			{"notDefeated", SurvivorState::NotDefeated}};
		return names;
	}

	bool isResourceReasonCompatible(ResourceConditionAction p_action, ResourceChangeReason p_reason) noexcept
	{
		switch (p_action)
		{
		case ResourceConditionAction::Spent:
			return p_reason == ResourceChangeReason::AbilityCost || p_reason == ResourceChangeReason::MovementCost;
		case ResourceConditionAction::Gained:
			return p_reason == ResourceChangeReason::Effect || p_reason == ResourceChangeReason::ActivationRefill;
		case ResourceConditionAction::Lost:
			return p_reason == ResourceChangeReason::Effect ||
				   p_reason == ResourceChangeReason::NextActivationPenaltyConsumption ||
				   p_reason == ResourceChangeReason::EffectiveMaximumClamp;
		case ResourceConditionAction::NextActivationPenalty:
			// The applied accumulation event, never its later consumption.
			return p_reason == ResourceChangeReason::Effect;
		}
		return false;
	}

	ConditionLimits conditionLimits(const GameRules &p_rules) noexcept
	{
		return ConditionLimits{p_rules.battle.maxConditionDepth, p_rules.battle.teamCapacity};
	}

	void collectConditionReferences(
		const std::vector<ConditionSpec> &p_conditions,
		ConditionReferences &p_references)
	{
		for (const ConditionSpec &condition : p_conditions)
		{
			collectFrom(condition, p_references);
		}
	}
}
