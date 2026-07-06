#pragma once

#include "battle/battle_event.hpp"

#include <span>

namespace pg
{
	class BattleUnit;

	class BattleCondition
	{
	public:
		enum class Scope
		{
			Ability,
			Turn,
			Fight,
			Game
		};

		struct Advancement
		{
			float progress = 0.0f;
			int completedRepeatCount = 0;
		};

		Scope scope = Scope::Fight;
		int requiredRepeatCount = 1;

		virtual ~BattleCondition() = default;
		[[nodiscard]] virtual bool isScopeLocked() const noexcept;
		[[nodiscard]] virtual Advancement evaluateEvents(
			std::span<const BattleEvent *const> p_events, Advancement p_current,
			const BattleUnit *p_subject = nullptr) const;
		[[nodiscard]] Advancement evaluateEvents(std::span<const BattleEvent *const> p_events) const
		{
			return evaluateEvents(p_events, Advancement{}, nullptr);
		}
		[[nodiscard]] bool isComplete(const Advancement &p_advancement) const noexcept;

	protected:
		[[nodiscard]] virtual float evaluateEventProgress(
			const BattleEvent &p_event, const BattleUnit *p_subject) const = 0;
	};

	template <typename TEvent>
	class BattleConditionTemplated : public BattleCondition
	{
	protected:
		[[nodiscard]] float evaluateEventProgress(
			const BattleEvent &p_event, const BattleUnit *p_subject) const final
		{
			const TEvent *typed = p_event.getIf<TEvent>();
			return typed != nullptr ? evaluateTypedEventProgress(*typed, p_subject) : 0.0f;
		}
		[[nodiscard]] virtual float evaluateTypedEventProgress(
			const TEvent &p_event, const BattleUnit *p_subject) const = 0;
	};

	[[nodiscard]] float computeLinearProgress(float p_amount, float p_requiredAmount) noexcept;
}
