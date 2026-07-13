#include "world/generator/town_blueprint.hpp"

#include <algorithm>
#include <queue>
#include <set>
#include <stdexcept>

namespace pg
{
	namespace
	{
		LocalCell cell(const JsonReader &p_reader, const std::string &p_key)
		{
			const std::array<int, 2> value = p_reader.require<std::array<int, 2>>(p_key);
			return {value[0], value[1]};
		}
		LocalRect rect(const JsonReader &p_reader, const std::string &p_key)
		{
			JsonReader value = p_reader.child(p_key);
			value.forbidUnknown({"min", "max"});
			LocalRect result{cell(value, "min"), cell(value, "max")};
			if (result.min.row > result.max.row || result.min.col > result.max.col)
				throw JsonError(value.file(), value.pathFor("min"), "rectangle minimum exceeds maximum");
			return result;
		}
		bool contains(LocalRect p_bounds, LocalCell p_cell)
		{
			return p_cell.row >= p_bounds.min.row && p_cell.row <= p_bounds.max.row && p_cell.col >= p_bounds.min.col && p_cell.col <= p_bounds.max.col;
		}
		TownBlueprintRole role(JsonReader &p_reader)
		{
			return p_reader.requireEnum<TownBlueprintRole>("role", std::map<std::string, TownBlueprintRole>{{"creatureCenter", TownBlueprintRole::CreatureCenter}, {"shop", TownBlueprintRole::Shop}, {"gym", TownBlueprintRole::Gym}, {"port", TownBlueprintRole::Port}, {"home", TownBlueprintRole::Home}});
		}
	}

	LocalCell rotateTownCell(LocalCell p_cell, LocalRect p_bounds, int p_quarterTurns)
	{
		const int turns = ((p_quarterTurns % 4) + 4) % 4;
		const int row = p_cell.row - p_bounds.min.row, col = p_cell.col - p_bounds.min.col;
		const int rows = p_bounds.max.row - p_bounds.min.row, cols = p_bounds.max.col - p_bounds.min.col;
		switch (turns) { case 0: return p_cell; case 1: return {p_bounds.min.row + col, p_bounds.min.col + rows - row}; case 2: return {p_bounds.min.row + rows - row, p_bounds.min.col + cols - col}; default: return {p_bounds.min.row + cols - col, p_bounds.min.col + row}; }
	}
	LocalRect rotateTownRect(LocalRect p_rect, LocalRect p_bounds, int p_quarterTurns)
	{
		const std::array corners = {rotateTownCell(p_rect.min, p_bounds, p_quarterTurns), rotateTownCell({p_rect.min.row, p_rect.max.col}, p_bounds, p_quarterTurns), rotateTownCell({p_rect.max.row, p_rect.min.col}, p_bounds, p_quarterTurns), rotateTownCell(p_rect.max, p_bounds, p_quarterTurns)};
		LocalRect result{corners[0], corners[0]}; for (const LocalCell point : corners) { result.min.row = std::min(result.min.row, point.row); result.min.col = std::min(result.min.col, point.col); result.max.row = std::max(result.max.row, point.row); result.max.col = std::max(result.max.col, point.col); } return result;
	}
	std::string renderTownBlueprintAscii(const TownBlueprint &p_blueprint, int p_quarterTurns)
	{
		LocalRect bounds = rotateTownRect(p_blueprint.bounds, p_blueprint.bounds, p_quarterTurns); std::string result;
		for (int row = bounds.min.row; row <= bounds.max.row; ++row) { for (int col = bounds.min.col; col <= bounds.max.col; ++col) { const LocalCell local{row, col}; const bool boundary = std::ranges::any_of(p_blueprint.boundary, [&](LocalCell point) { return rotateTownCell(point, p_blueprint.bounds, p_quarterTurns) == local; }); result += boundary ? '#' : '.'; } result += '\n'; } return result;
	}
	TownBlueprint parseTownBlueprint(JsonReader &p_reader)
	{
		p_reader.forbidUnknown({"version", "kind", "bounds", "rotations", "endpoints", "lots", "paths", "decorationZones", "terraces", "stairs", "boundary", "waterfront"});
		if (p_reader.require<int>("version") != 1) throw JsonError(p_reader.file(), p_reader.pathFor("version"), "unsupported town blueprint version");
		TownBlueprint result; result.kind = p_reader.requireEnum<TownBlueprintKind>("kind", std::map<std::string, TownBlueprintKind>{{"city", TownBlueprintKind::City}, {"gym", TownBlueprintKind::Gym}, {"port", TownBlueprintKind::Port}}); result.bounds = rect(p_reader, "bounds"); result.allowedQuarterTurns = p_reader.require<std::vector<int>>("rotations");
		if (result.allowedQuarterTurns.empty()) throw JsonError(p_reader.file(), p_reader.pathFor("rotations"), "needs at least one rotation");
		std::set<int> rotations; for (int turn : result.allowedQuarterTurns) if (turn < 0 || turn > 3 || !rotations.insert(turn).second) throw JsonError(p_reader.file(), p_reader.pathFor("rotations"), "rotations must be unique quarter turns in [0, 3]");
		std::set<std::string> ids;
		for (JsonReader endpoint : p_reader.childArray("endpoints")) { endpoint.forbidUnknown({"id", "cell"}); LocalEndpoint value{endpoint.require<std::string>("id"), cell(endpoint, "cell")}; if (value.id.empty() || !ids.insert(value.id).second || !contains(result.bounds, value.cell)) throw JsonError(endpoint.file(), endpoint.path(), "endpoint has duplicate id or lies outside bounds"); result.endpoints.push_back(std::move(value)); }
		for (JsonReader lotReader : p_reader.childArray("lots")) { lotReader.forbidUnknown({"id", "role", "reservation", "approach", "required"}); TownLotTemplate lot{lotReader.require<std::string>("id"), role(lotReader), rect(lotReader, "reservation"), lotReader.require<std::string>("approach"), lotReader.optional<bool>("required", true)}; if (lot.id.empty() || !ids.insert(lot.id).second || !contains(result.bounds, lot.reservation.min) || !contains(result.bounds, lot.reservation.max)) throw JsonError(lotReader.file(), lotReader.path(), "lot has duplicate id or lies outside bounds"); result.lots.push_back(std::move(lot)); }
		std::set<std::string> pathIds;
		for (JsonReader pathReader : p_reader.childArray("paths")) { pathReader.forbidUnknown({"id", "from", "to", "surface"}); TownPathTemplate path{pathReader.require<std::string>("id"), pathReader.require<std::string>("from"), pathReader.require<std::string>("to"), {}}; for (const auto &point : pathReader.require<std::vector<std::array<int, 2>>>("surface")) { LocalCell local{point[0], point[1]}; if (!contains(result.bounds, local)) throw JsonError(pathReader.file(), pathReader.pathFor("surface"), "path surface lies outside bounds"); path.surface.push_back(local); } if (path.id.empty() || !pathIds.insert(path.id).second || path.surface.empty()) throw JsonError(pathReader.file(), pathReader.path(), "path needs unique id and non-empty surface"); result.paths.push_back(std::move(path)); }
		if (p_reader.contains("decorationZones")) for (JsonReader zoneReader : p_reader.childArray("decorationZones")) { zoneReader.forbidUnknown({"id", "cells"}); TownDecorationZone zone{.id = zoneReader.require<std::string>("id")}; if (zone.id.empty() || !ids.insert(zone.id).second) throw JsonError(zoneReader.file(), zoneReader.pathFor("id"), "decoration zone id must be unique"); for (const auto &point : zoneReader.require<std::vector<std::array<int,2>>>("cells")) { LocalCell local{point[0],point[1]}; if (!contains(result.bounds, local)) throw JsonError(zoneReader.file(), zoneReader.pathFor("cells"), "decoration cell lies outside bounds"); zone.cells.push_back(local); } if (zone.cells.empty()) throw JsonError(zoneReader.file(), zoneReader.pathFor("cells"), "decoration zone must not be empty"); result.decorationZones.push_back(std::move(zone)); }
		if(p_reader.contains("terraces")) for(JsonReader terraceReader:p_reader.childArray("terraces")) { terraceReader.forbidUnknown({"id","levelOffset","cells"}); TownTerraceTemplate terrace{.id=terraceReader.require<std::string>("id"),.levelOffset=terraceReader.require<int>("levelOffset")}; if(terrace.id.empty()||!ids.insert(terrace.id).second) throw JsonError(terraceReader.file(),terraceReader.pathFor("id"),"terrace id must be unique"); for(const auto &point:terraceReader.require<std::vector<std::array<int,2>>>("cells")){LocalCell local{point[0],point[1]};if(!contains(result.bounds,local))throw JsonError(terraceReader.file(),terraceReader.pathFor("cells"),"terrace cell lies outside bounds");terrace.cells.push_back(local);} if(terrace.cells.empty())throw JsonError(terraceReader.file(),terraceReader.pathFor("cells"),"terrace must not be empty");result.terraces.push_back(std::move(terrace)); }
		if(p_reader.contains("stairs")) for(JsonReader stairReader:p_reader.childArray("stairs")){stairReader.forbidUnknown({"id","lower","upper","expectedLevelDelta","reservation"});TownStairTemplate stair{.id=stairReader.require<std::string>("id"),.lower=stairReader.require<std::string>("lower"),.upper=stairReader.require<std::string>("upper"),.expectedLevelDelta=stairReader.require<int>("expectedLevelDelta"),.reservation=rect(stairReader,"reservation")};if(stair.id.empty()||!ids.insert(stair.id).second||stair.expectedLevelDelta<1||!contains(result.bounds,stair.reservation.min)||!contains(result.bounds,stair.reservation.max))throw JsonError(stairReader.file(),stairReader.path(),"stair is invalid");result.stairs.push_back(std::move(stair));}
		for (const auto &point : p_reader.require<std::vector<std::array<int, 2>>>("boundary")) { LocalCell local{point[0], point[1]}; if (!contains(result.bounds, local)) throw JsonError(p_reader.file(), p_reader.pathFor("boundary"), "boundary lies outside bounds"); result.boundary.push_back(local); }
		if (result.boundary.empty()) throw JsonError(p_reader.file(), p_reader.pathFor("boundary"), "boundary must not be empty");
		if (p_reader.contains("waterfront")) { JsonReader water = p_reader.child("waterfront"); water.forbidUnknown({"dock", "boarding"}); TownWaterfront value; for (const auto &point : water.require<std::vector<std::array<int,2>>>("dock")) { LocalCell local{point[0],point[1]}; if (!contains(result.bounds,local)) throw JsonError(water.file(),water.pathFor("dock"),"dock lies outside bounds"); value.dock.push_back(local); } value.boarding = cell(water,"boarding"); if (value.dock.empty() || !contains(result.bounds,value.boarding)) throw JsonError(water.file(),water.path(),"waterfront needs a dock and in-bounds boarding point"); result.waterfront = std::move(value); }
		std::set<std::string> endpointIds; for (const auto &endpoint : result.endpoints) endpointIds.insert(endpoint.id); for (const auto &lot : result.lots) if (!endpointIds.contains(lot.approach)) throw JsonError(p_reader.file(), p_reader.pathFor("lots"), "lot references missing approach endpoint"); for (const auto &path : result.paths) if (!endpointIds.contains(path.from) || !endpointIds.contains(path.to)) throw JsonError(p_reader.file(), p_reader.pathFor("paths"), "path references missing endpoint");
		if (!endpointIds.contains("town-entrance")) throw JsonError(p_reader.file(), p_reader.pathFor("endpoints"), "blueprint needs a town-entrance endpoint");
		for(const TownStairTemplate &stair:result.stairs) if(!endpointIds.contains(stair.lower)||!endpointIds.contains(stair.upper)) throw JsonError(p_reader.file(),p_reader.pathFor("stairs"),"stair references missing endpoint");
		std::set<std::string> reached{"town-entrance"}; bool changed = true; while (changed) { changed = false; for (const TownPathTemplate &path : result.paths) { if (reached.contains(path.from) && reached.insert(path.to).second) changed = true; if (reached.contains(path.to) && reached.insert(path.from).second) changed = true; } for(const TownStairTemplate &stair:result.stairs){if(reached.contains(stair.lower)&&reached.insert(stair.upper).second)changed=true;if(reached.contains(stair.upper)&&reached.insert(stair.lower).second)changed=true;} }
		for (const TownLotTemplate &lot : result.lots) if (lot.required && !reached.contains(lot.approach)) throw JsonError(p_reader.file(), p_reader.pathFor("paths"), "required lot approach is disconnected");
		const auto roleCount = [&](TownBlueprintRole p_role) { return std::ranges::count_if(result.lots, [&](const TownLotTemplate &p_lot) { return p_lot.required && p_lot.role == p_role; }); };
		if (roleCount(TownBlueprintRole::CreatureCenter) != 1 || roleCount(TownBlueprintRole::Shop) != 1 || roleCount(TownBlueprintRole::Home) < 2 || (result.kind == TownBlueprintKind::City && roleCount(TownBlueprintRole::Home) < 3) || (result.kind == TownBlueprintKind::Gym && roleCount(TownBlueprintRole::Gym) != 1) || (result.kind == TownBlueprintKind::Port && (roleCount(TownBlueprintRole::Port) != 1 || !result.waterfront.has_value()))) throw JsonError(p_reader.file(), p_reader.pathFor("lots"), "blueprint does not provide its kind's required roles");
		return result;
	}
}
