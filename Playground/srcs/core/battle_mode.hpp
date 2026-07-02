#pragma once

#include "core/mode.hpp"

namespace pg
{
	class BattleMode : public Mode
	{
	protected:
		virtual void activate();
		virtual void deactivate();

	public:
		explicit BattleMode(GameContext &p_context);

		void enter() override;
		void exit() override;
	};
}
