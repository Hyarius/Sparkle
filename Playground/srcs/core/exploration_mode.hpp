#pragma once

#include "core/mode.hpp"

namespace pg
{
	class ExplorationMode : public Mode
	{
	protected:
		virtual void activate();
		virtual void deactivate();

	public:
		explicit ExplorationMode(GameContext &p_context);

		void enter() override;
		void exit() override;
	};
}
