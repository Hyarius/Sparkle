#include "feats/battle_condition.hpp"

#include <algorithm>
#include <cmath>
#include <map>

namespace pg
{
	bool BattleCondition::isScopeLocked() const noexcept
	{
		return false;
	}

	BattleCondition::Advancement BattleCondition::evaluateEvents(
		std::span<const BattleEvent *const> p_events, Advancement p_current,
		const BattleUnit *p_subject) const
	{
		p_current.progress = std::clamp(p_current.progress, 0.0f, 99.999f);
		p_current.completedRepeatCount = std::clamp(p_current.completedRepeatCount, 0, requiredRepeatCount);
		if (isComplete(p_current) || requiredRepeatCount <= 0)
		{
			return p_current;
		}

		auto recordWindow = [&](float p_progress) {
			if (p_progress >= 100.0f)
			{
				p_current.completedRepeatCount = std::min(requiredRepeatCount, p_current.completedRepeatCount + 1);
				p_current.progress = 0.0f;
			}
			else
			{
				p_current.progress = std::max(p_current.progress, std::clamp(p_progress, 0.0f, 99.999f));
			}
		};

		if (scope == Scope::Ability)
		{
			for (const BattleEvent *event : p_events)
			{
				if (event != nullptr) recordWindow(std::max(0.0f, evaluateEventProgress(*event, p_subject)));
				if (isComplete(p_current)) break;
			}
			return p_current;
		}

		if (scope == Scope::Turn)
		{
			std::map<int, float> turns;
			for (const BattleEvent *event : p_events)
			{
				if (event != nullptr) turns[battleEventContext(*event).turnIndex] += std::max(0.0f, evaluateEventProgress(*event, p_subject));
			}
			for (const auto &[unused, progress] : turns)
			{
				(void)unused;
				recordWindow(progress);
				if (isComplete(p_current)) break;
			}
			return p_current;
		}

		float total = p_current.progress;
		for (const BattleEvent *event : p_events)
		{
			if (event != nullptr) total += std::max(0.0f, evaluateEventProgress(*event, p_subject));
		}
		while (total >= 100.0f && !isComplete(p_current))
		{
			total -= 100.0f;
			++p_current.completedRepeatCount;
		}
		p_current.progress = isComplete(p_current) ? 0.0f : std::clamp(total, 0.0f, 99.999f);
		return p_current;
	}

	bool BattleCondition::isComplete(const Advancement &p_advancement) const noexcept
	{
		return p_advancement.completedRepeatCount >= requiredRepeatCount;
	}

	float computeLinearProgress(float p_amount, float p_requiredAmount) noexcept
	{
		return p_requiredAmount > 0.0f ? p_amount / p_requiredAmount * 100.0f : 0.0f;
	}
}
