#pragma once

#include "battle/battle_types.hpp"
#include "core/definition_fields.hpp"
#include "core/json.hpp"

#include <cstdint>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace pg
{
	// An AI definition is intent, expressed as ordered data. It holds no callback, no script, no
	// expression language and no utility score, and it never touches the battle: step 11 evaluates
	// these rules against a snapshot and submits the same commands a player would, through the same
	// gateway. That is what keeps an enemy unable to do anything a player could not.
	//
	// Evaluation contract, locked here because the data is shaped for it: rules are tried in
	// authored order; a rule whose conditions all hold produces one command; if that command turns
	// out to be illegal, evaluation continues at the next rule; after a command is accepted,
	// evaluation restarts at rule zero. Selector ties resolve by encounter roster order, then by
	// BattleUnitId. "Nearest" is Manhattan distance on x/z. A health ratio is integer permille of
	// effective current over effective maximum HP - never a float.

	enum class AIUnitSelector
	{
		Self,
		NearestEnemy,
		NearestAlly,
		LowestHealthEnemy,
		LowestHealthAlly,
		HighestHealthEnemy,
		HighestHealthAlly
	};

	enum class AIInclusiveComparison
	{
		AtMost,
		AtLeast
	};

	enum class AIBestAreaPreference
	{
		Enemies,
		Allies
	};

	// An explicit empty value: the JSON type "always" owns no hidden state, and saying so in the
	// variant is what stops a later reader from inventing one.
	struct AIAlwaysCondition
	{
		[[nodiscard]] bool operator==(const AIAlwaysCondition &) const noexcept = default;
	};

	struct AIHealthRatioCondition
	{
		AIUnitSelector selector = AIUnitSelector::Self;
		AIInclusiveComparison comparison = AIInclusiveComparison::AtMost;
		std::uint16_t permille = 0;

		[[nodiscard]] bool operator==(const AIHealthRatioCondition &) const noexcept = default;
	};

	struct AIResourceCondition
	{
		BattleResource resource = BattleResource::ActionPoints;
		int amount = 0;

		[[nodiscard]] bool operator==(const AIResourceCondition &) const noexcept = default;
	};

	struct AIDistanceCondition
	{
		AIUnitSelector selector = AIUnitSelector::NearestEnemy;
		AIInclusiveComparison comparison = AIInclusiveComparison::AtMost;
		std::uint16_t cells = 0;

		[[nodiscard]] bool operator==(const AIDistanceCondition &) const noexcept = default;
	};

	struct AIStatusIdReference
	{
		std::string statusId;

		[[nodiscard]] bool operator==(const AIStatusIdReference &) const = default;
	};

	struct AIStatusTagReference
	{
		std::string tag;

		[[nodiscard]] bool operator==(const AIStatusTagReference &) const = default;
	};

	// One closed representation of the two status queries, so a condition can never carry both a
	// status id and a tag, or neither:
	//
	//     type "hasStatus"    + status -> AIStatusIdReference
	//     type "hasStatusTag" + tag    -> AIStatusTagReference
	using AIStatusReference = std::variant<AIStatusIdReference, AIStatusTagReference>;

	struct AIStatusCondition
	{
		AIUnitSelector selector = AIUnitSelector::Self;
		AIStatusReference reference = AIStatusIdReference{};
		bool present = true;

		[[nodiscard]] bool operator==(const AIStatusCondition &) const = default;
	};

	// Costs only. Whether a legal target exists belongs to decision planning and to the command
	// gateway, which is where a player's cast is checked too.
	struct AIAbilityAffordableCondition
	{
		std::string abilityId;

		[[nodiscard]] bool operator==(const AIAbilityAffordableCondition &) const = default;
	};

	using AIConditionSpec = std::variant<
		AIAlwaysCondition,
		AIHealthRatioCondition,
		AIResourceCondition,
		AIDistanceCondition,
		AIStatusCondition,
		AIAbilityAffordableCondition>;

	struct AIUnitAnchor
	{
		AIUnitSelector selector = AIUnitSelector::Self;

		[[nodiscard]] bool operator==(const AIUnitAnchor &) const noexcept = default;
	};

	// Another explicit empty value: the acting unit comes from the planner context, not from the
	// authored data.
	struct AISelfCellAnchor
	{
		[[nodiscard]] bool operator==(const AISelfCellAnchor &) const noexcept = default;
	};

	// Step 11 enumerates the legal anchors, maximises the affected units of the preferred side,
	// minimises the other side's, then breaks ties in canonical cell order. It never ignores the
	// ability's own affected filter.
	struct AIBestAreaAnchor
	{
		AIBestAreaPreference preferred = AIBestAreaPreference::Enemies;

		[[nodiscard]] bool operator==(const AIBestAreaAnchor &) const noexcept = default;
	};

	struct AINearestLegalCellToAnchor
	{
		AIUnitSelector selector = AIUnitSelector::NearestEnemy;

		[[nodiscard]] bool operator==(const AINearestLegalCellToAnchor &) const noexcept = default;
	};

	using AIAnchorSpec = std::variant<AIUnitAnchor, AISelfCellAnchor, AIBestAreaAnchor, AINearestLegalCellToAnchor>;

	struct AICastAbilityDecision
	{
		std::string abilityId;
		// Owned by value: never a borrowed JSON node, and never a pointer into a registry.
		AIAnchorSpec anchor = AISelfCellAnchor{};

		[[nodiscard]] bool operator==(const AICastAbilityDecision &) const = default;
	};

	// Zero means every movement point currently available; any other value caps the spend.
	struct AIMoveTowardDecision
	{
		AIUnitSelector selector = AIUnitSelector::NearestEnemy;
		std::uint32_t maximumMovementPoints = 0;

		[[nodiscard]] bool operator==(const AIMoveTowardDecision &) const noexcept = default;
	};

	struct AIMoveAwayDecision
	{
		AIUnitSelector selector = AIUnitSelector::NearestEnemy;
		std::uint32_t maximumMovementPoints = 0;

		[[nodiscard]] bool operator==(const AIMoveAwayDecision &) const noexcept = default;
	};

	struct AIEndTurnDecision
	{
		[[nodiscard]] bool operator==(const AIEndTurnDecision &) const noexcept = default;
	};

	using AIDecisionSpec =
		std::variant<AICastAbilityDecision, AIMoveTowardDecision, AIMoveAwayDecision, AIEndTurnDecision>;

	// An empty condition list is the AND identity: it means "always". "always" exists as an
	// explicit type for readability and may therefore not be combined with another condition.
	struct AIRuleDefinition
	{
		std::string id;
		std::vector<AIConditionSpec> conditions;
		AIDecisionSpec decision;
		DefinitionSource source;
	};

	struct AIBehaviourDefinition
	{
		std::string id;
		// A translation key, not a sentence (see resources/data/locales).
		std::string displayNameKey;
		// Authored order is evaluation order.
		std::vector<AIRuleDefinition> rules;
		DefinitionSource source;
	};

	inline constexpr int AISchemaVersion = 1;
	inline constexpr std::int64_t MaximumAIHealthPermille = 1000;
	inline constexpr std::int64_t MaximumAIDistanceCells = 128;
	inline constexpr std::int64_t MaximumAIMovementPoints = 1000000;

	[[nodiscard]] const std::map<std::string, AIUnitSelector> &aiUnitSelectorNames();
	[[nodiscard]] const std::map<std::string, AIInclusiveComparison> &aiInclusiveComparisonNames();
	[[nodiscard]] const std::map<std::string, AIBestAreaPreference> &aiBestAreaPreferenceNames();

	// Leaves the id empty: the registry loader assigns the validated filename stem. Ability, status
	// and tag references stay plain strings here and are resolved against the combat registries in
	// core/combat_definition_validation.hpp, inside the same load transaction.
	[[nodiscard]] AIBehaviourDefinition parseAIBehaviourDefinition(JsonReader &p_reader);

	// Every ability a behaviour would cast, in authored rule order - the list an encounter checks
	// against the spawn's derived loadout.
	[[nodiscard]] std::vector<std::string> aiCastAbilityReferences(const AIBehaviourDefinition &p_behaviour);
}
