#include "world/generator/town_planner.hpp"
#include "world/generator/world_plan_math.hpp"
#include "world/prefab_placement_math.hpp"
#include "structures/voxel/spk_voxel_orientation.hpp"

#include <set>

namespace pg
{
	TownMutationSnapshot snapshotTownMutation(const WorldPlan &p_plan) noexcept
	{
		std::size_t pathCells=0;for(int row=0;row<p_plan.townPath.size();++row)for(int col=0;col<p_plan.townPath.size();++col)pathCells+=p_plan.townPath.at(row,col)!=0;
		return {.placements = p_plan.placements.size(), .stairways = p_plan.stairways.size(), .portals = p_plan.portals.size(), .towns = p_plan.towns.size(), .claims = p_plan.townClaims.size(), .boatLinks=p_plan.boatLinks.size(),.townPathCells=pathCells, .warnings = p_plan.stats.warnings.size()};
	}
	bool matchesTownMutationSnapshot(const WorldPlan &p_plan, TownMutationSnapshot p_snapshot) noexcept
	{
		return snapshotTownMutation(p_plan) == p_snapshot;
	}
	std::vector<LocalCell> enumerateTownOrigins(LocalCell p_center, int p_radius)
	{
		std::vector<LocalCell> result;
		if (p_radius < 0) return result;
		result.push_back(p_center);
		for (int radius = 1; radius <= p_radius; ++radius)
		{
			for (int col = -radius; col <= radius; ++col) result.push_back({p_center.row - radius, p_center.col + col});
			for (int row = -radius + 1; row <= radius; ++row) result.push_back({p_center.row + row, p_center.col + radius});
			for (int col = radius - 1; col >= -radius; --col) result.push_back({p_center.row + radius, p_center.col + col});
			for (int row = radius - 1; row >= -radius + 1; --row) result.push_back({p_center.row + row, p_center.col - radius});
		}
		return result;
	}

	bool hasValidGlobalRoadArrival(const WorldPlan &p_plan, const TownCandidate &p_candidate) noexcept
	{
		const LocalCell entrance = p_candidate.entranceCell;
		if (!p_plan.road.contains(entrance.row, entrance.col) || p_plan.road.at(entrance.row, entrance.col) == 0)
		{
			return false;
		}
		if (std::ranges::find(p_candidate.pathCells, entrance) == p_candidate.pathCells.end())
		{
			return false;
		}
		return true;
	}

	TownPlanResult resolveFlatTownCandidate(const WorldPlan &p_plan, const TownBlueprint &p_blueprint, int p_originRow, int p_originCol, int p_quarterTurns)
	{
		TownCandidate candidate{.blueprintId = p_blueprint.id, .quarterTurns = p_quarterTurns, .originRow = p_originRow, .originCol = p_originCol};
		const LocalRect rotatedBounds = rotateTownRect(p_blueprint.bounds, p_blueprint.bounds, p_quarterTurns);
		int expectedHeight = -1;
		const auto resolve = [&](LocalCell p_local) { return LocalCell{p_originRow + p_local.row - rotatedBounds.min.row, p_originCol + p_local.col - rotatedBounds.min.col}; };
		const auto entrance = std::ranges::find_if(
			p_blueprint.endpoints,
			[](const LocalEndpoint &p_endpoint) { return p_endpoint.id == "town-entrance"; });
		if (entrance == p_blueprint.endpoints.end())
		{
			return {.rejection = TownRejection{TownRejectCategory::Disconnected, "town-entrance", {}}};
		}
		candidate.entranceCell = resolve(rotateTownCell(entrance->cell, p_blueprint.bounds, p_quarterTurns));
		for (LocalCell boundary : p_blueprint.boundary)
		{
			const LocalCell world = resolve(rotateTownCell(boundary, p_blueprint.bounds, p_quarterTurns));
			if (!p_plan.land.contains(world.row, world.col)) return {.rejection = TownRejection{TownRejectCategory::OutOfBounds, "boundary", world}};
			if (p_plan.land.at(world.row, world.col) == 0 || p_plan.water.at(world.row, world.col) != 0) return {.rejection = TownRejection{TownRejectCategory::Water, "boundary", world}};
			if (expectedHeight < 0) expectedHeight = p_plan.height.at(world.row, world.col);
		}
		if(!p_blueprint.terraces.empty())
		{
			std::optional<int> base;
			for(const TownTerraceTemplate &terrace:p_blueprint.terraces) for(LocalCell local:terrace.cells){LocalCell world=resolve(rotateTownCell(local,p_blueprint.bounds,p_quarterTurns));if(!p_plan.land.contains(world.row,world.col)||p_plan.land.at(world.row,world.col)==0||p_plan.water.at(world.row,world.col)!=0)return {.rejection=TownRejection{TownRejectCategory::Water,terrace.id,world}};if(terrace.levelOffset==0&&!base)base=p_plan.height.at(world.row,world.col);}
			if(!base)return {.rejection=TownRejection{TownRejectCategory::TerraceHeight,"terraces",{}}};
			for(const TownTerraceTemplate &terrace:p_blueprint.terraces)for(LocalCell local:terrace.cells){LocalCell world=resolve(rotateTownCell(local,p_blueprint.bounds,p_quarterTurns));if(p_plan.height.at(world.row,world.col)!=*base+terrace.levelOffset)return {.rejection=TownRejection{TownRejectCategory::TerraceHeight,terrace.id,world}};}
		}
		for (LocalCell local : p_blueprint.boundary) candidate.boundaryCells.push_back(resolve(rotateTownCell(local, p_blueprint.bounds, p_quarterTurns)));
		for(const PlanTownRecord &town:p_plan.towns)for(const auto &[row,col]:town.boundaryCells)if(std::ranges::find(candidate.boundaryCells,LocalCell{row,col})!=candidate.boundaryCells.end())return {.rejection=TownRejection{TownRejectCategory::ClaimConflict,"boundary",{row,col}}};
		std::set<std::pair<int, int>> paths; for (const TownPathTemplate &path : p_blueprint.paths) for (LocalCell local : path.surface) { const LocalCell world = resolve(rotateTownCell(local, p_blueprint.bounds, p_quarterTurns)); if (paths.insert({world.row, world.col}).second) candidate.pathCells.push_back(world); }
		if (!candidate.pathCells.empty()) expectedHeight=p_plan.height.at(candidate.pathCells.front().row,candidate.pathCells.front().col);
		for (const LocalCell &world : candidate.pathCells) if (!p_plan.land.contains(world.row,world.col) || p_plan.land.at(world.row,world.col) == 0 || p_plan.water.at(world.row,world.col) != 0) return {.rejection = TownRejection{TownRejectCategory::Water,"path",world}}; else if(p_blueprint.terraces.empty() && p_blueprint.kind!=TownBlueprintKind::Port && p_plan.height.at(world.row,world.col)!=expectedHeight) return {.rejection=TownRejection{TownRejectCategory::TerraceHeight,"path",world}};
		std::set<std::pair<int,int>> decorations; for (const TownDecorationZone &zone : p_blueprint.decorationZones) for (LocalCell local : zone.cells) { LocalCell world = resolve(rotateTownCell(local,p_blueprint.bounds,p_quarterTurns)); if (decorations.insert({world.row,world.col}).second) candidate.decorationCells.push_back(world); }
		if (p_blueprint.waterfront) { for (LocalCell local : p_blueprint.waterfront->dock) { LocalCell world=resolve(rotateTownCell(local,p_blueprint.bounds,p_quarterTurns)); if (!p_plan.land.contains(world.row,world.col)) return {.rejection=TownRejection{TownRejectCategory::WaterfrontMismatch,"dock",world}};candidate.dockCells.push_back(world); } LocalCell boarding=resolve(rotateTownCell(p_blueprint.waterfront->boarding,p_blueprint.bounds,p_quarterTurns));if(p_plan.land.at(boarding.row,boarding.col)!=0&&p_plan.water.at(boarding.row,boarding.col)==0)return {.rejection=TownRejection{TownRejectCategory::WaterfrontMismatch,"boarding",boarding}}; const int blocks=p_plan.config.blocksPerCell, offset=p_plan.worldOffset(); candidate.boardingEndpoint=spk::Vector3Int{offset+boarding.col*blocks+blocks/2,p_plan.surfaceY(expectedHeight)+1,offset+boarding.row*blocks+blocks/2};for(const LocalCell &dock:candidate.dockCells)candidate.dockClaims.push_back({.min={offset+dock.col*blocks,candidate.boardingEndpoint->y-1,offset+dock.row*blocks},.max={offset+(dock.col+1)*blocks-1,candidate.boardingEndpoint->y,offset+(dock.row+1)*blocks-1}}); }
		return {.candidate = std::move(candidate)};
	}

	TownPlanResult resolveFlatTownCandidate(const WorldPlan &p_plan, const TownBlueprint &p_blueprint, const TownContentContext &p_content, int p_originRow, int p_originCol, int p_quarterTurns)
	{
		TownPlanResult result = resolveFlatTownCandidate(p_plan, p_blueprint, p_originRow, p_originCol, p_quarterTurns);
		if (!result.candidate) return result;
		TownCandidate &candidate = *result.candidate;
		for(std::size_t first=0; first<p_blueprint.lots.size(); ++first) for(std::size_t second=first+1; second<p_blueprint.lots.size(); ++second) if(p_blueprint.lots[first].reservation.min==p_blueprint.lots[second].reservation.min && p_blueprint.lots[first].reservation.max==p_blueprint.lots[second].reservation.max) return {.rejection=TownRejection{TownRejectCategory::ClaimConflict,p_blueprint.lots[second].id,p_blueprint.lots[second].reservation.min}};
		const auto prefabFor = [&](TownBlueprintRole role, const std::string &lotId) -> std::string {
			switch (role) { case TownBlueprintRole::CreatureCenter: return p_content.town.creatureCenter; case TownBlueprintRole::Shop: return p_content.town.shop; case TownBlueprintRole::Gym: return p_content.town.gym; case TownBlueprintRole::Port: return p_content.town.port; case TownBlueprintRole::Home: { if (p_content.town.homes.empty()) return {}; worldgen::Rng rng(worldgen::deriveSeed(p_content.worldSeed, "town/" + p_content.stableEntityId + "/" + p_blueprint.id + "/" + lotId)); return p_content.town.homes[static_cast<std::size_t>(rng.below(static_cast<int>(p_content.town.homes.size())))]; } } return {}; };
		const int blocks = p_plan.config.blocksPerCell; const int offset = p_plan.worldOffset();
		const LocalRect candidateBounds = rotateTownRect(p_blueprint.bounds, p_blueprint.bounds, p_quarterTurns);
		const auto worldOf = [&](LocalCell p_local) {
			p_local = rotateTownCell(p_local, p_blueprint.bounds, p_quarterTurns);
			return LocalCell{p_originRow + p_local.row - candidateBounds.min.row, p_originCol + p_local.col - candidateBounds.min.col};
		};
		for (const TownLotTemplate &lot : p_blueprint.lots)
		{
			const std::string prefabId = prefabFor(lot.role, lot.id);
			const PrefabDefinition *prefab = prefabId.empty() ? nullptr : p_content.prefabs.tryGet(prefabId);
			if (prefab == nullptr) return {.rejection = TownRejection{TownRejectCategory::MissingPrefab, lot.id, {}}};
			const PrefabEntrance *entrance=prefab->entrance ? &*prefab->entrance : nullptr;
			const PrefabAnchor *door = entrance ? prefab->tryAnchor(entrance->anchorName) : nullptr;
			if (door == nullptr || entrance->outwardFacing!=spk::VoxelOrientation::NegativeZ || door->position.z != 0) return {.rejection = TownRejection{TownRejectCategory::DoorMismatch, lot.id, {}}};
			const LocalRect rotated = rotateTownRect(lot.reservation, p_blueprint.bounds, p_quarterTurns);
			const LocalRect rotatedBounds = rotateTownRect(p_blueprint.bounds,p_blueprint.bounds,p_quarterTurns);
			const auto endpoint = std::ranges::find_if(p_blueprint.endpoints,[&](const LocalEndpoint &p_endpoint){return p_endpoint.id==lot.approach;});
			if (endpoint == p_blueprint.endpoints.end()) return {.rejection=TownRejection{TownRejectCategory::Disconnected,lot.id,{}}};
			const LocalCell doorCell=rotateTownCell(endpoint->cell,p_blueprint.bounds,p_quarterTurns);
			const int row = p_originRow + doorCell.row-rotatedBounds.min.row;
			const int col = p_originCol + doorCell.col-rotatedBounds.min.col;
			if (!p_plan.land.contains(row, col)) return {.rejection = TownRejection{TownRejectCategory::OutOfBounds, lot.id, {row,col}}};
			// Face the building toward the authored street segment at its approach.
			// Lots around the same square deliberately face different directions, so
			// inheriting the town's rotation would only be correct for one facade.
			int buildingTurns = p_quarterTurns;
			for (const TownPathTemplate &path : p_blueprint.paths)
			{
				if (path.from != lot.approach && path.to != lot.approach)
				{
					continue;
				}
				std::vector<LocalCell> surface;
				for (LocalCell local : path.surface)
				{
					surface.push_back(worldOf(local));
				}
				const auto doorOnSurface = std::ranges::find(surface, LocalCell{row, col});
				if (doorOnSurface == surface.end() || surface.size() < 2)
				{
					continue;
				}
				const std::size_t index = static_cast<std::size_t>(doorOnSurface - surface.begin());
				const LocalCell neighbour = index == 0 ? surface[1] : surface[index - 1];
				const int dr = neighbour.row - row;
				const int dc = neighbour.col - col;
				if (std::abs(dr) + std::abs(dc) != 1)
				{
					return {.rejection = TownRejection{TownRejectCategory::Disconnected, lot.id, {row, col}}};
				}
				// The canonical prefab door faces local -Z. These turns rotate it
				// toward north, west, south, and east respectively.
				buildingTurns = dr < 0 ? 0 : dc < 0 ? 1 : dr > 0 ? 2 : 3;
				break;
			}
			const spk::VoxelOrientation orientation=spk::orientationFromQuarterTurns(buildingTurns); const spk::Vector3Int doorDelta=spk::rotateQuarterTurns(door->position-prefab->prefab.pivot(),buildingTurns);
			int doorX=offset+col*blocks+blocks/2,doorZ=offset+row*blocks+blocks/2;switch(orientation){case spk::VoxelOrientation::PositiveZ:doorZ=offset+row*blocks;break;case spk::VoxelOrientation::NegativeZ:doorZ=offset+(row+1)*blocks-1;break;case spk::VoxelOrientation::PositiveX:doorX=offset+col*blocks;break;case spk::VoxelOrientation::NegativeX:doorX=offset+(col+1)*blocks-1;break;}
			// The authored door anchor is the exterior floor block (local y = -1),
			// matching legacy/default placement where the prefab's y=0 layer starts at
			// surface+1. Aligning it to surface+1 would lift the whole building one block.
			const spk::Vector3Int desiredDoor{doorX, p_plan.surfaceY(p_plan.height.at(row,col)), doorZ};
			PrefabPlacement placement{.prefabId = prefabId, .anchor = desiredDoor-doorDelta, .orientation = orientation, .foundation = true, .anchorToPivot = true};
			switch(lot.role){case TownBlueprintRole::CreatureCenter:placement.townRole=TownBuildingRole::CreatureCenter;break;case TownBlueprintRole::Shop:placement.townRole=TownBuildingRole::Shop;break;case TownBlueprintRole::Gym:placement.townRole=TownBuildingRole::Gym;break;case TownBlueprintRole::Port:placement.townRole=TownBuildingRole::Port;break;case TownBlueprintRole::Home:placement.townRole=TownBuildingRole::Home;break;}
			const ResolvedPlacementBox box = resolvePlacement(prefab->prefab, placement); const spk::Vector3Int pivot = prefab->prefab.pivot(); const spk::Vector3Int low = prefab->clearance ? prefab->clearance->min : prefab->prefab.minBounds(); const spk::Vector3Int high = prefab->clearance ? prefab->clearance->max : prefab->prefab.maxBounds(); const int turns = spk::quarterTurnsOf(placement.orientation); const auto a = spk::rotateQuarterTurns(low-pivot,turns); const auto b = spk::rotateQuarterTurns(high-pivot,turns); PlanClaim claim{box.destination + spk::Vector3Int{std::min(a.x,b.x),std::min(a.y,b.y),std::min(a.z,b.z)}, box.destination + spk::Vector3Int{std::max(a.x,b.x),std::max(a.y,b.y),std::max(a.z,b.z)}};
			const int reservationMinRow=p_originRow+rotated.min.row-rotatedBounds.min.row,reservationMaxRow=p_originRow+rotated.max.row-rotatedBounds.min.row,reservationMinCol=p_originCol+rotated.min.col-rotatedBounds.min.col,reservationMaxCol=p_originCol+rotated.max.col-rotatedBounds.min.col;if(p_plan.cellIndexFromWorld(claim.min.z)<reservationMinRow||p_plan.cellIndexFromWorld(claim.max.z)>reservationMaxRow||p_plan.cellIndexFromWorld(claim.min.x)<reservationMinCol||p_plan.cellIndexFromWorld(claim.max.x)>reservationMaxCol)return {.rejection=TownRejection{TownRejectCategory::ClaimConflict,lot.id,{row,col}}};
			const auto overlaps=[&](const PlanClaim &other){return claim.min.x <= other.max.x && claim.max.x >= other.min.x && claim.min.y <= other.max.y && claim.max.y >= other.min.y && claim.min.z <= other.max.z && claim.max.z >= other.min.z;};
			for(int footprintRow=p_plan.cellIndexFromWorld(claim.min.z); footprintRow<=p_plan.cellIndexFromWorld(claim.max.z); ++footprintRow) for(int footprintCol=p_plan.cellIndexFromWorld(claim.min.x); footprintCol<=p_plan.cellIndexFromWorld(claim.max.x); ++footprintCol) if(!p_plan.land.contains(footprintRow,footprintCol) || p_plan.land.at(footprintRow,footprintCol)==0 || p_plan.water.at(footprintRow,footprintCol)!=0 || p_plan.height.at(footprintRow,footprintCol)!=p_plan.height.at(row,col)) return {.rejection=TownRejection{TownRejectCategory::TerraceHeight,lot.id,{footprintRow,footprintCol}}};
			for (const PlanClaim &other : candidate.buildingClaims) if (overlaps(other)) return {.rejection = TownRejection{TownRejectCategory::ClaimConflict, lot.id, {row,col}}};
			for (const PlanClaim &other : p_plan.townClaims) if (overlaps(other)) return {.rejection = TownRejection{TownRejectCategory::ClaimConflict, lot.id, {row,col}}};
			candidate.buildings.push_back(std::move(placement)); candidate.buildingClaims.push_back(claim);
		}
		for(const TownStairTemplate &stair:p_blueprint.stairs)
		{
			if(p_content.roadStairPrefabs==nullptr||p_content.roadStairPrefabs->empty())return {.rejection=TownRejection{TownRejectCategory::MissingPrefab,stair.id,{}}};
			const auto lowerEndpoint=std::ranges::find_if(p_blueprint.endpoints,[&](const LocalEndpoint &endpoint){return endpoint.id==stair.lower;});const auto upperEndpoint=std::ranges::find_if(p_blueprint.endpoints,[&](const LocalEndpoint &endpoint){return endpoint.id==stair.upper;});
			const LocalRect rotatedBounds=rotateTownRect(p_blueprint.bounds,p_blueprint.bounds,p_quarterTurns);const auto worldOf=[&](LocalCell local){local=rotateTownCell(local,p_blueprint.bounds,p_quarterTurns);return LocalCell{p_originRow+local.row-rotatedBounds.min.row,p_originCol+local.col-rotatedBounds.min.col};};const LocalCell lower=worldOf(lowerEndpoint->cell),upper=worldOf(upperEndpoint->cell);const int delta=p_plan.height.at(upper.row,upper.col)-p_plan.height.at(lower.row,lower.col);if(delta!=stair.expectedLevelDelta||std::abs(upper.row-lower.row)+std::abs(upper.col-lower.col)!=1)return {.rejection=TownRejection{TownRejectCategory::TerraceHeight,stair.id,upper}};
			spk::VoxelOrientation orientation=upper.row<lower.row?spk::VoxelOrientation::NegativeZ:upper.row>lower.row?spk::VoxelOrientation::PositiveZ:upper.col<lower.col?spk::VoxelOrientation::NegativeX:spk::VoxelOrientation::PositiveX;const int blocks=p_plan.config.blocksPerCell,offset=p_plan.worldOffset();PrefabPlacement placement{.prefabId=p_content.roadStairPrefabs->front(),.anchor={offset+lower.col*blocks+blocks/2,p_plan.surfaceY(p_plan.height.at(lower.row,lower.col))+1,offset+lower.row*blocks+blocks/2},.orientation=orientation,.foundation=true};PlanStairway record{.topAnchor={offset+upper.col*blocks+blocks/2,p_plan.surfaceY(p_plan.height.at(upper.row,upper.col))+1,offset+upper.row*blocks+blocks/2},.bottomAnchor=placement.anchor,.plateauCell={upper.col,0,upper.row},.centerPath={placement.anchor},.lowRow=lower.row,.lowCol=lower.col,.steps=delta,.road=true};record.centerPath.push_back(record.topAnchor);auto planned=planStair({.placements={placement},.record=record},{.plan=p_plan,.prefabs=p_content.prefabs});if(!planned)return {.rejection=TownRejection{TownRejectCategory::ClaimConflict,stair.id,lower}};for(const PlanClaim &claim:planned->claims){for(const PlanClaim &building:candidate.buildingClaims)if(claim.min.x<=building.max.x&&claim.max.x>=building.min.x&&claim.min.z<=building.max.z&&claim.max.z>=building.min.z)return {.rejection=TownRejection{TownRejectCategory::ClaimConflict,stair.id,lower}};planned->record.footprints.push_back({claim.min.x,claim.max.x,claim.min.z,claim.max.z});}candidate.stairs.push_back(std::move(*planned));
		}
		return result;
	}
}
