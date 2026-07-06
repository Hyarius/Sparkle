#include "ai/ai_factory.hpp"

#include "abilities/ability.hpp"
#include "ai/conditions/basic_conditions.hpp"
#include "ai/decisions/basic_decisions.hpp"
#include "core/json.hpp"
#include "core/registry.hpp"
#include "statuses/status.hpp"

#include <map>
#include <string>

namespace
{
	template <typename TDefinition>
	[[nodiscard]] const TDefinition &requireDefinition(
		pg::JsonReader &p_reader,
		const std::string &p_key,
		const pg::Registry<TDefinition> &p_registry,
		const std::string &p_domain)
	{
		const std::string id = p_reader.require<std::string>(p_key);
		const TDefinition *result = p_registry.tryGet(id);
		if (id.empty() || result == nullptr)
		{
			throw pg::JsonError(
				p_reader.file(), p_reader.pathFor(p_key), "unknown " + p_domain + " id '" + id + "'");
		}
		return *result;
	}

	[[noreturn]] void unknownType(pg::JsonReader &p_reader, const std::string &p_type, const std::string &p_known)
	{
		throw pg::JsonError(
			p_reader.file(), p_reader.pathFor("type"),
			"unknown polymorphic type '" + p_type + "' (known types: " + p_known + ")");
	}
}

namespace pg
{
	std::unique_ptr<AICondition> parseAICondition(
		JsonReader &p_reader,
		const Registry<Ability> &p_abilities,
		const Registry<Status> &p_statuses)
	{
		const std::string type = p_reader.require<std::string>("type");
		if (type == "enemyWithinRange")
		{
			p_reader.forbidUnknown({"type", "range"});
			const int range = p_reader.require<int>("range");
			if (range < 0)
			{
				throw JsonError(p_reader.file(), p_reader.pathFor("range"), "range must not be negative");
			}
			return std::make_unique<EnemyWithinRangeCondition>(range);
		}
		if (type == "allyHpBelow" || type == "selfHpBelow")
		{
			p_reader.forbidUnknown({"type", "percent"});
			const float percent = p_reader.require<float>("percent");
			if (percent < 0.0f || percent > 100.0f)
			{
				throw JsonError(p_reader.file(), p_reader.pathFor("percent"), "percent must be between 0 and 100");
			}
			return type == "allyHpBelow"
				? std::unique_ptr<AICondition>(std::make_unique<AllyHpBelowCondition>(percent))
				: std::unique_ptr<AICondition>(std::make_unique<SelfHpBelowCondition>(percent));
		}
		if (type == "hasStatus")
		{
			p_reader.forbidUnknown({"type", "status", "on"});
			const Status &status = requireDefinition(p_reader, "status", p_statuses, "status");
			static const std::map<std::string, AIStatusSubject> subjects = {
				{"self", AIStatusSubject::Self}, {"enemy", AIStatusSubject::Enemy}};
			return std::make_unique<HasStatusCondition>(
				status.id, p_reader.requireEnum<AIStatusSubject>("on", subjects));
		}
		if (type == "canCast")
		{
			p_reader.forbidUnknown({"type", "ability"});
			return std::make_unique<CanCastCondition>(
				requireDefinition(p_reader, "ability", p_abilities, "ability"));
		}
		unknownType(p_reader, type, "allyHpBelow, canCast, enemyWithinRange, hasStatus, selfHpBelow");
	}

	std::unique_ptr<AIDecision> parseAIDecision(
		JsonReader &p_reader,
		const Registry<Ability> &p_abilities)
	{
		const std::string type = p_reader.require<std::string>("type");
		if (type == "castAbility")
		{
			p_reader.forbidUnknown({"type", "ability", "target"});
			static const std::map<std::string, AITarget> targets = {
				{"nearestEnemy", AITarget::NearestEnemy},
				{"lowestHpEnemy", AITarget::LowestHpEnemy},
				{"self", AITarget::Self},
				{"lowestHpAlly", AITarget::LowestHpAlly}};
			const Ability &ability = requireDefinition(p_reader, "ability", p_abilities, "ability");
			return std::make_unique<CastAbilityDecision>(
				ability, p_reader.requireEnum<AITarget>("target", targets));
		}
		p_reader.forbidUnknown({"type"});
		if (type == "moveTowardNearestEnemy")
		{
			return std::make_unique<MoveTowardNearestEnemyDecision>();
		}
		if (type == "moveAwayFromEnemies")
		{
			return std::make_unique<MoveAwayFromEnemiesDecision>();
		}
		if (type == "endTurn")
		{
			return std::make_unique<EndTurnDecision>();
		}
		unknownType(p_reader, type, "castAbility, endTurn, moveAwayFromEnemies, moveTowardNearestEnemy");
	}
}
