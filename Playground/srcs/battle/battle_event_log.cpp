#include "battle/battle_event_log.hpp"

#include <stdexcept>

namespace pg
{
	void BattleEventLog::append(BattleEvent p_event)
	{
		_events.push_back(std::move(p_event));
	}

	std::span<const BattleEvent> BattleEventLog::events() const noexcept
	{
		return _events;
	}

	std::size_t BattleEventLog::size() const noexcept
	{
		return _events.size();
	}

	std::vector<BattleEvent> BattleEventLog::copy(EventRange p_range) const
	{
		if (p_range.first.value == 0 || p_range.onePastLast.value < p_range.first.value)
		{
			throw std::out_of_range("battle event range is malformed");
		}
		// Sequence numbers start at 1 and are contiguous, so sequence N lives at index N-1.
		if (p_range.onePastLast.value - 1 > _events.size())
		{
			throw std::out_of_range("battle event range runs past the committed stream");
		}

		std::vector<BattleEvent> copied;
		copied.reserve(static_cast<std::size_t>(p_range.count()));
		for (std::uint64_t sequence = p_range.first.value; sequence < p_range.onePastLast.value; ++sequence)
		{
			copied.push_back(_events[static_cast<std::size_t>(sequence - 1)]);
		}
		return copied;
	}

	std::string_view toString(BattleBatchKind p_kind) noexcept
	{
		switch (p_kind)
		{
		case BattleBatchKind::Construction:
			return "construction";
		case BattleBatchKind::Command:
			return "command";
		case BattleBatchKind::Timeline:
			return "timeline";
		case BattleBatchKind::TamingSystem:
			return "tamingSystem";
		case BattleBatchKind::TechnicalAbort:
			return "technicalAbort";
		}
		return "command";
	}
}
