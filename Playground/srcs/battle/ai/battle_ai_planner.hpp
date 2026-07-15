#pragma once

#include "ai/ai_definition.hpp"
#include "battle/battle_command.hpp"

#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace pg
{
	class BattleSession;

	enum class AIPlanFailure
	{
		None,
		WrongPhase,
		UnitNotActive,
		MissingBehaviour,
		NoRuleProducedCommand
	};

	// This is intentionally diagnostic-only state.  Rules and conditions never read it back, so
	// adding a trace cannot change simulation or the material-state digest.
	struct AIRuleTrace
	{
		std::string ruleId;
		bool conditionsMatched = false;
		bool decisionProducedLegalCommand = false;
		std::optional<std::string> detailCode;

		[[nodiscard]] bool operator==(const AIRuleTrace &) const = default;
	};

	struct AIPlannedCommand
	{
		BattleUnitId unit;
		std::string behaviourId;
		std::string ruleId;
		BattleCommand command;
		std::vector<AIRuleTrace> evaluatedRules;

		[[nodiscard]] bool operator==(const AIPlannedCommand &) const = default;
	};

	using AIPlanResult = std::variant<AIPlannedCommand, AIPlanFailure>;

	// A stateless query/planning service.  It borrows the session only for the duration of one
	// call, reads a copied snapshot and the shared legal-plan APIs, and never submits a command.
	class BattleAIPlanner
	{
	public:
		[[nodiscard]] AIPlanResult chooseNextCommand(
			const BattleSession &p_session,
			BattleUnitId p_unit,
			const AIBehaviourDefinition &p_behaviour) const;
	};
}
