#include "battle/battle_command.hpp"

namespace pg
{
	std::string_view toString(CommandController p_controller) noexcept
	{
		switch (p_controller)
		{
		case CommandController::System:
			return "system";
		case CommandController::Player:
			return "player";
		case CommandController::EnemyAi:
			return "enemyAi";
		case CommandController::DebugAutoplay:
			return "debugAutoplay";
		}
		return "system";
	}
}
