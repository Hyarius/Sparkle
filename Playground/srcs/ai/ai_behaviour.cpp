#include "ai/ai_behaviour.hpp"

#include "ai/ai_factory.hpp"
#include "core/json.hpp"

#include <stdexcept>

namespace pg
{
	const std::vector<AIRule> &AIBehaviour::activeRules() const
	{
		const auto iterator = rulesByMode.find(activeMode);
		if (iterator == rulesByMode.end())
		{
			throw std::logic_error("AI behaviour active mode is not defined");
		}
		return iterator->second;
	}

	AIBehaviour parseAIBehaviour(
		JsonReader &p_reader,
		const Registry<Ability> &p_abilities,
		const Registry<Status> &p_statuses)
	{
		p_reader.forbidUnknown({"version", "activeMode", "rulesByMode"});
		if (p_reader.require<int>("version") != 1)
		{
			throw JsonError(p_reader.file(), p_reader.pathFor("version"), "unsupported AI behaviour version");
		}
		AIBehaviour result;
		result.activeMode = p_reader.require<std::string>("activeMode");
		if (result.activeMode.empty())
		{
			throw JsonError(p_reader.file(), p_reader.pathFor("activeMode"), "active mode must not be empty");
		}

		JsonReader modesReader = p_reader.child("rulesByMode");
		if (modesReader.value().empty())
		{
			throw JsonError(p_reader.file(), modesReader.path(), "at least one AI mode is required");
		}
		for (const auto &[mode, rulesValue] : modesReader.value().items())
		{
			const std::string modePath = modesReader.pathFor(mode);
			if (mode.empty() || !rulesValue.is_array())
			{
				throw JsonError(p_reader.file(), modePath, mode.empty() ? "mode name must not be empty" : "expected an array");
			}
			std::vector<AIRule> rules;
			for (std::size_t index = 0; index < rulesValue.size(); ++index)
			{
				const std::string rulePath = modePath + "[" + std::to_string(index) + "]";
				if (!rulesValue[index].is_object())
				{
					throw JsonError(p_reader.file(), rulePath, "expected an object");
				}
				JsonReader ruleReader(rulesValue[index], p_reader.file(), rulePath);
				ruleReader.forbidUnknown({"conditions", "decision"});
				AIRule rule;
				for (JsonReader conditionReader : ruleReader.childArray("conditions"))
				{
					rule.conditions.push_back(parseAICondition(conditionReader, p_abilities, p_statuses));
				}
				JsonReader decisionReader = ruleReader.child("decision");
				rule.decision = parseAIDecision(decisionReader, p_abilities);
				rules.push_back(std::move(rule));
			}
			result.rulesByMode.emplace(mode, std::move(rules));
		}
		if (!result.rulesByMode.contains(result.activeMode))
		{
			throw JsonError(
				p_reader.file(), p_reader.pathFor("activeMode"),
				"active mode '" + result.activeMode + "' is not present in rulesByMode");
		}
		return result;
	}
}
