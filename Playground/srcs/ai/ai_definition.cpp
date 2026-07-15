#include "ai/ai_definition.hpp"

#include "core/content_id.hpp"

#include <algorithm>
#include <limits>
#include <set>
#include <variant>

namespace pg
{
	namespace
	{
		enum class AIConditionKind
		{
			Always,
			HealthRatio,
			ResourceAtLeast,
			Distance,
			HasStatus,
			HasStatusTag,
			AbilityAffordable
		};

		enum class AIAnchorKind
		{
			Unit,
			SelfCell,
			BestArea,
			NearestLegalCellTo
		};

		enum class AIDecisionKind
		{
			CastAbility,
			MoveToward,
			MoveAway,
			EndTurn
		};

		[[nodiscard]] const std::map<std::string, AIConditionKind> &conditionKindNames()
		{
			static const std::map<std::string, AIConditionKind> names{
				{"abilityAffordable", AIConditionKind::AbilityAffordable},
				{"always", AIConditionKind::Always},
				{"distance", AIConditionKind::Distance},
				{"hasStatus", AIConditionKind::HasStatus},
				{"hasStatusTag", AIConditionKind::HasStatusTag},
				{"healthRatio", AIConditionKind::HealthRatio},
				{"resourceAtLeast", AIConditionKind::ResourceAtLeast}};
			return names;
		}

		[[nodiscard]] const std::map<std::string, AIAnchorKind> &anchorKindNames()
		{
			static const std::map<std::string, AIAnchorKind> names{
				{"bestArea", AIAnchorKind::BestArea},
				{"nearestLegalCellTo", AIAnchorKind::NearestLegalCellTo},
				{"selfCell", AIAnchorKind::SelfCell},
				{"unit", AIAnchorKind::Unit}};
			return names;
		}

		[[nodiscard]] const std::map<std::string, AIDecisionKind> &decisionKindNames()
		{
			static const std::map<std::string, AIDecisionKind> names{
				{"castAbility", AIDecisionKind::CastAbility},
				{"endTurn", AIDecisionKind::EndTurn},
				{"moveAway", AIDecisionKind::MoveAway},
				{"moveToward", AIDecisionKind::MoveToward}};
			return names;
		}

		[[nodiscard]] std::string requireReferencedId(
			const JsonReader &p_reader,
			const std::string &p_key,
			std::string_view p_kind)
		{
			std::string value = p_reader.require<std::string>(p_key);
			requireContentId(value, p_reader.file(), p_reader.pathFor(p_key), p_kind);
			return value;
		}

		[[nodiscard]] std::uint32_t requireMovementPoints(JsonReader &p_reader)
		{
			// Zero is the authored "spend everything"; any other value is a real cap, so the range
			// is not simply [0, 1000000] with a magic hole in it.
			const std::int64_t authored =
				requireIntegerInRange(p_reader, "maximumMovementPoints", 0, MaximumAIMovementPoints);
			return static_cast<std::uint32_t>(authored);
		}

		[[nodiscard]] AIConditionSpec parseCondition(JsonReader &p_reader)
		{
			const AIConditionKind kind = p_reader.requireEnum<AIConditionKind>("type", conditionKindNames());

			switch (kind)
			{
			case AIConditionKind::Always:
			{
				p_reader.forbidUnknown({"type"});
				return AIAlwaysCondition{};
			}
			case AIConditionKind::HealthRatio:
			{
				p_reader.forbidUnknown({"type", "selector", "comparison", "permille"});

				AIHealthRatioCondition spec;
				spec.selector = p_reader.requireEnum<AIUnitSelector>("selector", aiUnitSelectorNames());
				spec.comparison = p_reader.requireEnum<AIInclusiveComparison>("comparison", aiInclusiveComparisonNames());
				spec.permille = static_cast<std::uint16_t>(
					requireIntegerInRange(p_reader, "permille", 0, MaximumAIHealthPermille));
				return spec;
			}
			case AIConditionKind::ResourceAtLeast:
			{
				p_reader.forbidUnknown({"type", "resource", "amount"});

				AIResourceCondition spec;
				spec.resource = p_reader.requireEnum<BattleResource>("resource", battleResourceNames());
				// A runtime pool is an int, so a JSON value above INT_MAX fails here with its path
				// rather than narrowing into a negative amount.
				spec.amount = static_cast<int>(requireIntegerInRange(
					p_reader,
					"amount",
					0,
					static_cast<std::int64_t>(std::numeric_limits<int>::max())));
				return spec;
			}
			case AIConditionKind::Distance:
			{
				p_reader.forbidUnknown({"type", "selector", "comparison", "cells"});

				AIDistanceCondition spec;
				spec.selector = p_reader.requireEnum<AIUnitSelector>("selector", aiUnitSelectorNames());
				spec.comparison = p_reader.requireEnum<AIInclusiveComparison>("comparison", aiInclusiveComparisonNames());
				spec.cells =
					static_cast<std::uint16_t>(requireIntegerInRange(p_reader, "cells", 0, MaximumAIDistanceCells));
				return spec;
			}
			case AIConditionKind::HasStatus:
			{
				p_reader.forbidUnknown({"type", "selector", "status", "present"});

				AIStatusCondition spec;
				spec.selector = p_reader.requireEnum<AIUnitSelector>("selector", aiUnitSelectorNames());
				spec.reference = AIStatusIdReference{requireReferencedId(p_reader, "status", "status id")};
				spec.present = p_reader.require<bool>("present");
				return spec;
			}
			case AIConditionKind::HasStatusTag:
			{
				p_reader.forbidUnknown({"type", "selector", "tag", "present"});

				AIStatusCondition spec;
				spec.selector = p_reader.requireEnum<AIUnitSelector>("selector", aiUnitSelectorNames());
				spec.reference = AIStatusTagReference{requireReferencedId(p_reader, "tag", "status tag")};
				spec.present = p_reader.require<bool>("present");
				return spec;
			}
			case AIConditionKind::AbilityAffordable:
			{
				p_reader.forbidUnknown({"type", "ability"});

				AIAbilityAffordableCondition spec;
				spec.abilityId = requireReferencedId(p_reader, "ability", "ability id");
				return spec;
			}
			}

			throw JsonError(p_reader.file(), p_reader.path(), "unhandled AI condition type");
		}

		[[nodiscard]] AIAnchorSpec parseAnchor(JsonReader &p_reader)
		{
			const AIAnchorKind kind = p_reader.requireEnum<AIAnchorKind>("type", anchorKindNames());

			switch (kind)
			{
			case AIAnchorKind::Unit:
			{
				p_reader.forbidUnknown({"type", "selector"});

				AIUnitAnchor spec;
				spec.selector = p_reader.requireEnum<AIUnitSelector>("selector", aiUnitSelectorNames());
				return spec;
			}
			case AIAnchorKind::SelfCell:
			{
				p_reader.forbidUnknown({"type"});
				return AISelfCellAnchor{};
			}
			case AIAnchorKind::BestArea:
			{
				p_reader.forbidUnknown({"type", "preferred"});

				AIBestAreaAnchor spec;
				spec.preferred = p_reader.requireEnum<AIBestAreaPreference>("preferred", aiBestAreaPreferenceNames());
				return spec;
			}
			case AIAnchorKind::NearestLegalCellTo:
			{
				p_reader.forbidUnknown({"type", "selector"});

				AINearestLegalCellToAnchor spec;
				spec.selector = p_reader.requireEnum<AIUnitSelector>("selector", aiUnitSelectorNames());
				return spec;
			}
			}

			throw JsonError(p_reader.file(), p_reader.path(), "unhandled AI anchor type");
		}

		[[nodiscard]] AIDecisionSpec parseDecision(JsonReader &p_reader)
		{
			const AIDecisionKind kind = p_reader.requireEnum<AIDecisionKind>("type", decisionKindNames());

			switch (kind)
			{
			case AIDecisionKind::CastAbility:
			{
				p_reader.forbidUnknown({"type", "ability", "anchor"});

				AICastAbilityDecision spec;
				spec.abilityId = requireReferencedId(p_reader, "ability", "ability id");

				JsonReader anchor = p_reader.child("anchor");
				spec.anchor = parseAnchor(anchor);
				return spec;
			}
			case AIDecisionKind::MoveToward:
			{
				p_reader.forbidUnknown({"type", "selector", "maximumMovementPoints"});

				AIMoveTowardDecision spec;
				spec.selector = p_reader.requireEnum<AIUnitSelector>("selector", aiUnitSelectorNames());
				spec.maximumMovementPoints = requireMovementPoints(p_reader);
				return spec;
			}
			case AIDecisionKind::MoveAway:
			{
				p_reader.forbidUnknown({"type", "selector", "maximumMovementPoints"});

				AIMoveAwayDecision spec;
				spec.selector = p_reader.requireEnum<AIUnitSelector>("selector", aiUnitSelectorNames());
				spec.maximumMovementPoints = requireMovementPoints(p_reader);
				return spec;
			}
			case AIDecisionKind::EndTurn:
			{
				p_reader.forbidUnknown({"type"});
				return AIEndTurnDecision{};
			}
			}

			throw JsonError(p_reader.file(), p_reader.path(), "unhandled AI decision type");
		}

		[[nodiscard]] AIRuleDefinition parseRule(JsonReader &p_reader)
		{
			p_reader.forbidUnknown({"id", "conditions", "decision"});

			AIRuleDefinition result;
			result.id = p_reader.require<std::string>("id");
			requireContentId(result.id, p_reader.file(), p_reader.pathFor("id"), "AI rule id");
			result.source = DefinitionSource{p_reader.file(), p_reader.path()};

			std::vector<JsonReader> conditions = p_reader.childArray("conditions");
			result.conditions.reserve(conditions.size());
			for (JsonReader &condition : conditions)
			{
				result.conditions.push_back(parseCondition(condition));
			}

			// "always" is the readable spelling of the empty list, so combining the two would be two
			// spellings of one rule - and a reader would have to check whether they agree.
			const bool hasAlways = std::ranges::any_of(result.conditions, [](const AIConditionSpec &p_condition) {
				return std::holds_alternative<AIAlwaysCondition>(p_condition);
			});
			if (hasAlways && result.conditions.size() > 1)
			{
				throw JsonError(
					p_reader.file(),
					p_reader.pathFor("conditions"),
					"rule '" + result.id + "' combines 'always' with another condition; 'always' stands alone");
			}

			JsonReader decision = p_reader.child("decision");
			result.decision = parseDecision(decision);
			return result;
		}

		[[nodiscard]] bool isUnconditional(const AIRuleDefinition &p_rule) noexcept
		{
			return p_rule.conditions.empty() ||
				   (p_rule.conditions.size() == 1 && std::holds_alternative<AIAlwaysCondition>(p_rule.conditions[0]));
		}
	}

	const std::map<std::string, AIUnitSelector> &aiUnitSelectorNames()
	{
		static const std::map<std::string, AIUnitSelector> names{
			{"highestHealthAlly", AIUnitSelector::HighestHealthAlly},
			{"highestHealthEnemy", AIUnitSelector::HighestHealthEnemy},
			{"lowestHealthAlly", AIUnitSelector::LowestHealthAlly},
			{"lowestHealthEnemy", AIUnitSelector::LowestHealthEnemy},
			{"nearestAlly", AIUnitSelector::NearestAlly},
			{"nearestEnemy", AIUnitSelector::NearestEnemy},
			{"self", AIUnitSelector::Self}};
		return names;
	}

	const std::map<std::string, AIInclusiveComparison> &aiInclusiveComparisonNames()
	{
		static const std::map<std::string, AIInclusiveComparison> names{
			{"atLeast", AIInclusiveComparison::AtLeast},
			{"atMost", AIInclusiveComparison::AtMost}};
		return names;
	}

	const std::map<std::string, AIBestAreaPreference> &aiBestAreaPreferenceNames()
	{
		static const std::map<std::string, AIBestAreaPreference> names{
			{"allies", AIBestAreaPreference::Allies},
			{"enemies", AIBestAreaPreference::Enemies}};
		return names;
	}

	AIBehaviourDefinition parseAIBehaviourDefinition(JsonReader &p_reader)
	{
		p_reader.forbidUnknown({"version", "displayNameKey", "rules"});
		requireVersion(p_reader, AISchemaVersion);

		AIBehaviourDefinition result;
		result.displayNameKey = requireDisplayNameKey(p_reader);
		result.source = DefinitionSource{p_reader.file(), p_reader.path()};

		std::vector<JsonReader> entries = p_reader.childArray("rules");
		if (entries.empty())
		{
			throw JsonError(p_reader.file(), p_reader.pathFor("rules"), "a behaviour has at least one rule");
		}

		std::set<std::string> ruleIds;
		result.rules.reserve(entries.size());
		for (JsonReader &entry : entries)
		{
			AIRuleDefinition rule = parseRule(entry);
			if (!ruleIds.insert(rule.id).second)
			{
				throw JsonError(entry.file(), entry.path(), "duplicate AI rule id '" + rule.id + "'");
			}
			result.rules.push_back(std::move(rule));
		}

		// The termination proof, and it is deliberately a syntactic one: every path through the
		// rule list reaches a rule that always matches and always produces a legal command. Without
		// it, a behaviour whose every rule is conditional could hold the scheduler forever.
		const AIRuleDefinition &last = result.rules.back();
		if (!isUnconditional(last) || !std::holds_alternative<AIEndTurnDecision>(last.decision))
		{
			throw JsonError(
				last.source.file,
				last.source.jsonPath,
				"the last rule of a behaviour is its termination fallback: it takes no condition and its decision "
				"is 'endTurn'; rule '" +
					last.id + "' is not");
		}
		return result;
	}

	std::vector<std::string> aiCastAbilityReferences(const AIBehaviourDefinition &p_behaviour)
	{
		std::vector<std::string> result;
		for (const AIRuleDefinition &rule : p_behaviour.rules)
		{
			if (const auto *cast = std::get_if<AICastAbilityDecision>(&rule.decision))
			{
				result.push_back(cast->abilityId);
			}
		}
		return result;
	}
}
