#pragma once

#include <memory>

namespace pg
{
	class AICondition;
	class AIDecision;
	class JsonReader;
	struct Ability;
	struct Status;
	template <typename TDefinition>
	class Registry;

	[[nodiscard]] std::unique_ptr<AICondition> parseAICondition(
		JsonReader &p_reader,
		const Registry<Ability> &p_abilities,
		const Registry<Status> &p_statuses);
	[[nodiscard]] std::unique_ptr<AIDecision> parseAIDecision(
		JsonReader &p_reader,
		const Registry<Ability> &p_abilities);
}
