#pragma once

#include "core/json.hpp"
#include "core/registry.hpp"

#include <string>
#include <vector>

namespace pg
{
	// Structural roles are deliberately independent of prefab ids.  Biomes bind a
	// role to their own art while a composition describes only the settlement shape.
	enum class TownBuildingRole
	{
		None,
		CreatureCenter,
		Shop,
		Gym,
		Port,
		Home
	};
	enum class TownCompositionKind
	{
		City,
		Gym,
		Port
	};

	struct CountRange
	{
		int minimum = 0;
		int maximum = 0;
	};

	struct TownBuildingRequest
	{
		std::string id;
		TownBuildingRole role = TownBuildingRole::None;
		CountRange count{};
		bool required = true;
		int minimumSpacing = 0;
		int placementPriority = 0;
	};

	struct TownSceneryRequest
	{
		std::string id;
		std::string prefabId;
		CountRange count{};
		int spacing = 1;
		bool required = false;
	};

	struct TownLayoutSettings
	{
		int mainRoadWidth = 3;
		int urbanRoadWidth = 3;
		int minimumBuildingSpacing = 4;
		int buildingAttemptsPerItem = 128;
		int layoutAttempts = 32;
		int routeTurnPenalty = 2;
		int routeSlopePenalty = 20;
	};

	struct TownComposition
	{
		std::string id;
		TownCompositionKind kind = TownCompositionKind::City;
		TownLayoutSettings layout{};
		std::vector<TownBuildingRequest> buildings;
		std::vector<TownSceneryRequest> roadScenery;
		std::vector<TownSceneryRequest> groundScenery;
	};

	[[nodiscard]] TownComposition parseTownComposition(JsonReader &p_reader);
	[[nodiscard]] std::string townCompositionKindName(TownCompositionKind p_kind);
	[[nodiscard]] std::string townBuildingRoleName(TownBuildingRole p_role);
	using TownCompositionCatalog = Registry<TownComposition>;
}
