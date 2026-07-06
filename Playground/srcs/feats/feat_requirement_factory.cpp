#include "feats/feat_requirement.hpp"

#include "abilities/ability.hpp"
#include "battle/battle_attributes.hpp"
#include "core/json.hpp"
#include "statuses/status.hpp"

#include <algorithm>
#include <map>
#include <set>

namespace
{
	using Scope = pg::BattleCondition::Scope;

	[[nodiscard]] Scope parseScope(pg::JsonReader &p_reader, Scope p_locked, bool p_isLocked)
	{
		if (p_isLocked)
		{
			if (p_reader.contains("scope"))
			{
				throw pg::JsonError(p_reader.file(), p_reader.pathFor("scope"), "scope is fixed for this condition type");
			}
			return p_locked;
		}
		static const std::map<std::string, Scope> scopes = {
			{"ability", Scope::Ability}, {"turn", Scope::Turn}, {"fight", Scope::Fight}, {"game", Scope::Game}};
		return p_reader.requireEnum<Scope>("scope", scopes);
	}

	void validateKeys(pg::JsonReader &p_reader, std::initializer_list<std::string_view> p_specific)
	{
		std::set<std::string> allowed = {"type", "scope", "requiredRepeatCount"};
		for (std::string_view key : p_specific) allowed.emplace(key);
		for (const auto &[key, unused] : p_reader.value().items())
		{
			(void)unused;
			if (!allowed.contains(key))
			{
				throw pg::JsonError(p_reader.file(), p_reader.pathFor(key), "unknown field");
			}
		}
	}

	[[nodiscard]] float positiveNumber(pg::JsonReader &p_reader, const std::string &p_key)
	{
		const float result = p_reader.require<float>(p_key);
		if (result <= 0.0f)
		{
			throw pg::JsonError(p_reader.file(), p_reader.pathFor(p_key), "value must be positive");
		}
		return result;
	}

	[[nodiscard]] std::vector<std::string> stringArray(
		pg::JsonReader &p_reader, const std::string &p_key, bool p_optional = false)
	{
		if (p_optional && !p_reader.contains(p_key)) return {};
		const nlohmann::json value = p_reader.require<nlohmann::json>(p_key);
		if (!value.is_array())
		{
			throw pg::JsonError(p_reader.file(), p_reader.pathFor(p_key), "expected an array");
		}
		std::vector<std::string> result;
		for (std::size_t index = 0; index < value.size(); ++index)
		{
			if (!value[index].is_string() || value[index].get<std::string>().empty())
			{
				throw pg::JsonError(p_reader.file(), p_reader.pathFor(p_key) + "[" + std::to_string(index) + "]", "expected a non-empty id");
			}
			result.push_back(value[index].get<std::string>());
		}
		return result;
	}

	template <typename TEnum>
	[[nodiscard]] int optionalEnum(
		pg::JsonReader &p_reader, const std::string &p_key, const std::map<std::string, TEnum> &p_values, int p_default = -1)
	{
		return p_reader.contains(p_key) ? static_cast<int>(p_reader.requireEnum<TEnum>(p_key, p_values)) : p_default;
	}

	[[nodiscard]] std::unique_ptr<pg::EventFeatRequirement> eventRequirement(
		pg::JsonReader &p_reader, pg::BattleEventType p_type, bool p_locked = false, Scope p_scope = Scope::Fight)
	{
		auto result = std::make_unique<pg::EventFeatRequirement>();
		result->eventType = p_type;
		result->scopeLocked = p_locked;
		result->scope = parseScope(p_reader, p_scope, p_locked);
		result->requiredRepeatCount = p_reader.optional<int>("requiredRepeatCount", 1);
		if (result->requiredRepeatCount <= 0)
		{
			throw pg::JsonError(p_reader.file(), p_reader.pathFor("requiredRepeatCount"), "value must be positive");
		}
		return result;
	}
}

namespace pg
{
	bool EventFeatRequirement::isScopeLocked() const noexcept
	{
		return scopeLocked;
	}

	float EventFeatRequirement::evaluateEventProgress(
		const BattleEvent &p_event, const BattleUnit *p_subject) const
	{
		if (battleEventType(p_event) != eventType) return 0.0f;
		const BattleEventContext &context = battleEventContext(p_event);
		if (p_subject != nullptr)
		{
			if (role == Role::Caster && context.caster != p_subject) return 0.0f;
			if (role == Role::Target && context.target != p_subject) return 0.0f;
			if (role == Role::Either && context.caster != p_subject && context.target != p_subject) return 0.0f;
		}
		if (requireCasterTargetEquality && context.caster != context.target) return 0.0f;
		if (requireCasterTargetDifference && context.caster == context.target) return 0.0f;

		int amount = 0;
		int eventDistance = 0;
		int eventDamageKind = -1;
		int eventShieldKind = -1;
		int eventResourceKind = -1;
		int eventOrientation = -1;
		const Status *eventStatus = nullptr;
		bool eventUnitSurvived = true;
		std::visit([&](const auto &p_typed) {
			using T = std::remove_cvref_t<decltype(p_typed)>;
			if constexpr (std::same_as<T, DamageEvent>) { amount = p_typed.amount; eventDamageKind = static_cast<int>(p_typed.kind); }
			else if constexpr (std::same_as<T, HealEvent>) amount = p_typed.amount;
			else if constexpr (std::same_as<T, AbilityCastEvent>) eventDistance = p_typed.distance;
			else if constexpr (std::same_as<T, ShieldAppliedEvent>) { amount = p_typed.amount; eventShieldKind = static_cast<int>(p_typed.kind); }
			else if constexpr (std::same_as<T, DamageAbsorbedEvent>) { amount = p_typed.amount; eventDamageKind = static_cast<int>(p_typed.kind); }
			else if constexpr (std::same_as<T, ShieldBrokenEvent>) eventShieldKind = static_cast<int>(p_typed.kind);
			else if constexpr (std::same_as<T, StatusAppliedEvent> || std::same_as<T, StatusRemovedEvent>) { amount = p_typed.count; eventStatus = p_typed.status; }
			else if constexpr (std::same_as<T, BattleWonEvent>) eventUnitSurvived = p_typed.unitSurvived;
			else if constexpr (std::same_as<T, ResourceConsumedEvent>) { amount = p_typed.amount; eventResourceKind = static_cast<int>(p_typed.resource); }
			else if constexpr (std::same_as<T, DistanceTravelledEvent> || std::same_as<T, TeleportedEvent>) eventDistance = p_typed.distance;
			else if constexpr (std::same_as<T, DisplacementEvent>) { eventDistance = p_typed.distance; eventOrientation = static_cast<int>(p_typed.orientation); }
			else if constexpr (std::same_as<T, TurnStartedEvent> || std::same_as<T, TurnEndedEvent>) eventDistance = p_typed.nearestUnitDistance;
			else if constexpr (std::same_as<T, HitSurvivedEvent>) amount = p_typed.remainingHealth;
		}, p_event.data);

		if (damageKind >= 0 && eventDamageKind != damageKind) return 0.0f;
		if (shieldKind >= 0 && eventShieldKind != shieldKind) return 0.0f;
		if (resourceKind >= 0 && eventResourceKind != resourceKind) return 0.0f;
		if (orientation >= 0 && eventOrientation != orientation) return 0.0f;
		if (requireUnitSurvival && !eventUnitSurvived) return 0.0f;
		if (!statusId.empty() && (eventStatus == nullptr || eventStatus->id != statusId)) return 0.0f;
		if (!abilityIds.empty())
		{
			if (context.sourceAbility == nullptr ||
				std::ranges::find(abilityIds, context.sourceAbility->id) == abilityIds.end()) return 0.0f;
		}
		if (rangeCondition == "within" && eventDistance > range) return 0.0f;
		if (rangeCondition == "atLeast" && eventDistance < range) return 0.0f;
		if (distanceCondition == "within" && eventDistance > distance) return 0.0f;
		if (distanceCondition == "atLeast" && eventDistance < distance) return 0.0f;
		if (distanceCondition == "between" && (eventDistance < distance || eventDistance > maximumDistance)) return 0.0f;
		const float value = countEvents ? 1.0f : static_cast<float>(
			(eventType == BattleEventType::DistanceTravelled || eventType == BattleEventType::Teleported || eventType == BattleEventType::Displacement)
				? eventDistance : amount);
		return computeLinearProgress(value, requiredAmount);
	}

	bool CompositeFeatRequirement::isScopeLocked() const noexcept
	{
		return true;
	}

	BattleCondition::Advancement CompositeFeatRequirement::evaluateEvents(
		std::span<const BattleEvent *const> p_events, Advancement p_current,
		const BattleUnit *p_subject) const
	{
		if (children.empty()) return p_current;
		float aggregate = operation == Operator::And ? 100.0f : 0.0f;
		bool complete = operation == Operator::And;
		for (const auto &child : children)
		{
			const Advancement childResult = child->evaluateEvents(p_events, {}, p_subject);
			const bool childComplete = child->isComplete(childResult);
			const float score = childComplete ? 100.0f : childResult.progress;
			if (operation == Operator::And)
			{
				aggregate = std::min(aggregate, score);
				complete = complete && childComplete;
			}
			else
			{
				aggregate = std::max(aggregate, score);
				complete = complete || childComplete;
			}
		}
		if (complete) p_current.completedRepeatCount = requiredRepeatCount;
		p_current.progress = complete ? 0.0f : aggregate;
		return p_current;
	}

	float CompositeFeatRequirement::evaluateEventProgress(const BattleEvent &, const BattleUnit *) const
	{
		return 0.0f;
	}

	std::unique_ptr<BattleCondition> parseBattleCondition(
		JsonReader &p_reader, BattleConditionCatalog p_catalog)
	{
		const std::string type = p_reader.require<std::string>("type");
		if (type == "and" || type == "or")
		{
			validateKeys(p_reader, {"children"});
			auto result = std::make_unique<CompositeFeatRequirement>();
			result->operation = type == "and" ? CompositeFeatRequirement::Operator::And : CompositeFeatRequirement::Operator::Or;
			result->requiredRepeatCount = 1;
			for (JsonReader child : p_reader.childArray("children"))
			{
				result->children.push_back(parseBattleCondition(child, p_catalog));
			}
			if (result->children.empty()) throw JsonError(p_reader.file(), p_reader.pathFor("children"), "expected at least one child");
			return result;
		}

		std::unique_ptr<EventFeatRequirement> result;
		if (type == "dealDamage" || type == "takeDamage")
		{
			validateKeys(p_reader, {"requiredAmount", "damageKind", "sourceAbilities"});
			result = eventRequirement(p_reader, BattleEventType::Damage);
			result->role = type == "dealDamage" ? EventFeatRequirement::Role::Caster : EventFeatRequirement::Role::Target;
			result->requiredAmount = positiveNumber(p_reader, "requiredAmount");
			static const std::map<std::string, DamageKind> kinds = {{"physical", DamageKind::Physical}, {"magical", DamageKind::Magical}};
			const std::string kind = p_reader.optional<std::string>("damageKind", "any");
			if (kind != "any")
			{
				const auto found = kinds.find(kind);
				if (found == kinds.end()) throw JsonError(p_reader.file(), p_reader.pathFor("damageKind"), "expected any, physical, or magical");
				result->damageKind = static_cast<int>(found->second);
			}
			result->abilityIds = stringArray(p_reader, "sourceAbilities", true);
		}
		else if (type == "surviveHit" || type == "maxDamageAbsorbedInOneHit")
		{
			validateKeys(p_reader, {"requiredAmount"});
			result = eventRequirement(p_reader, type == "surviveHit" ? BattleEventType::HitSurvived : BattleEventType::DamageAbsorbed, true, Scope::Ability);
			result->role = EventFeatRequirement::Role::Target;
			result->requiredAmount = positiveNumber(p_reader, "requiredAmount");
		}
		else if (type == "healHealth" || type == "healTarget")
		{
			validateKeys(p_reader, {"requiredAmount", "target"});
			result = eventRequirement(p_reader, BattleEventType::Heal);
			result->role = EventFeatRequirement::Role::Caster;
			result->requiredAmount = positiveNumber(p_reader, "requiredAmount");
			if (type == "healTarget")
			{
				const std::string target = p_reader.optional<std::string>("target", "any");
				if (target == "self") result->requireCasterTargetEquality = true;
				else if (target == "ally") result->requireCasterTargetDifference = true;
				else if (target != "any") throw JsonError(p_reader.file(), p_reader.pathFor("target"), "expected self, ally, or any");
			}
		}
		else if (type == "applyShield" || type == "applyShieldCount" || type == "shieldBroken")
		{
			validateKeys(p_reader, {type == "applyShield" ? "requiredAmount" : "requiredCount", "kind"});
			result = eventRequirement(p_reader, type == "shieldBroken" ? BattleEventType::ShieldBroken : BattleEventType::ShieldApplied);
			result->role = type == "shieldBroken" ? EventFeatRequirement::Role::Target : EventFeatRequirement::Role::Caster;
			result->requiredAmount = positiveNumber(p_reader, type == "applyShield" ? "requiredAmount" : "requiredCount");
			result->countEvents = type != "applyShield";
			static const std::map<std::string, ShieldKind> kinds = {{"physical", ShieldKind::Physical}, {"magical", ShieldKind::Magical}};
			const std::string kind = p_reader.optional<std::string>("kind", "any");
			if (kind != "any")
			{
				const auto found = kinds.find(kind);
				if (found == kinds.end()) throw JsonError(p_reader.file(), p_reader.pathFor("kind"), "expected any, physical, or magical");
				result->shieldKind = static_cast<int>(found->second);
			}
		}
		else if (type == "absorbDamageWithShield")
		{
			validateKeys(p_reader, {"requiredAmount"}); result = eventRequirement(p_reader, BattleEventType::DamageAbsorbed);
			result->role = EventFeatRequirement::Role::Target;
			result->requiredAmount = positiveNumber(p_reader, "requiredAmount");
		}
		else if (type == "applyStatusCount" || type == "removeStatusCount")
		{
			validateKeys(p_reader, {"requiredCount", "status", "sourceAbilities"});
			result = eventRequirement(p_reader, type == "applyStatusCount" ? BattleEventType::StatusApplied : BattleEventType::StatusRemoved);
			result->role = EventFeatRequirement::Role::Caster;
			result->requiredAmount = positiveNumber(p_reader, "requiredCount"); result->countEvents = true;
			result->statusId = p_reader.optional<std::string>("status", "");
			result->abilityIds = stringArray(p_reader, "sourceAbilities", true);
		}
		else if (type == "killCount" || type == "lastHit")
		{
			validateKeys(p_reader, {"requiredCount", "sourceAbilities"}); result = eventRequirement(p_reader, BattleEventType::UnitDefeated);
			result->role = EventFeatRequirement::Role::Caster;
			result->requiredAmount = positiveNumber(p_reader, "requiredCount"); result->countEvents = true;
			result->abilityIds = stringArray(p_reader, "sourceAbilities", true);
		}
		else if (type == "winBattleCount")
		{
			if (p_catalog != BattleConditionCatalog::Feat)
			{
				throw JsonError(p_reader.file(), p_reader.pathFor("type"), "unknown taming condition type '" + type + "'");
			}
			validateKeys(p_reader, {"requireUnitSurvival"}); result = eventRequirement(p_reader, BattleEventType::BattleWon, true, Scope::Game);
			result->role = EventFeatRequirement::Role::Caster;
			result->countEvents = true; result->requiredAmount = 1.0f; result->requireUnitSurvival = p_reader.optional<bool>("requireUnitSurvival", false);
		}
		else if (type == "consumeResources")
		{
			validateKeys(p_reader, {"resource", "requiredAmount"}); result = eventRequirement(p_reader, BattleEventType::ResourceConsumed);
			result->role = EventFeatRequirement::Role::Caster;
			result->requiredAmount = positiveNumber(p_reader, "requiredAmount");
			static const std::map<std::string, BattleResourceKind> resources = {{"ap", BattleResourceKind::AP}, {"mp", BattleResourceKind::MP}};
			result->resourceKind = static_cast<int>(p_reader.requireEnum<BattleResourceKind>("resource", resources));
		}
		else if (type == "totalDistanceTravelled" || type == "teleportDistance" || type == "teleportCount")
		{
			const bool count = type == "teleportCount"; validateKeys(p_reader, {count ? "requiredCount" : "requiredDistance"});
			result = eventRequirement(p_reader, type == "totalDistanceTravelled" ? BattleEventType::DistanceTravelled : BattleEventType::Teleported);
			result->role = EventFeatRequirement::Role::Caster;
			result->requiredAmount = positiveNumber(p_reader, count ? "requiredCount" : "requiredDistance"); result->countEvents = count;
		}
		else if (type == "displacementDealt" || type == "displacementReceived")
		{
			const bool count = type == "displacementReceived"; validateKeys(p_reader, {count ? "requiredCount" : "requiredDistance", "orientation"});
			result = eventRequirement(p_reader, BattleEventType::Displacement); result->countEvents = count;
			result->role = type == "displacementDealt" ? EventFeatRequirement::Role::Caster : EventFeatRequirement::Role::Target;
			result->requiredAmount = positiveNumber(p_reader, count ? "requiredCount" : "requiredDistance");
			static const std::map<std::string, DisplacementOrientation> orientations = {{"towardCaster", DisplacementOrientation::TowardCaster}, {"awayFromCaster", DisplacementOrientation::AwayFromCaster}};
			result->orientation = optionalEnum(p_reader, "orientation", orientations);
		}
		else if (type == "turnStartPosition" || type == "turnEndPosition")
		{
			validateKeys(p_reader, {"target", "condition", "distance", "maximumDistance"});
			result = eventRequirement(p_reader, type == "turnStartPosition" ? BattleEventType::TurnStarted : BattleEventType::TurnEnded, true, Scope::Ability);
			result->role = EventFeatRequirement::Role::Caster;
			result->countEvents = true; result->requiredAmount = 1.0f;
			result->distanceCondition = p_reader.require<std::string>("condition"); result->distance = p_reader.require<int>("distance");
			result->maximumDistance = p_reader.optional<int>("maximumDistance", result->distance);
			if (result->distance < 0 || (result->distanceCondition != "within" && result->distanceCondition != "atLeast" && result->distanceCondition != "between"))
				throw JsonError(p_reader.file(), p_reader.pathFor("condition"), "invalid distance condition");
		}
		else if (type == "castAbilityCount")
		{
			validateKeys(p_reader, {"abilities", "requiredCount", "targetRangeCondition", "range"}); result = eventRequirement(p_reader, BattleEventType::AbilityCast);
			result->role = EventFeatRequirement::Role::Caster;
			result->abilityIds = stringArray(p_reader, "abilities"); result->requiredAmount = positiveNumber(p_reader, "requiredCount"); result->countEvents = true;
			result->rangeCondition = p_reader.optional<std::string>("targetRangeCondition", "either"); result->range = p_reader.optional<int>("range", 0);
			if (result->rangeCondition != "either" && result->rangeCondition != "within" && result->rangeCondition != "atLeast")
				throw JsonError(p_reader.file(), p_reader.pathFor("targetRangeCondition"), "invalid range condition");
		}
		else
		{
			throw JsonError(
				p_reader.file(), p_reader.pathFor("type"),
				"unknown " + std::string(p_catalog == BattleConditionCatalog::Feat ? "feat requirement" : "taming condition") +
					" type '" + type + "'");
		}
		return result;
	}

	std::unique_ptr<FeatRequirement> parseFeatRequirement(JsonReader &p_reader)
	{
		return parseBattleCondition(p_reader, BattleConditionCatalog::Feat);
	}
}
