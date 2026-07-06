#include "taming/taming_progress.hpp"

#include "taming/taming_profile.hpp"

#include <algorithm>

namespace pg
{
	TamingProgress::TamingProgress(const TamingProfile &p_profile) :
		_profile(&p_profile),
		advancements(p_profile.conditions.size())
	{
	}

	bool TamingProgress::isImpressed() const noexcept
	{
		if (_failed || _profile == nullptr || _profile->conditions.empty() ||
			advancements.size() != _profile->conditions.size())
		{
			return false;
		}
		for (std::size_t index = 0; index < _profile->conditions.size(); ++index)
		{
			if (!_profile->conditions[index]->isComplete(advancements[index]))
			{
				return false;
			}
		}
		return true;
	}

	bool TamingProgress::hasFailed() const noexcept
	{
		return _failed;
	}

	const TamingProfile *TamingProgress::profile() const noexcept
	{
		return _profile;
	}

	void TamingProgress::evaluateEvents(std::span<const BattleEvent *const> p_events)
	{
		if (_failed || isImpressed() || _profile == nullptr)
		{
			return;
		}
		for (std::size_t index = 0; index < _profile->conditions.size(); ++index)
		{
			advancements[index] = _profile->conditions[index]->evaluateEvents(
				p_events, advancements[index], nullptr);
		}
	}

	void TamingProgress::markFailed() noexcept
	{
		if (!isImpressed())
		{
			_failed = true;
		}
	}

	void TamingProgress::reset() noexcept
	{
		_failed = false;
		std::ranges::fill(advancements, BattleCondition::Advancement{});
	}
}
