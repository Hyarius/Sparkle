#pragma once

#include <filesystem>

namespace pg
{
	class MacroWorldPlan;

	class PlanDebugImage
	{
	public:
		static void writePng(const MacroWorldPlan &p_plan, const std::filesystem::path &p_output);
	};
}
