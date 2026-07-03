#pragma once

#include <cstdint>
namespace pg
{
	class BattleContext;
	class BattlePlacementRules
	{
	public:
		[[nodiscard]] static bool autoPlace(BattleContext &p_context, std::uint32_t p_seed);
	};
}
