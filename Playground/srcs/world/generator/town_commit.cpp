#include "world/generator/town_commit.hpp"

#include <stdexcept>

namespace pg
{
	void commitTownCandidate(WorldPlan &p_plan, const TownCandidate &p_candidate)
	{
		if (p_candidate.compositionId.empty())
		{
			throw std::invalid_argument("cannot commit a town without a composition id");
		}
		for (const PlanTownRecord &town : p_plan.towns)
		{
			if (town.macroEntityIndex == p_candidate.macroEntityIndex)
			{
				throw std::logic_error("settlement already owns a committed town");
			}
		}
		WorldPlan merged = p_plan;
		PlanTownRecord record{.kind = p_candidate.kind, .macroEntityIndex = p_candidate.macroEntityIndex, .compositionId = p_candidate.compositionId, .bounds = p_candidate.bounds, .centerRow = p_candidate.centerRow, .centerCol = p_candidate.centerCol};
		record.boundaryCells = p_candidate.boundaryCells;
		record.mainRoadSurface = p_candidate.mainRoadSurface;
		record.urbanRoadSurface = p_candidate.urbanRoadSurface;
		for (const ResolvedTownEntrance &entrance : p_candidate.entrances)
		{
			ResolvedTownEntranceRecord resolved{.buildingInstanceId = entrance.buildingInstanceId, .threshold = entrance.threshold, .outward = entrance.outward, .connectionPoint = {p_candidate.bounds.minX + entrance.connectionPoint.x, p_candidate.bounds.minZ + entrance.connectionPoint.z}};
			for (TownColumn approach : entrance.approachColumns)
			{
				resolved.approachColumns.emplace_back(p_candidate.bounds.minX + approach.x, p_candidate.bounds.minZ + approach.z);
			}
			record.entrances.push_back(std::move(resolved));
		}
		record.routes = p_candidate.routes;
		record.dockCells = p_candidate.dockCells;
		record.boardingEndpoint = p_candidate.boardingEndpoint;
		record.boatLinkIndices = p_candidate.boatLinkIndices;
		const std::size_t firstPlacement = merged.placements.size();
		for (const PrefabPlacement &building : p_candidate.buildings)
		{
			merged.placements.push_back(building);
		}
		for (std::size_t i = 0; i < p_candidate.buildings.size(); ++i)
		{
			record.buildingPlacementIndices.push_back(firstPlacement + i);
		}
		merged.townClaims.insert(merged.townClaims.end(), p_candidate.buildingClaims.begin(), p_candidate.buildingClaims.end());
		merged.townClaims.insert(merged.townClaims.end(), p_candidate.dockClaims.begin(), p_candidate.dockClaims.end());
		merged.townClaims.insert(merged.townClaims.end(), p_candidate.sceneryClaims.begin(), p_candidate.sceneryClaims.end());
		const std::size_t firstRoadScenery = merged.placements.size();
		merged.placements.insert(merged.placements.end(), p_candidate.roadScenery.begin(), p_candidate.roadScenery.end());
		for (std::size_t i = 0; i < p_candidate.roadScenery.size(); ++i)
		{
			record.roadSceneryPlacementIndices.push_back(firstRoadScenery + i);
		}
		const std::size_t firstGroundScenery = merged.placements.size();
		merged.placements.insert(merged.placements.end(), p_candidate.groundScenery.begin(), p_candidate.groundScenery.end());
		for (std::size_t i = 0; i < p_candidate.groundScenery.size(); ++i)
		{
			record.groundSceneryPlacementIndices.push_back(firstGroundScenery + i);
		}
		// Compatibility only: macro consumers get a coarse derived mark, while all
		// planner and realization code consults exact paved columns above.
		for (const PlanPavedColumn &path : p_candidate.urbanRoadSurface)
		{
			const int row = merged.cellIndexFromWorld(path.worldZ);
			const int col = merged.cellIndexFromWorld(path.worldX);
			if (!merged.townPath.contains(row, col))
			{
				throw std::out_of_range("urban road lies outside the world plan");
			}
			merged.townPath.at(row, col) = 1;
		}
		merged.towns.push_back(std::move(record));
		p_plan = std::move(merged);
	}
}
