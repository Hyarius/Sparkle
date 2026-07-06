#pragma once

#include "feats/battle_condition.hpp"

#include <memory>
#include <string>
#include <vector>

namespace pg
{
	class JsonReader;
	using FeatRequirement = BattleCondition;
	enum class BattleConditionCatalog
	{
		Feat,
		Taming
	};

	class EventFeatRequirement final : public FeatRequirement
	{
	public:
		enum class Role { Either, Caster, Target };
		BattleEventType eventType = BattleEventType::Damage;
		Role role = Role::Either;
		float requiredAmount = 1.0f;
		bool countEvents = false;
		bool scopeLocked = false;
		bool requireCasterTargetEquality = false;
		bool requireCasterTargetDifference = false;
		bool requireUnitSurvival = false;
		int damageKind = -1;
		int shieldKind = -1;
		int resourceKind = -1;
		int orientation = -1;
		std::string statusId;
		std::vector<std::string> abilityIds;
		std::string rangeCondition = "either";
		int range = 0;
		std::string distanceCondition;
		int distance = 0;
		int maximumDistance = 0;

		[[nodiscard]] bool isScopeLocked() const noexcept override;

	protected:
		[[nodiscard]] float evaluateEventProgress(
			const BattleEvent &p_event, const BattleUnit *p_subject) const override;
	};

	class CompositeFeatRequirement final : public FeatRequirement
	{
	public:
		enum class Operator { And, Or };
		Operator operation = Operator::And;
		std::vector<std::unique_ptr<FeatRequirement>> children;

		[[nodiscard]] bool isScopeLocked() const noexcept override;
		[[nodiscard]] Advancement evaluateEvents(
			std::span<const BattleEvent *const> p_events, Advancement p_current,
			const BattleUnit *p_subject = nullptr) const override;

	protected:
		[[nodiscard]] float evaluateEventProgress(const BattleEvent &, const BattleUnit *) const override;
	};

	[[nodiscard]] std::unique_ptr<FeatRequirement> parseFeatRequirement(JsonReader &p_reader);
	[[nodiscard]] std::unique_ptr<BattleCondition> parseBattleCondition(
		JsonReader &p_reader, BattleConditionCatalog p_catalog);
}
