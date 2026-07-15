#pragma once

#include "battle/battle_types.hpp"
#include "core/definition_fields.hpp"
#include "core/game_rules.hpp"
#include "core/json.hpp"

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <variant>
#include <vector>

namespace pg
{
	// One condition specification serves both progression domains. A Feat Requirement and a
	// taming requirement are the same authored tree; only the runtime context that binds the
	// subject differs, so there is no second evaluator and no parallel schema.
	//
	// Definition layer only: nothing here evaluates an event, holds advancement or persists.
	// Steps 15-17 own that. What this file locks is the vocabulary those steps must speak.

	// A condition is always read relative to one bound subject, never to an absolute side. The
	// same tree therefore means "the owner heals an ally" on a Feat Board and "the player team
	// damages this wild creature" in a taming profile:
	//
	//     Feat    subject = the deployed persistent creature, subjectTeam = Player
	//     Taming  subject = the tracked wild unit,            subjectTeam = Enemy
	//
	// subjectAllies excludes the subject; subjectTeam includes it.
	enum class ConditionUnitSet
	{
		Subject,
		SubjectAllies,
		SubjectTeam,
		OpponentTeam,
		Any
	};

	// Which side of a source-target event the actor must occupy.
	enum class ConditionEventRole
	{
		Source,
		Target,
		Either
	};

	// The span a leaf's metric is computed over. Stable window identities are
	// (BattleId, BattleActionId), (BattleId, TurnIndex), BattleId, and the persistent
	// condition owner's key respectively.
	//
	//     ability  one successfully committed effect batch initiated by one ability
	//     turn     one unit activation, ActivationStarted through ActivationEnded
	//     fight    one terminal, non-aborted battle
	//     game     lifetime accumulation across committed battles - for taming, reset at
	//              encounter construction, so it means the current encounter
	enum class ConditionWindow
	{
		Ability,
		Turn,
		Fight,
		Game
	};

	// How qualifying windows accumulate. A qualifying window counts once, never once per event.
	enum class WindowCountMode
	{
		Cumulative,
		Consecutive
	};

	enum class ConditionAggregation
	{
		Count,
		Sum,
		Maximum,
		Minimum
	};

	enum class ConditionComparison
	{
		AtLeast,
		GreaterThan,
		AtMost,
		LessThan,
		Equal,
		Between
	};

	enum class DamageKindFilter
	{
		Any,
		Physical,
		Magical
	};

	enum class DamageComponent
	{
		Total,
		Health,
		Shield
	};

	enum class PresenceFilter
	{
		Any,
		Required,
		Forbidden
	};

	enum class StatusConditionAction
	{
		Applied,
		Removed
	};

	enum class ShieldConditionAction
	{
		Applied,
		Absorbed,
		Broken
	};

	enum class MovementFilter
	{
		Voluntary,
		Displacement,
		Teleport,
		Any
	};

	enum class MovementDirection
	{
		Any,
		TowardSource,
		AwayFromSource
	};

	enum class ResourceConditionAction
	{
		Spent,
		Gained,
		Lost,
		NextActivationPenalty
	};

	// Why a resource pool changed. The filter is required so that an authored "gain AP from an
	// effect" requirement cannot be farmed by the ordinary activation refill.
	enum class PositionSample
	{
		TurnStart,
		TurnEnd,
		AfterMove,
		ThroughoutWindow
	};

	enum class UnitRemovalReasonFilter
	{
		Defeated,
		Impressed,
		Any
	};

	// The outcome from the subject's side, so a wild creature's profile and a player creature's
	// Feat read the same words.
	enum class RelativeBattleOutcome
	{
		SubjectTeamVictory,
		OpponentTeamVictory,
		Draw,
		Completed
	};

	enum class SurvivorState
	{
		Active,
		NotDefeated
	};

	// The window header every leaf carries. The initializers only make aggregate construction
	// deterministic in C++: the parser requires all four keys and diagnoses a missing one
	// rather than accepting an initializer as a schema default.
	//
	// windowActor selects which action or activation opens an eligible short window: for
	// ability, the committed cast's source must belong to it; for turn, the activation owner
	// must. fight and game enclose everything and therefore require "any".
	struct ConditionLeafHeader
	{
		ConditionWindow window = ConditionWindow::Fight;
		ConditionUnitSet windowActor = ConditionUnitSet::Any;
		std::uint32_t requiredWindowCount = 1;
		WindowCountMode windowMode = WindowCountMode::Cumulative;
	};

	// The metric an amount-producing leaf computes inside one window, and the test it must
	// pass for that window to qualify. Sums use effective, post-mitigation, post-clamp values -
	// never a requested amount. maximum is present only for "between".
	struct MetricTest
	{
		ConditionAggregation aggregation = ConditionAggregation::Count;
		ConditionComparison comparison = ConditionComparison::AtLeast;
		std::int64_t threshold = 0;
		std::optional<std::int64_t> maximum;
	};

	struct DamageConditionSpec
	{
		ConditionLeafHeader leaf;
		ConditionUnitSet actor = ConditionUnitSet::Any;
		ConditionEventRole role = ConditionEventRole::Either;
		ConditionUnitSet counterpart = ConditionUnitSet::Any;
		DamageKindFilter kind = DamageKindFilter::Any;
		DamageComponent component = DamageComponent::Total;
		// Reads the event's pre-hit targetHadAnyShield fact, whatever the damage channel.
		PresenceFilter targetHadShield = PresenceFilter::Any;
		std::vector<std::string> sourceAbilities;
		MetricTest metric;
	};

	struct HealingConditionSpec
	{
		ConditionLeafHeader leaf;
		ConditionUnitSet actor = ConditionUnitSet::Any;
		ConditionEventRole role = ConditionEventRole::Either;
		ConditionUnitSet counterpart = ConditionUnitSet::Any;
		std::vector<std::string> sourceAbilities;
		// Effective health restored, after the max-HP clamp: overheal contributes zero.
		MetricTest metric;
	};

	// Consumes successful casts only. sameAbility keeps deterministic per-ability buckets and
	// qualifies when one bucket meets the comparison; otherwise every filtered cast counts.
	//
	// affected/minimumAffectedUnits filter the cast's already captured affectedUnits, which is
	// what lets "get hit by three attacks" count one cast window instead of each nested damage
	// effect.
	struct AbilityCastConditionSpec
	{
		ConditionLeafHeader leaf;
		ConditionUnitSet actor = ConditionUnitSet::Any;
		std::vector<std::string> abilities;
		bool sameAbility = false;
		ConditionUnitSet affected = ConditionUnitSet::Any;
		std::uint32_t minimumAffectedUnits = 0;
		std::uint32_t minimumTargetDistance = 0;
		std::uint32_t maximumTargetDistance = 0;
		ConditionComparison comparison = ConditionComparison::AtLeast;
		std::int64_t threshold = 0;
	};

	struct StatusConditionSpec
	{
		ConditionLeafHeader leaf;
		ConditionUnitSet actor = ConditionUnitSet::Any;
		ConditionEventRole role = ConditionEventRole::Either;
		ConditionUnitSet counterpart = ConditionUnitSet::Any;
		StatusConditionAction action = StatusConditionAction::Applied;
		std::vector<std::string> statuses;
		std::vector<std::string> tags;
		// count counts status events; sum adds the actual stack deltas.
		MetricTest metric;
	};

	struct ShieldConditionSpec
	{
		ConditionLeafHeader leaf;
		ConditionUnitSet actor = ConditionUnitSet::Any;
		ConditionEventRole role = ConditionEventRole::Either;
		ConditionUnitSet counterpart = ConditionUnitSet::Any;
		ShieldConditionAction action = ShieldConditionAction::Applied;
		DamageKindFilter kind = DamageKindFilter::Any;
		MetricTest metric;
	};

	struct MovementConditionSpec
	{
		ConditionLeafHeader leaf;
		ConditionUnitSet actor = ConditionUnitSet::Any;
		MovementFilter movement = MovementFilter::Any;
		MovementDirection direction = MovementDirection::Any;
		// Actual x/z cell distance: one multi-cell voluntary path is one event carrying its
		// entered-cell count.
		MetricTest metric;
	};

	struct ResourceConditionSpec
	{
		ConditionLeafHeader leaf;
		ConditionUnitSet actor = ConditionUnitSet::Any;
		ResourceConditionAction action = ResourceConditionAction::Spent;
		BattleResource resource = BattleResource::ActionPoints;
		std::vector<ResourceChangeReason> reasons;
		MetricTest metric;
	};

	// Distance is the minimum Manhattan x/z distance from the sampled actor to any active
	// member of relativeTo. throughoutWindow makes the comparison a whole-window invariant:
	// one failed sample irreversibly fails that window.
	struct PositionConditionSpec
	{
		ConditionLeafHeader leaf;
		PositionSample sample = PositionSample::TurnStart;
		ConditionUnitSet actor = ConditionUnitSet::Any;
		ConditionUnitSet relativeTo = ConditionUnitSet::Any;
		ConditionComparison comparison = ConditionComparison::AtMost;
		std::uint32_t distance = 0;
		std::optional<std::uint32_t> maximumDistance;
	};

	// Aggregation is implicitly count. RemovalReason::Impressed never satisfies "defeated", so
	// a taming impression can never pay a kill Feat.
	struct UnitRemovalConditionSpec
	{
		ConditionLeafHeader leaf;
		ConditionUnitSet removed = ConditionUnitSet::Any;
		ConditionUnitSet creditedTo = ConditionUnitSet::Any;
		UnitRemovalReasonFilter reason = UnitRemovalReasonFilter::Any;
		ConditionComparison comparison = ConditionComparison::AtLeast;
		std::int64_t threshold = 0;
	};

	// Fight window only: only a terminal, non-aborted BattleEnded qualifies.
	struct BattleOutcomeConditionSpec
	{
		ConditionLeafHeader leaf;
		RelativeBattleOutcome outcome = RelativeBattleOutcome::Completed;
		bool requireSubjectActive = false;
	};

	// Fight window only, read from the terminal snapshot. active excludes defeated and removed
	// units; notDefeated keeps impressed ones, for summary conditions.
	struct SurvivorCountConditionSpec
	{
		ConditionLeafHeader leaf;
		ConditionUnitSet units = ConditionUnitSet::Any;
		SurvivorState state = SurvivorState::Active;
		ConditionComparison comparison = ConditionComparison::AtLeast;
		std::uint32_t threshold = 0;
	};

	struct DamageAbsencePredicate
	{
		ConditionUnitSet actor = ConditionUnitSet::Any;
		ConditionEventRole role = ConditionEventRole::Either;
		ConditionUnitSet counterpart = ConditionUnitSet::Any;
		DamageKindFilter kind = DamageKindFilter::Any;
		DamageComponent component = DamageComponent::Total;
		std::vector<std::string> sourceAbilities;
	};

	struct AbilityCastAbsencePredicate
	{
		ConditionUnitSet actor = ConditionUnitSet::Any;
		std::vector<std::string> abilities;
	};

	struct StatusAbsencePredicate
	{
		ConditionUnitSet actor = ConditionUnitSet::Any;
		ConditionEventRole role = ConditionEventRole::Either;
		ConditionUnitSet counterpart = ConditionUnitSet::Any;
		StatusConditionAction action = StatusConditionAction::Applied;
		std::vector<std::string> statuses;
		std::vector<std::string> tags;
	};

	struct MovementAbsencePredicate
	{
		ConditionUnitSet actor = ConditionUnitSet::Any;
		MovementFilter movement = MovementFilter::Any;
	};

	// A second closed variant, holding filters and nothing else: an absence predicate can never
	// carry a window, a metric or a child condition. This is deliberately not a general
	// negation operator.
	using EventAbsencePredicate = std::variant<
		DamageAbsencePredicate,
		AbilityCastAbsencePredicate,
		StatusAbsencePredicate,
		MovementAbsencePredicate>;

	// Qualifies when no committed event matching the predicate occurs inside an eligible
	// window. With windowMode "consecutive" this is the streak condition ("take no physical
	// damage for two turns in a row").
	struct EventAbsenceConditionSpec
	{
		ConditionLeafHeader leaf;
		EventAbsencePredicate predicate;
	};

	struct ConditionSpec;

	// The composites carry children and no window of their own: they complete when all / any of
	// their children complete, each child according to its own window.
	struct AllOfConditionSpec
	{
		std::vector<ConditionSpec> children;
	};

	struct AnyOfConditionSpec
	{
		std::vector<ConditionSpec> children;
	};

	// The closed set of condition alternatives. The JSON "type" discriminator selects the
	// alternative and is not stored a second time, and there is no expression language or
	// script payload: authored data never carries executable content.
	using ConditionPayload = std::variant<
		DamageConditionSpec,
		HealingConditionSpec,
		AbilityCastConditionSpec,
		StatusConditionSpec,
		ShieldConditionSpec,
		MovementConditionSpec,
		ResourceConditionSpec,
		PositionConditionSpec,
		UnitRemovalConditionSpec,
		BattleOutcomeConditionSpec,
		SurvivorCountConditionSpec,
		EventAbsenceConditionSpec,
		AllOfConditionSpec,
		AnyOfConditionSpec>;

	// The sole owner of a condition's identity: no alternative repeats the id, the description
	// key or the discriminator. std::vector<ConditionSpec> is the owning indirection that makes
	// the recursion legal - deep value ownership, ordinary copy and move, no nullable children,
	// and an acyclic tree because JSON embeds its children rather than referencing them.
	//
	// The id is a step-01 content id, unique across the entire owning board or profile rather
	// than merely among siblings, so persisted progress and diagnostics keep a stable key when
	// an array is reordered.
	struct ConditionSpec
	{
		std::string id;
		// A translation key, not a sentence (see resources/data/locales).
		std::string descriptionKey;
		ConditionPayload payload;
		DefinitionSource source;
	};

	// Event amounts, counts and distances are checked signed 64-bit values; no progress is ever
	// a float percentage. The upper bound keeps lifetime accumulation practical.
	inline constexpr std::int64_t MaximumConditionAmount = 1000000000;
	inline constexpr std::int64_t MaximumConditionDistance = 128;
	inline constexpr std::int64_t MaximumRequiredWindowCount = 1000000;

	[[nodiscard]] const std::map<std::string, ConditionUnitSet> &conditionUnitSetNames();
	[[nodiscard]] const std::map<std::string, ConditionEventRole> &conditionEventRoleNames();
	[[nodiscard]] const std::map<std::string, ConditionWindow> &conditionWindowNames();
	[[nodiscard]] const std::map<std::string, WindowCountMode> &windowCountModeNames();
	[[nodiscard]] const std::map<std::string, ConditionAggregation> &conditionAggregationNames();
	[[nodiscard]] const std::map<std::string, ConditionComparison> &conditionComparisonNames();
	[[nodiscard]] const std::map<std::string, DamageKindFilter> &damageKindFilterNames();
	[[nodiscard]] const std::map<std::string, DamageComponent> &damageComponentNames();
	[[nodiscard]] const std::map<std::string, PresenceFilter> &presenceFilterNames();
	[[nodiscard]] const std::map<std::string, StatusConditionAction> &statusConditionActionNames();
	[[nodiscard]] const std::map<std::string, ShieldConditionAction> &shieldConditionActionNames();
	[[nodiscard]] const std::map<std::string, MovementFilter> &movementFilterNames();
	[[nodiscard]] const std::map<std::string, MovementDirection> &movementDirectionNames();
	[[nodiscard]] const std::map<std::string, ResourceConditionAction> &resourceConditionActionNames();
	[[nodiscard]] const std::map<std::string, ResourceChangeReason> &resourceChangeReasonNames();
	[[nodiscard]] const std::map<std::string, PositionSample> &positionSampleNames();
	[[nodiscard]] const std::map<std::string, UnitRemovalReasonFilter> &unitRemovalReasonFilterNames();
	[[nodiscard]] const std::map<std::string, RelativeBattleOutcome> &relativeBattleOutcomeNames();
	[[nodiscard]] const std::map<std::string, SurvivorState> &survivorStateNames();

	// Whether the reason may be authored with the action. Without this, "gain AP from an
	// effect" would be satisfied by every activation refill.
	[[nodiscard]] bool isResourceReasonCompatible(
		ResourceConditionAction p_action,
		ResourceChangeReason p_reason) noexcept;

	// The two authored limits condition parsing needs, so a Feat Board and a species' taming
	// profile are bounded by the same rules file.
	struct ConditionLimits
	{
		std::size_t maxDepth = 8;
		std::size_t teamCapacity = 6;
	};

	[[nodiscard]] ConditionLimits conditionLimits(const GameRules &p_rules) noexcept;

	// Ids are unique across the whole owning board or profile, so the accumulating set outlives
	// any one condition tree.
	struct ConditionParseState
	{
		ConditionLimits limits;
		std::set<std::string> ids;
	};

	// p_depth is 1 for a top-level condition. A tree deeper than the configured limit is
	// rejected while descending, which is also what keeps the recursion off the call stack's
	// limit.
	[[nodiscard]] ConditionSpec parseConditionSpec(
		JsonReader &p_reader,
		ConditionParseState &p_state,
		std::size_t p_depth = 1);

	[[nodiscard]] std::vector<ConditionSpec> parseConditions(
		const JsonReader &p_reader,
		const std::string &p_key,
		ConditionParseState &p_state,
		bool p_allowEmpty);

	// One authored reference to another definition, with the exact place it was authored.
	struct ConditionReference
	{
		std::string id;
		DefinitionSource source;
	};

	// Every ability and status id a condition tree names, in authored order. Collecting them is
	// separate from resolving them: the conditions layer knows nothing about registries, and
	// the cross-validation phase (core/combat_definition_validation.hpp) does the resolving.
	struct ConditionReferences
	{
		std::vector<ConditionReference> abilities;
		std::vector<ConditionReference> statuses;
	};

	void collectConditionReferences(
		const std::vector<ConditionSpec> &p_conditions,
		ConditionReferences &p_references);
}
