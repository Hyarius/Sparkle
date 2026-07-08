#pragma once

#include "world/generator/world_plan.hpp"

#include <filesystem>

namespace pg
{
	// Renders the world skeleton (zones shaded by height, rivers/lakes, roads/bridges,
	// settlements and POIs, zone labels, legend) to a PNG so a generated world can be
	// inspected at a glance. Returns false when the file could not be written.
	bool writeWorldMapPng(const WorldPlan &p_plan, const std::filesystem::path &p_path);
}
