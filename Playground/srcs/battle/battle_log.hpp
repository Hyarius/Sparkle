#pragma once

#include "battle/battle_event.hpp"

#include <vector>

namespace pg
{
	class BattleLog
	{
	private:
		std::vector<BattleEvent> _events;

	public:
		void append(BattleEvent p_event)
		{
			_events.push_back(std::move(p_event));
		}
		[[nodiscard]] const std::vector<BattleEvent> &events() const noexcept
		{
			return _events;
		}
		[[nodiscard]] std::size_t size() const noexcept
		{
			return _events.size();
		}
	};
}
