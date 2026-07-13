#pragma once

#include "world/generator/world_plan.hpp"

#include <cstdint>
#include <filesystem>

namespace pg
{
	// Where preview maps are saved: next to the executable, one file per seed, so maps
	// generated from different seeds can be kept and compared.
	[[nodiscard]] std::filesystem::path worldMapOutputPath(std::uint64_t p_seed);

	// Renders the world skeleton (zones shaded by height, rivers/lakes, roads/bridges,
	// settlements and POIs, zone labels, legend) to a PNG so a generated world can be
	// inspected at a glance. Returns false when the file could not be written.
	bool writeWorldMapPng(const WorldPlan &p_plan, const std::filesystem::path &p_path);
}
