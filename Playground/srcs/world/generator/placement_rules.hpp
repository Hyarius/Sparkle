#pragma once

#include "world/generator/world_plan.hpp"

#include <array>
#include <filesystem>
#include <utility>

namespace spk::JSON
{
	class Value;
}

namespace pg
{
	class JsonReader;
	struct PrefabDefinition;
	template <typename TDefinition>
	class Registry;

	// JSON slot name <-> entity kind, shared by placements.json and the per-biome
	// "worldgen.prefabs" blocks.
	[[nodiscard]] const std::array<std::pair<const char *, PlanEntityKind>, 6> &planEntityKeyTable() noexcept;

	// A prefab pool is either a single prefab id string or an array of them; each id is
	// validated against the prefab registry.
	[[nodiscard]] std::vector<std::string> parsePrefabPool(
		const spk::JSON::Value &p_value,
		const std::filesystem::path &p_file,
		const std::string &p_path,
		const Registry<PrefabDefinition> &p_prefabs);

	// Parses resources/data/worldgen/placements.json:
	//
	//   {
	//     "version": 1,
	//     "entities": { "gym": "gym-shell", "city": "town-house", "rarePoi": null, ... }
	//   }
	//
	// Every named prefab must exist in the prefab registry; a null (or missing) entity
	// entry means that entity kind gets no prefab. Stairways are not listed here: each
	// biome builds them from its palette stair/slope voxels (see synthesizeClimbPrefabs).
	[[nodiscard]] PlanPlacementRules parsePlanPlacementRules(
		JsonReader &p_reader,
		const Registry<PrefabDefinition> &p_prefabs);
}
