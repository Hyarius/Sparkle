#include "world/generator/town_composition.hpp"

#include <array>
#include <map>
#include <set>

namespace pg
{
	namespace
	{
		constexpr int kMaximumCount = 64;
		constexpr int kMaximumSpacing = 128;
		constexpr int kMaximumAttempts = 512;

		CountRange parseCount(const JsonReader &p_reader)
		{
			const spk::JSON::Value *value = p_reader.value().find("count");
			if (value == nullptr)
			{
				throw JsonError(p_reader.file(), p_reader.pathFor("count"), "missing required count");
			}
			try
			{
				if (value->isInteger())
				{
					const int exact = value->as<int>();
					return {exact, exact};
				}
				JsonReader range(*value, p_reader.file(), p_reader.pathFor("count"));
				range.forbidUnknown({"min", "max"});
				return {range.require<int>("min"), range.require<int>("max")};
			}
			catch (const JsonError &)
			{
				throw;
			}
			catch (const std::exception &error)
			{
				throw JsonError(p_reader.file(), p_reader.pathFor("count"), std::string("invalid count: ") + error.what());
			}
		}

		void validateCount(const JsonReader &p_reader, CountRange p_count)
		{
			if (p_count.minimum < 0 || p_count.maximum < 0 || p_count.minimum > p_count.maximum ||
				p_count.maximum > kMaximumCount)
			{
				throw JsonError(p_reader.file(), p_reader.pathFor("count"), "count must be ordered and between 0 and 64");
			}
		}

		void validateLayout(const JsonReader &p_reader, const TownLayoutSettings &p_layout)
		{
			for (const auto &[name, width] : std::array{
					std::pair{"mainRoadWidth", p_layout.mainRoadWidth},
					std::pair{"urbanRoadWidth", p_layout.urbanRoadWidth}})
			{
				if (width < 1 || width % 2 == 0 || width > 15)
					throw JsonError(p_reader.file(), p_reader.pathFor(name), "road widths must be odd and between 1 and 15");
			}
			if (p_layout.minimumBuildingSpacing < 0 || p_layout.minimumBuildingSpacing > kMaximumSpacing)
				throw JsonError(p_reader.file(), p_reader.pathFor("minimumBuildingSpacing"), "minimumBuildingSpacing is outside supported bounds");
			if (p_layout.buildingAttemptsPerItem < 1 || p_layout.buildingAttemptsPerItem > kMaximumAttempts ||
				p_layout.layoutAttempts < 1 || p_layout.layoutAttempts > kMaximumAttempts)
				throw JsonError(p_reader.file(), p_reader.path(), "buildingAttemptsPerItem and layoutAttempts must be between 1 and 512");
			if (p_layout.routeTurnPenalty < 0 || p_layout.routeSlopePenalty < 0)
				throw JsonError(p_reader.file(), p_reader.path(), "route penalties must be non-negative");
		}

		int requiredRoleCount(const TownComposition &p_composition, TownBuildingRole p_role)
		{
			int result = 0;
			for (const TownBuildingRequest &request : p_composition.buildings)
			{
				if (request.required && request.role == p_role)
				{
					result += request.count.minimum;
				}
			}
			return result;
		}
	}

	std::string townCompositionKindName(TownCompositionKind p_kind)
	{
		switch (p_kind)
		{
		case TownCompositionKind::City: return "city";
		case TownCompositionKind::Gym: return "gym";
		case TownCompositionKind::Port: return "port";
		}
		return "unknown";
	}

	std::string townBuildingRoleName(TownBuildingRole p_role)
	{
		switch (p_role)
		{
		case TownBuildingRole::CreatureCenter: return "creatureCenter";
		case TownBuildingRole::Shop: return "shop";
		case TownBuildingRole::Gym: return "gym";
		case TownBuildingRole::Port: return "port";
		case TownBuildingRole::Home: return "home";
		case TownBuildingRole::None: return "none";
		}
		return "unknown";
	}

	TownComposition parseTownComposition(JsonReader &p_reader)
	{
		p_reader.forbidUnknown({"version", "kind", "layout", "buildings", "roadScenery", "groundScenery"});
		if (p_reader.require<int>("version") != 1)
			throw JsonError(p_reader.file(), p_reader.pathFor("version"), "unsupported town composition version");

		TownComposition result;
		result.kind = p_reader.requireEnum<TownCompositionKind>("kind", std::map<std::string, TownCompositionKind>{{"city", TownCompositionKind::City}, {"gym", TownCompositionKind::Gym}, {"port", TownCompositionKind::Port}});
		JsonReader layout = p_reader.child("layout");
		layout.forbidUnknown({"mainRoadWidth", "urbanRoadWidth", "minimumBuildingSpacing", "buildingAttemptsPerItem", "layoutAttempts", "routeTurnPenalty", "routeSlopePenalty"});
		result.layout = {
			.mainRoadWidth = layout.require<int>("mainRoadWidth"),
			.urbanRoadWidth = layout.require<int>("urbanRoadWidth"),
			.minimumBuildingSpacing = layout.require<int>("minimumBuildingSpacing"),
			.buildingAttemptsPerItem = layout.require<int>("buildingAttemptsPerItem"),
			.layoutAttempts = layout.require<int>("layoutAttempts"),
			.routeTurnPenalty = layout.require<int>("routeTurnPenalty"),
			.routeSlopePenalty = layout.require<int>("routeSlopePenalty")};
		validateLayout(layout, result.layout);

		std::set<std::string> ids;
		for (JsonReader requestReader : p_reader.childArray("buildings"))
		{
			requestReader.forbidUnknown({"id", "role", "count", "required", "minimumSpacing", "placementPriority"});
			TownBuildingRequest request{
				.id = requestReader.require<std::string>("id"),
				.role = requestReader.requireEnum<TownBuildingRole>("role", std::map<std::string, TownBuildingRole>{{"creatureCenter", TownBuildingRole::CreatureCenter}, {"shop", TownBuildingRole::Shop}, {"gym", TownBuildingRole::Gym}, {"port", TownBuildingRole::Port}, {"home", TownBuildingRole::Home}}),
				.count = parseCount(requestReader),
				.required = requestReader.optional<bool>("required", true),
				.minimumSpacing = requestReader.optional<int>("minimumSpacing", result.layout.minimumBuildingSpacing),
				.placementPriority = requestReader.optional<int>("placementPriority", 0)};
			if (request.id.empty() || !ids.insert(request.id).second)
				throw JsonError(requestReader.file(), requestReader.pathFor("id"), "building request ids must be non-empty and unique");
			validateCount(requestReader, request.count);
			if (request.minimumSpacing < 0 || request.minimumSpacing > kMaximumSpacing)
				throw JsonError(requestReader.file(), requestReader.pathFor("minimumSpacing"), "minimumSpacing is outside supported bounds");
			if (request.required && request.count.minimum == 0)
				throw JsonError(requestReader.file(), requestReader.pathFor("count"), "a required building request needs a positive minimum count");
			result.buildings.push_back(std::move(request));
		}
		if (result.buildings.empty())
			throw JsonError(p_reader.file(), p_reader.pathFor("buildings"), "a composition needs building requests");

		const auto parseScenery = [&](const std::string &key, std::vector<TownSceneryRequest> &out) {
			if (!p_reader.contains(key)) return;
			for (JsonReader requestReader : p_reader.childArray(key))
			{
				requestReader.forbidUnknown({"id", "prefab", "count", "spacing", "required"});
				TownSceneryRequest request{.id = requestReader.require<std::string>("id"), .prefabId = requestReader.require<std::string>("prefab"), .count = parseCount(requestReader), .spacing = requestReader.require<int>("spacing"), .required = requestReader.optional<bool>("required", false)};
				if (request.id.empty() || request.prefabId.empty() || !ids.insert(request.id).second)
					throw JsonError(requestReader.file(), requestReader.path(), "scenery request ids and prefab ids must be non-empty and globally unique");
				validateCount(requestReader, request.count);
				if (request.spacing < 1 || request.spacing > kMaximumSpacing)
					throw JsonError(requestReader.file(), requestReader.pathFor("spacing"), "spacing must be between 1 and 128");
				if (request.required && request.count.minimum == 0)
					throw JsonError(requestReader.file(), requestReader.pathFor("count"), "required scenery needs a positive minimum count");
				out.push_back(std::move(request));
			}
		};
		parseScenery("roadScenery", result.roadScenery);
		parseScenery("groundScenery", result.groundScenery);

		if (requiredRoleCount(result, TownBuildingRole::CreatureCenter) != 1 || requiredRoleCount(result, TownBuildingRole::Shop) != 1)
			throw JsonError(p_reader.file(), p_reader.pathFor("buildings"), "every town composition needs exactly one required creatureCenter and shop");
		if (result.kind == TownCompositionKind::Gym && requiredRoleCount(result, TownBuildingRole::Gym) != 1)
			throw JsonError(p_reader.file(), p_reader.pathFor("buildings"), "a gym composition needs exactly one required gym");
		if (result.kind == TownCompositionKind::Port && requiredRoleCount(result, TownBuildingRole::Port) != 1)
			throw JsonError(p_reader.file(), p_reader.pathFor("buildings"), "a port composition needs exactly one required port");
		return result;
	}
}
