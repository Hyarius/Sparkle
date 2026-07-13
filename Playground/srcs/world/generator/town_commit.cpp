#include "world/generator/town_commit.hpp"

#include <stdexcept>

namespace pg
{
	void commitTownCandidate(WorldPlan &p_plan, const TownCandidate &p_candidate)
	{
		if (p_candidate.blueprintId.empty()) throw std::invalid_argument("cannot commit a town without a blueprint id");
		for (const PlanTownRecord &town : p_plan.towns) if (town.originRow == p_candidate.originRow && town.originCol == p_candidate.originCol) throw std::logic_error("town origin is already committed");
		WorldPlan merged = p_plan;
		PlanTownRecord record{.kind = p_candidate.kind, .macroEntityIndex = p_candidate.macroEntityIndex, .blueprintId = p_candidate.blueprintId, .quarterTurns = p_candidate.quarterTurns, .originRow = p_candidate.originRow, .originCol = p_candidate.originCol, .entranceCell = {p_candidate.entranceCell.row, p_candidate.entranceCell.col}};
		for (const LocalCell &cell : p_candidate.boundaryCells) record.boundaryCells.emplace_back(cell.row, cell.col);
		for (const LocalCell &cell : p_candidate.pathCells) record.pathCells.emplace_back(cell.row, cell.col);
		for (const LocalCell &cell : p_candidate.decorationCells) record.decorationCells.emplace_back(cell.row, cell.col);
		for (const LocalCell &cell : p_candidate.dockCells) record.dockCells.emplace_back(cell.row, cell.col);
		record.boardingEndpoint = p_candidate.boardingEndpoint;
		record.boatLinkIndices = p_candidate.boatLinkIndices;
		const std::size_t firstPlacement = merged.placements.size();
		for (const PrefabPlacement &building : p_candidate.buildings) merged.placements.push_back(building);
		for (std::size_t i = 0; i < p_candidate.buildings.size(); ++i) record.buildingPlacementIndices.push_back(firstPlacement + i);
		merged.townClaims.insert(merged.townClaims.end(), p_candidate.buildingClaims.begin(), p_candidate.buildingClaims.end());
		merged.townClaims.insert(merged.townClaims.end(),p_candidate.dockClaims.begin(),p_candidate.dockClaims.end());
		for(const StairCandidate &stair:p_candidate.stairs){PlanStairway stairRecord=stair.record;stairRecord.firstPlacement=merged.placements.size();stairRecord.placementCount=stair.placements.size();merged.placements.insert(merged.placements.end(),stair.placements.begin(),stair.placements.end());merged.townClaims.insert(merged.townClaims.end(),stair.claims.begin(),stair.claims.end());merged.stairways.push_back(std::move(stairRecord));}
		for (const LocalCell &path : p_candidate.pathCells)
		{
			if (!merged.townPath.contains(path.row, path.col)) throw std::out_of_range("town path lies outside the world plan");
			merged.townPath.at(path.row, path.col) = 1;
		}
		merged.towns.push_back(std::move(record));
		p_plan = std::move(merged);
	}
}
