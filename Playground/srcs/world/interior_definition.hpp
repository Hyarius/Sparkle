#pragma once

#include "core/json.hpp"
#include "core/registry.hpp"
#include "core/weighted_pool.hpp"

#include <string>

namespace pg
{
	struct PrefabDefinition;

	// A "random composition" recipe for the inside of a building
	// (resources/data/interiors/<id>.json). The file does not describe the interior
	// itself: it lists an entry room (which carries the arrival pad and the exit
	// teleporter) plus a pool of rooms allowed to connect to it. The world planner
	// composes a concrete layout out of these rooms, seeded per placement, whenever a
	// prefab carrying `"interior": "<id>"` is added to the plan.
	//
	// Room prefabs connect through anchors named "connector:+x", "connector:-x",
	// "connector:+z" or "connector:-z", positioned on the wall block they open through
	// (at body height, one cell above the floor). Two rooms join by facing connectors;
	// the composer carves the shared doorway, so unused connectors simply stay walls.
	struct InteriorDefinition
	{
		std::string id;
		std::string entryPrefabId;
		WeightedPool<std::string> rooms;
		int minRooms = 1; // rooms grown beyond the entry room
		int maxRooms = 3;
	};

	[[nodiscard]] InteriorDefinition parseInteriorDefinition(
		JsonReader &p_reader,
		const Registry<PrefabDefinition> &p_prefabs);
}
