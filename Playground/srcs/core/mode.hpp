#pragma once

#include "core/game_context.hpp"
#include "structures/system/event/spk_update_tick.hpp"

namespace pg
{
	class Mode
	{
	protected:
		GameContext &_context;

	public:
		explicit Mode(GameContext &p_context) :
			_context(p_context)
		{
		}

		virtual ~Mode() = default;

		Mode(const Mode &) = delete;
		Mode &operator=(const Mode &) = delete;

		virtual void enter() = 0;
		virtual void exit() = 0;
		virtual void update(const spk::UpdateTick &p_tick)
		{
			(void)p_tick;
		}
	};
}
