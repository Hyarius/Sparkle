#pragma once

#include "world/generator/macro_world_plan.hpp"
#include "world/generator/worldgen_params.hpp"

#include <cstdint>

namespace pg
{
	class MacroWorldGenerator
	{
	public:
		[[nodiscard]] static MacroWorldPlan generate(const WorldgenParams &p_params, std::uint64_t p_runSeed);

		static void landmass(MacroWorldPlan &p_plan, const WorldgenParams &p_params, std::uint64_t p_runSeed);
		static void heightRivers(MacroWorldPlan &p_plan, const WorldgenParams &p_params, std::uint64_t p_runSeed);
		static void cities(MacroWorldPlan &p_plan, const WorldgenParams &p_params, std::uint64_t p_runSeed);
		static void biomes(MacroWorldPlan &p_plan, const WorldgenParams &p_params, std::uint64_t p_runSeed);
		static void satellites(MacroWorldPlan &p_plan, const WorldgenParams &p_params, std::uint64_t p_runSeed);
		static void transportGraph(MacroWorldPlan &p_plan, const WorldgenParams &p_params, std::uint64_t p_runSeed);
		static void roads(MacroWorldPlan &p_plan, const WorldgenParams &p_params, std::uint64_t p_runSeed);
	};
}
