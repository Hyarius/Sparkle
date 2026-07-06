#pragma once

#include "ai/ai_condition.hpp"
#include "ai/ai_decision.hpp"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace pg
{
	class JsonReader;
	struct Ability;
	struct Status;
	template <typename TDefinition>
	class Registry;

	struct AIRule
	{
		std::vector<std::unique_ptr<AICondition>> conditions;
		std::unique_ptr<AIDecision> decision;
	};

	struct AIBehaviour
	{
		std::string id;
		std::string activeMode;
		std::map<std::string, std::vector<AIRule>> rulesByMode;

		[[nodiscard]] const std::vector<AIRule> &activeRules() const;
	};

	[[nodiscard]] AIBehaviour parseAIBehaviour(
		JsonReader &p_reader,
		const Registry<Ability> &p_abilities,
		const Registry<Status> &p_statuses);
}
