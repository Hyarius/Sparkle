#include "conditions/condition_definition.hpp"

#include "core/content_id.hpp"
#include "core/definition_fields.hpp"

#include <algorithm>
#include <initializer_list>
#include <string>
#include <string_view>

namespace pg
{
	namespace
	{
		enum class ConditionKind
		{
			Damage,
			Healing,
			AbilityCast,
			Status,
			Shield,
			Movement,
			Resource,
			Position,
			UnitRemoval,
			BattleOutcome,
			SurvivorCount,
			EventAbsence,
			AllOf,
			AnyOf
		};

		enum class AbsenceKind
		{
			Damage,
			AbilityCast,
			Status,
			Movement
		};

		[[nodiscard]] const std::map<std::string, ConditionKind> &conditionKindNames()
		{
			static const std::map<std::string, ConditionKind> names{
				{"abilityCast", ConditionKind::AbilityCast},
				{"allOf", ConditionKind::AllOf},
				{"anyOf", ConditionKind::AnyOf},
				{"battleOutcome", ConditionKind::BattleOutcome},
				{"damage", ConditionKind::Damage},
				{"eventAbsence", ConditionKind::EventAbsence},
				{"healing", ConditionKind::Healing},
				{"movement", ConditionKind::Movement},
				{"position", ConditionKind::Position},
				{"resource", ConditionKind::Resource},
				{"shield", ConditionKind::Shield},
				{"status", ConditionKind::Status},
				{"survivorCount", ConditionKind::SurvivorCount},
				{"unitRemoval", ConditionKind::UnitRemoval}};
			return names;
		}

		// The absence predicate is a closed set of filters, not a general negation operator: a
		// type outside these four is rejected rather than wrapped.
		[[nodiscard]] const std::map<std::string, AbsenceKind> &absenceKindNames()
		{
			static const std::map<std::string, AbsenceKind> names{
				{"abilityCast", AbsenceKind::AbilityCast},
				{"damage", AbsenceKind::Damage},
				{"movement", AbsenceKind::Movement},
				{"status", AbsenceKind::Status}};
			return names;
		}

		// Content-id filters in authored order, deduplicated. An empty array is the wildcard -
		// "any ability", "any status" - and is authored as [] rather than omitted, so the schema
		// stays uniform.
		[[nodiscard]] std::vector<std::string> requireIdFilter(
			const JsonReader &p_reader,
			const std::string &p_key,
			std::string_view p_kind)
		{
			const std::vector<std::string> authored = p_reader.require<std::vector<std::string>>(p_key);

			std::vector<std::string> result;
			result.reserve(authored.size());
			for (const std::string &id : authored)
			{
				requireContentId(id, p_reader.file(), p_reader.pathFor(p_key), p_kind);
				if (std::ranges::find(result, id) == result.end())
				{
					result.push_back(id);
				}
			}
			return result;
		}

		[[nodiscard]] std::uint32_t requireCount(
			const JsonReader &p_reader,
			const std::string &p_key,
			std::int64_t p_maximum)
		{
			return static_cast<std::uint32_t>(requireIntegerInRange(p_reader, p_key, 0, p_maximum));
		}

		// A leaf carrying one direct threshold has no second operand to bound.
		void forbidBetween(const JsonReader &p_reader, ConditionComparison p_comparison)
		{
			if (p_comparison == ConditionComparison::Between)
			{
				throw JsonError(
					p_reader.file(),
					p_reader.pathFor("comparison"),
					"this condition tests one direct threshold, so 'between' has no upper bound to read");
			}
		}

		void requireWindow(const JsonReader &p_reader, const ConditionLeafHeader &p_leaf, ConditionWindow p_window)
		{
			if (p_leaf.window != p_window)
			{
				throw JsonError(
					p_reader.file(),
					p_reader.pathFor("window"),
					"this condition is only meaningful over one whole fight, so its window must be 'fight'");
			}
		}

		ConditionLeafHeader parseLeafHeader(const JsonReader &p_reader)
		{
			ConditionLeafHeader result;
			result.window = p_reader.requireEnum<ConditionWindow>("window", conditionWindowNames());
			result.windowActor = p_reader.requireEnum<ConditionUnitSet>("windowActor", conditionUnitSetNames());
			result.requiredWindowCount = static_cast<std::uint32_t>(
				requireIntegerInRange(p_reader, "requiredWindowCount", 1, MaximumRequiredWindowCount));
			result.windowMode = p_reader.requireEnum<WindowCountMode>("windowMode", windowCountModeNames());

			const bool longWindow =
				result.window == ConditionWindow::Fight || result.window == ConditionWindow::Game;
			if (longWindow && result.windowActor != ConditionUnitSet::Any)
			{
				throw JsonError(
					p_reader.file(),
					p_reader.pathFor("windowActor"),
					"a fight or game window encloses everything and has no opening action to filter, so it requires "
					"\"any\"");
			}

			// A game window is one continuous persisted window, so it can neither be counted
			// several times nor broken into a streak.
			if (result.window == ConditionWindow::Game)
			{
				if (result.requiredWindowCount != 1)
				{
					throw JsonError(
						p_reader.file(),
						p_reader.pathFor("requiredWindowCount"),
						"a game window is a single continuous window, so requiredWindowCount must be 1");
				}
				if (result.windowMode != WindowCountMode::Cumulative)
				{
					throw JsonError(
						p_reader.file(),
						p_reader.pathFor("windowMode"),
						"a game window has no neighbouring window to form a streak with, so windowMode must be "
						"\"cumulative\"");
				}
			}
			return result;
		}

		// Every aggregation is meaningful on an amount-producing event: count ignores the amount,
		// sum totals it, maximum and minimum select one event's effective value. A leaf whose
		// events carry no amount narrows the set and says why.
		constexpr std::initializer_list<ConditionAggregation> AllAggregations{
			ConditionAggregation::Count,
			ConditionAggregation::Sum,
			ConditionAggregation::Maximum,
			ConditionAggregation::Minimum};

		constexpr std::initializer_list<ConditionAggregation> CountOnly{ConditionAggregation::Count};

		MetricTest parseMetric(
			const JsonReader &p_reader,
			std::initializer_list<ConditionAggregation> p_allowed = AllAggregations,
			std::string_view p_rejection = {})
		{
			MetricTest result;
			result.aggregation = p_reader.requireEnum<ConditionAggregation>("aggregation", conditionAggregationNames());
			if (std::ranges::find(p_allowed, result.aggregation) == p_allowed.end())
			{
				throw JsonError(p_reader.file(), p_reader.pathFor("aggregation"), std::string(p_rejection));
			}

			result.comparison = p_reader.requireEnum<ConditionComparison>("comparison", conditionComparisonNames());
			result.threshold = requireIntegerInRange(p_reader, "threshold", 0, MaximumConditionAmount);

			if (result.comparison == ConditionComparison::Between)
			{
				const std::int64_t maximum =
					requireIntegerInRange(p_reader, "maximum", 0, MaximumConditionAmount);
				if (maximum < result.threshold)
				{
					throw JsonError(
						p_reader.file(),
						p_reader.pathFor("maximum"),
						"'between' reads threshold as its lower bound, so maximum may not be smaller");
				}
				result.maximum = maximum;
			}
			else if (p_reader.contains("maximum"))
			{
				throw JsonError(
					p_reader.file(),
					p_reader.pathFor("maximum"),
					"'maximum' is the upper bound of a 'between' comparison and belongs to no other");
			}
			return result;
		}

		// A resource leaf always states why the pool changed. Without the filter, an authored
		// "gain AP from an effect" requirement would be satisfied by every ordinary activation
		// refill, so an incompatible reason is an error rather than a never-matching filter.
		std::vector<ResourceChangeReason> requireResourceReasons(
			const JsonReader &p_reader,
			ResourceConditionAction p_action,
			const std::string &p_actionName)
		{
			const std::vector<std::string> authored = p_reader.require<std::vector<std::string>>("reasons");
			if (authored.empty())
			{
				throw JsonError(
					p_reader.file(),
					p_reader.pathFor("reasons"),
					"a resource condition states why the pool changed, so it needs at least one reason");
			}

			const std::map<std::string, ResourceChangeReason> &names = resourceChangeReasonNames();

			std::vector<ResourceChangeReason> result;
			result.reserve(authored.size());
			for (const std::string &name : authored)
			{
				const auto entry = names.find(name);
				if (entry == names.end())
				{
					std::string known;
					for (const auto &[candidate, unused] : names)
					{
						(void)unused;
						if (!known.empty())
						{
							known += ", ";
						}
						known += candidate;
					}
					throw JsonError(
						p_reader.file(),
						p_reader.pathFor("reasons"),
						"unknown resource change reason '" + name + "' (expected one of: " + known + ")");
				}
				if (std::ranges::find(result, entry->second) != result.end())
				{
					throw JsonError(
						p_reader.file(),
						p_reader.pathFor("reasons"),
						"duplicate resource change reason '" + name + "'");
				}
				if (!isResourceReasonCompatible(p_action, entry->second))
				{
					throw JsonError(
						p_reader.file(),
						p_reader.pathFor("reasons"),
						"a '" + p_actionName + "' resource condition is never caused by '" + name + "'");
				}
				result.push_back(entry->second);
			}
			return result;
		}

		EventAbsencePredicate parseAbsencePredicate(JsonReader &p_reader)
		{
			const AbsenceKind kind = p_reader.requireEnum<AbsenceKind>("type", absenceKindNames());

			switch (kind)
			{
			case AbsenceKind::Damage:
			{
				p_reader.forbidUnknown({"type", "actor", "role", "counterpart", "kind", "component", "sourceAbilities"});

				DamageAbsencePredicate predicate;
				predicate.actor = p_reader.requireEnum<ConditionUnitSet>("actor", conditionUnitSetNames());
				predicate.role = p_reader.requireEnum<ConditionEventRole>("role", conditionEventRoleNames());
				predicate.counterpart = p_reader.requireEnum<ConditionUnitSet>("counterpart", conditionUnitSetNames());
				predicate.kind = p_reader.requireEnum<DamageKindFilter>("kind", damageKindFilterNames());
				predicate.component = p_reader.requireEnum<DamageComponent>("component", damageComponentNames());
				predicate.sourceAbilities = requireIdFilter(p_reader, "sourceAbilities", "ability id");
				return predicate;
			}
			case AbsenceKind::AbilityCast:
			{
				p_reader.forbidUnknown({"type", "actor", "abilities"});

				AbilityCastAbsencePredicate predicate;
				predicate.actor = p_reader.requireEnum<ConditionUnitSet>("actor", conditionUnitSetNames());
				predicate.abilities = requireIdFilter(p_reader, "abilities", "ability id");
				return predicate;
			}
			case AbsenceKind::Status:
			{
				p_reader.forbidUnknown({"type", "actor", "role", "counterpart", "action", "statuses", "tags"});

				StatusAbsencePredicate predicate;
				predicate.actor = p_reader.requireEnum<ConditionUnitSet>("actor", conditionUnitSetNames());
				predicate.role = p_reader.requireEnum<ConditionEventRole>("role", conditionEventRoleNames());
				predicate.counterpart = p_reader.requireEnum<ConditionUnitSet>("counterpart", conditionUnitSetNames());
				predicate.action = p_reader.requireEnum<StatusConditionAction>("action", statusConditionActionNames());
				predicate.statuses = requireIdFilter(p_reader, "statuses", "status id");
				predicate.tags = requireIdFilter(p_reader, "tags", "tag");
				return predicate;
			}
			case AbsenceKind::Movement:
			{
				p_reader.forbidUnknown({"type", "actor", "movement"});

				MovementAbsencePredicate predicate;
				predicate.actor = p_reader.requireEnum<ConditionUnitSet>("actor", conditionUnitSetNames());
				predicate.movement = p_reader.requireEnum<MovementFilter>("movement", movementFilterNames());
				return predicate;
			}
			}

			throw JsonError(p_reader.file(), p_reader.path(), "unhandled absence predicate type");
		}

		[[nodiscard]] std::vector<ConditionSpec> parseChildren(
			const JsonReader &p_reader,
			ConditionParseState &p_state,
			std::size_t p_depth);

		ConditionPayload parsePayload(
			JsonReader &p_reader,
			ConditionKind p_kind,
			ConditionParseState &p_state,
			std::size_t p_depth)
		{
			switch (p_kind)
			{
			case ConditionKind::Damage:
			{
				p_reader.forbidUnknown(
					{"id",
					 "type",
					 "descriptionKey",
					 "window",
					 "windowActor",
					 "requiredWindowCount",
					 "windowMode",
					 "actor",
					 "role",
					 "counterpart",
					 "kind",
					 "component",
					 "targetHadShield",
					 "sourceAbilities",
					 "aggregation",
					 "comparison",
					 "threshold",
					 "maximum"});

				DamageConditionSpec spec;
				spec.leaf = parseLeafHeader(p_reader);
				spec.actor = p_reader.requireEnum<ConditionUnitSet>("actor", conditionUnitSetNames());
				spec.role = p_reader.requireEnum<ConditionEventRole>("role", conditionEventRoleNames());
				spec.counterpart = p_reader.requireEnum<ConditionUnitSet>("counterpart", conditionUnitSetNames());
				spec.kind = p_reader.requireEnum<DamageKindFilter>("kind", damageKindFilterNames());
				spec.component = p_reader.requireEnum<DamageComponent>("component", damageComponentNames());
				spec.targetHadShield = p_reader.requireEnum<PresenceFilter>("targetHadShield", presenceFilterNames());
				spec.sourceAbilities = requireIdFilter(p_reader, "sourceAbilities", "ability id");
				spec.metric = parseMetric(p_reader);
				return spec;
			}
			case ConditionKind::Healing:
			{
				p_reader.forbidUnknown(
					{"id",
					 "type",
					 "descriptionKey",
					 "window",
					 "windowActor",
					 "requiredWindowCount",
					 "windowMode",
					 "actor",
					 "role",
					 "counterpart",
					 "sourceAbilities",
					 "aggregation",
					 "comparison",
					 "threshold",
					 "maximum"});

				HealingConditionSpec spec;
				spec.leaf = parseLeafHeader(p_reader);
				spec.actor = p_reader.requireEnum<ConditionUnitSet>("actor", conditionUnitSetNames());
				spec.role = p_reader.requireEnum<ConditionEventRole>("role", conditionEventRoleNames());
				spec.counterpart = p_reader.requireEnum<ConditionUnitSet>("counterpart", conditionUnitSetNames());
				spec.sourceAbilities = requireIdFilter(p_reader, "sourceAbilities", "ability id");
				spec.metric = parseMetric(p_reader);
				return spec;
			}
			case ConditionKind::AbilityCast:
			{
				p_reader.forbidUnknown(
					{"id",
					 "type",
					 "descriptionKey",
					 "window",
					 "windowActor",
					 "requiredWindowCount",
					 "windowMode",
					 "actor",
					 "abilities",
					 "sameAbility",
					 "affected",
					 "minimumAffectedUnits",
					 "minimumTargetDistance",
					 "maximumTargetDistance",
					 "comparison",
					 "threshold"});

				AbilityCastConditionSpec spec;
				spec.leaf = parseLeafHeader(p_reader);
				spec.actor = p_reader.requireEnum<ConditionUnitSet>("actor", conditionUnitSetNames());
				spec.abilities = requireIdFilter(p_reader, "abilities", "ability id");
				spec.sameAbility = p_reader.require<bool>("sameAbility");
				spec.affected = p_reader.requireEnum<ConditionUnitSet>("affected", conditionUnitSetNames());

				const std::int64_t affectedCap = 2 * static_cast<std::int64_t>(p_state.limits.teamCapacity);
				spec.minimumAffectedUnits = requireCount(p_reader, "minimumAffectedUnits", affectedCap);
				// A filtered set with a zero minimum would silently accept a cast that affected
				// nobody in it; "any" plus zero is the authored way to disable the filter.
				if (spec.affected != ConditionUnitSet::Any && spec.minimumAffectedUnits == 0)
				{
					throw JsonError(
						p_reader.file(),
						p_reader.pathFor("minimumAffectedUnits"),
						"an affected-unit filter selects nothing with a minimum of zero; author at least 1, or set "
						"affected to \"any\"");
				}

				spec.minimumTargetDistance =
					requireCount(p_reader, "minimumTargetDistance", MaximumConditionDistance);
				spec.maximumTargetDistance =
					requireCount(p_reader, "maximumTargetDistance", MaximumConditionDistance);
				if (spec.maximumTargetDistance < spec.minimumTargetDistance)
				{
					throw JsonError(
						p_reader.file(),
						p_reader.pathFor("maximumTargetDistance"),
						"the cast distance band is inclusive, so its maximum may not be below its minimum");
				}

				spec.comparison = p_reader.requireEnum<ConditionComparison>("comparison", conditionComparisonNames());
				forbidBetween(p_reader, spec.comparison);
				spec.threshold = requireIntegerInRange(p_reader, "threshold", 0, MaximumConditionAmount);
				return spec;
			}
			case ConditionKind::Status:
			{
				p_reader.forbidUnknown(
					{"id",
					 "type",
					 "descriptionKey",
					 "window",
					 "windowActor",
					 "requiredWindowCount",
					 "windowMode",
					 "actor",
					 "role",
					 "counterpart",
					 "action",
					 "statuses",
					 "tags",
					 "aggregation",
					 "comparison",
					 "threshold",
					 "maximum"});

				StatusConditionSpec spec;
				spec.leaf = parseLeafHeader(p_reader);
				spec.actor = p_reader.requireEnum<ConditionUnitSet>("actor", conditionUnitSetNames());
				spec.role = p_reader.requireEnum<ConditionEventRole>("role", conditionEventRoleNames());
				spec.counterpart = p_reader.requireEnum<ConditionUnitSet>("counterpart", conditionUnitSetNames());
				spec.action = p_reader.requireEnum<StatusConditionAction>("action", statusConditionActionNames());
				spec.statuses = requireIdFilter(p_reader, "statuses", "status id");
				spec.tags = requireIdFilter(p_reader, "tags", "tag");
				spec.metric = parseMetric(p_reader);
				return spec;
			}
			case ConditionKind::Shield:
			{
				p_reader.forbidUnknown(
					{"id",
					 "type",
					 "descriptionKey",
					 "window",
					 "windowActor",
					 "requiredWindowCount",
					 "windowMode",
					 "actor",
					 "role",
					 "counterpart",
					 "action",
					 "kind",
					 "aggregation",
					 "comparison",
					 "threshold",
					 "maximum"});

				ShieldConditionSpec spec;
				spec.leaf = parseLeafHeader(p_reader);
				spec.actor = p_reader.requireEnum<ConditionUnitSet>("actor", conditionUnitSetNames());
				spec.role = p_reader.requireEnum<ConditionEventRole>("role", conditionEventRoleNames());
				spec.counterpart = p_reader.requireEnum<ConditionUnitSet>("counterpart", conditionUnitSetNames());
				spec.action = p_reader.requireEnum<ShieldConditionAction>("action", shieldConditionActionNames());
				spec.kind = p_reader.requireEnum<DamageKindFilter>("kind", damageKindFilterNames());

				// A break carries no amount: the shield is simply gone, so only its occurrences
				// can be counted.
				const bool broken = spec.action == ShieldConditionAction::Broken;
				spec.metric = parseMetric(
					p_reader,
					broken ? CountOnly : AllAggregations,
					"a broken shield reports no amount, so 'broken' can only be counted");
				return spec;
			}
			case ConditionKind::Movement:
			{
				p_reader.forbidUnknown(
					{"id",
					 "type",
					 "descriptionKey",
					 "window",
					 "windowActor",
					 "requiredWindowCount",
					 "windowMode",
					 "actor",
					 "movement",
					 "direction",
					 "aggregation",
					 "comparison",
					 "threshold",
					 "maximum"});

				MovementConditionSpec spec;
				spec.leaf = parseLeafHeader(p_reader);
				spec.actor = p_reader.requireEnum<ConditionUnitSet>("actor", conditionUnitSetNames());
				spec.movement = p_reader.requireEnum<MovementFilter>("movement", movementFilterNames());
				spec.direction = p_reader.requireEnum<MovementDirection>("direction", movementDirectionNames());

				// Only a displacement is authored relative to a source; a voluntary path or a
				// teleport has no such direction to compare against.
				const bool directional = spec.direction != MovementDirection::Any;
				const bool carriesDirection =
					spec.movement == MovementFilter::Displacement || spec.movement == MovementFilter::Any;
				if (directional && !carriesDirection)
				{
					throw JsonError(
						p_reader.file(),
						p_reader.pathFor("direction"),
						"only a displacement carries a direction relative to its source, so this movement kind "
						"requires \"any\"");
				}

				spec.metric = parseMetric(p_reader);
				return spec;
			}
			case ConditionKind::Resource:
			{
				p_reader.forbidUnknown(
					{"id",
					 "type",
					 "descriptionKey",
					 "window",
					 "windowActor",
					 "requiredWindowCount",
					 "windowMode",
					 "actor",
					 "action",
					 "resource",
					 "reasons",
					 "aggregation",
					 "comparison",
					 "threshold",
					 "maximum"});

				ResourceConditionSpec spec;
				spec.leaf = parseLeafHeader(p_reader);
				spec.actor = p_reader.requireEnum<ConditionUnitSet>("actor", conditionUnitSetNames());
				const std::string action = p_reader.require<std::string>("action");
				spec.action = p_reader.requireEnum<ResourceConditionAction>("action", resourceConditionActionNames());
				spec.resource = p_reader.requireEnum<BattleResource>("resource", battleResourceNames());
				spec.reasons = requireResourceReasons(p_reader, spec.action, action);
				spec.metric = parseMetric(p_reader);
				return spec;
			}
			case ConditionKind::Position:
			{
				p_reader.forbidUnknown(
					{"id",
					 "type",
					 "descriptionKey",
					 "window",
					 "windowActor",
					 "requiredWindowCount",
					 "windowMode",
					 "sample",
					 "actor",
					 "relativeTo",
					 "comparison",
					 "distance",
					 "maximumDistance"});

				PositionConditionSpec spec;
				spec.leaf = parseLeafHeader(p_reader);
				spec.sample = p_reader.requireEnum<PositionSample>("sample", positionSampleNames());

				// A whole-window invariant needs a window that opens and closes; the persistent
				// game window never does.
				if (spec.sample == PositionSample::ThroughoutWindow && spec.leaf.window == ConditionWindow::Game)
				{
					throw JsonError(
						p_reader.file(),
						p_reader.pathFor("sample"),
						"'throughoutWindow' is an invariant over one opened window, so it cannot use the persistent "
						"game window");
				}

				spec.actor = p_reader.requireEnum<ConditionUnitSet>("actor", conditionUnitSetNames());
				spec.relativeTo = p_reader.requireEnum<ConditionUnitSet>("relativeTo", conditionUnitSetNames());
				spec.comparison = p_reader.requireEnum<ConditionComparison>("comparison", conditionComparisonNames());

				const bool comparable = spec.comparison == ConditionComparison::AtMost ||
										spec.comparison == ConditionComparison::AtLeast ||
										spec.comparison == ConditionComparison::Equal ||
										spec.comparison == ConditionComparison::Between;
				if (!comparable)
				{
					throw JsonError(
						p_reader.file(),
						p_reader.pathFor("comparison"),
						"a cell distance is compared with 'atMost', 'atLeast', 'equal' or 'between'");
				}

				spec.distance = requireCount(p_reader, "distance", MaximumConditionDistance);
				if (spec.comparison == ConditionComparison::Between)
				{
					const std::uint32_t maximum =
						requireCount(p_reader, "maximumDistance", MaximumConditionDistance);
					if (maximum < spec.distance)
					{
						throw JsonError(
							p_reader.file(),
							p_reader.pathFor("maximumDistance"),
							"'between' reads distance as its lower bound, so maximumDistance may not be smaller");
					}
					spec.maximumDistance = maximum;
				}
				else if (p_reader.contains("maximumDistance"))
				{
					throw JsonError(
						p_reader.file(),
						p_reader.pathFor("maximumDistance"),
						"'maximumDistance' is the upper bound of a 'between' comparison and belongs to no other");
				}
				return spec;
			}
			case ConditionKind::UnitRemoval:
			{
				p_reader.forbidUnknown(
					{"id",
					 "type",
					 "descriptionKey",
					 "window",
					 "windowActor",
					 "requiredWindowCount",
					 "windowMode",
					 "removed",
					 "creditedTo",
					 "reason",
					 "comparison",
					 "threshold"});

				UnitRemovalConditionSpec spec;
				spec.leaf = parseLeafHeader(p_reader);
				spec.removed = p_reader.requireEnum<ConditionUnitSet>("removed", conditionUnitSetNames());
				spec.creditedTo = p_reader.requireEnum<ConditionUnitSet>("creditedTo", conditionUnitSetNames());
				spec.reason = p_reader.requireEnum<UnitRemovalReasonFilter>("reason", unitRemovalReasonFilterNames());
				spec.comparison = p_reader.requireEnum<ConditionComparison>("comparison", conditionComparisonNames());
				forbidBetween(p_reader, spec.comparison);
				spec.threshold = requireIntegerInRange(p_reader, "threshold", 0, MaximumConditionAmount);
				return spec;
			}
			case ConditionKind::BattleOutcome:
			{
				p_reader.forbidUnknown(
					{"id",
					 "type",
					 "descriptionKey",
					 "window",
					 "windowActor",
					 "requiredWindowCount",
					 "windowMode",
					 "outcome",
					 "requireSubjectActive"});

				BattleOutcomeConditionSpec spec;
				spec.leaf = parseLeafHeader(p_reader);
				requireWindow(p_reader, spec.leaf, ConditionWindow::Fight);
				spec.outcome = p_reader.requireEnum<RelativeBattleOutcome>("outcome", relativeBattleOutcomeNames());
				spec.requireSubjectActive = p_reader.require<bool>("requireSubjectActive");
				return spec;
			}
			case ConditionKind::SurvivorCount:
			{
				p_reader.forbidUnknown(
					{"id",
					 "type",
					 "descriptionKey",
					 "window",
					 "windowActor",
					 "requiredWindowCount",
					 "windowMode",
					 "units",
					 "state",
					 "comparison",
					 "threshold"});

				SurvivorCountConditionSpec spec;
				spec.leaf = parseLeafHeader(p_reader);
				requireWindow(p_reader, spec.leaf, ConditionWindow::Fight);
				spec.units = p_reader.requireEnum<ConditionUnitSet>("units", conditionUnitSetNames());
				spec.state = p_reader.requireEnum<SurvivorState>("state", survivorStateNames());
				spec.comparison = p_reader.requireEnum<ConditionComparison>("comparison", conditionComparisonNames());
				forbidBetween(p_reader, spec.comparison);
				// Both teams together, so a survivor count can never demand more units than the
				// board can hold.
				spec.threshold = requireCount(
					p_reader,
					"threshold",
					2 * static_cast<std::int64_t>(p_state.limits.teamCapacity));
				return spec;
			}
			case ConditionKind::EventAbsence:
			{
				p_reader.forbidUnknown(
					{"id",
					 "type",
					 "descriptionKey",
					 "window",
					 "windowActor",
					 "requiredWindowCount",
					 "windowMode",
					 "predicate"});

				EventAbsenceConditionSpec spec;
				spec.leaf = parseLeafHeader(p_reader);

				JsonReader predicateReader = p_reader.child("predicate");
				spec.predicate = parseAbsencePredicate(predicateReader);
				return spec;
			}
			case ConditionKind::AllOf:
			{
				p_reader.forbidUnknown({"id", "type", "descriptionKey", "children"});

				AllOfConditionSpec spec;
				spec.children = parseChildren(p_reader, p_state, p_depth);
				return spec;
			}
			case ConditionKind::AnyOf:
			{
				p_reader.forbidUnknown({"id", "type", "descriptionKey", "children"});

				AnyOfConditionSpec spec;
				spec.children = parseChildren(p_reader, p_state, p_depth);
				return spec;
			}
			}

			throw JsonError(p_reader.file(), p_reader.path(), "unhandled condition type");
		}

		std::vector<ConditionSpec> parseChildren(
			const JsonReader &p_reader,
			ConditionParseState &p_state,
			std::size_t p_depth)
		{
			std::vector<JsonReader> entries = p_reader.childArray("children");
			// A composite of one is the child itself, and a composite of none completes on
			// nothing: both are authoring mistakes rather than shorthand.
			if (entries.size() < 2)
			{
				throw JsonError(
					p_reader.file(),
					p_reader.pathFor("children"),
					"a composite condition combines at least two children");
			}

			std::vector<ConditionSpec> result;
			result.reserve(entries.size());
			for (JsonReader &entry : entries)
			{
				result.push_back(parseConditionSpec(entry, p_state, p_depth + 1));
			}
			return result;
		}
	}

	ConditionSpec parseConditionSpec(JsonReader &p_reader, ConditionParseState &p_state, std::size_t p_depth)
	{
		if (p_depth > p_state.limits.maxDepth)
		{
			throw JsonError(
				p_reader.file(),
				p_reader.path(),
				"condition nesting is deeper than gameRules.battle.maxConditionDepth (" +
					std::to_string(p_state.limits.maxDepth) + ")");
		}

		// The type is read first: it selects the alternative, and only the selected parser knows
		// which fields the object may carry.
		const ConditionKind kind = p_reader.requireEnum<ConditionKind>("type", conditionKindNames());

		ConditionSpec result;
		result.id = p_reader.require<std::string>("id");
		requireContentId(result.id, p_reader.file(), p_reader.pathFor("id"), "condition id");
		// Unique across the entire owning board or profile, not merely among siblings: this is
		// the key persisted progress and diagnostics use once an array is reordered.
		if (!p_state.ids.insert(result.id).second)
		{
			throw JsonError(
				p_reader.file(),
				p_reader.pathFor("id"),
				"duplicate condition id '" + result.id + "' in this definition");
		}
		result.descriptionKey = requireDescriptionKey(p_reader);
		result.source = DefinitionSource{p_reader.file(), p_reader.path()};
		result.payload = parsePayload(p_reader, kind, p_state, p_depth);
		return result;
	}

	std::vector<ConditionSpec> parseConditions(
		const JsonReader &p_reader,
		const std::string &p_key,
		ConditionParseState &p_state,
		bool p_allowEmpty)
	{
		std::vector<JsonReader> entries = p_reader.childArray(p_key);
		if (entries.empty() && !p_allowEmpty)
		{
			throw JsonError(p_reader.file(), p_reader.pathFor(p_key), "expected at least one condition");
		}

		std::vector<ConditionSpec> result;
		result.reserve(entries.size());
		for (JsonReader &entry : entries)
		{
			result.push_back(parseConditionSpec(entry, p_state, 1));
		}
		return result;
	}
}
