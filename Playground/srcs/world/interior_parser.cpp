#include "world/interior_definition.hpp"

#include "core/json.hpp"
#include "core/registry.hpp"
#include "world/prefab_definition.hpp"

#include <algorithm>

namespace pg
{
	namespace
	{
		// Entry rooms must carry the two portal pads; every room needs at least one
		// connector or it could never join the composition.
		void validateRoomPrefab(
			const JsonReader &p_reader,
			const std::string &p_path,
			const Registry<PrefabDefinition> &p_prefabs,
			const std::string &p_prefabId,
			bool p_entry)
		{
			const PrefabDefinition *prefab = p_prefabs.tryGet(p_prefabId);
			if (prefab == nullptr)
			{
				throw JsonError(p_reader.file(), p_path, "unknown room prefab '" + p_prefabId + "'");
			}
			if (p_entry && (prefab->tryAnchor("entry") == nullptr || prefab->tryAnchor("exit") == nullptr))
			{
				throw JsonError(
					p_reader.file(), p_path, "entry room '" + p_prefabId + "' needs 'entry' and 'exit' anchors");
			}
			const bool hasConnector = std::ranges::any_of(prefab->anchors, [](const PrefabAnchor &p_anchor) {
				return p_anchor.name.starts_with("connector:");
			});
			if (!p_entry && !hasConnector)
			{
				throw JsonError(
					p_reader.file(), p_path, "room prefab '" + p_prefabId + "' has no connector anchor");
			}
		}
	}

	InteriorDefinition parseInteriorDefinition(
		JsonReader &p_reader,
		const Registry<PrefabDefinition> &p_prefabs)
	{
		p_reader.forbidUnknown({"version", "entry", "rooms", "minRooms", "maxRooms"});
		if (p_reader.require<int>("version") != 1)
		{
			throw JsonError(p_reader.file(), p_reader.pathFor("version"), "unsupported interior version");
		}

		InteriorDefinition result;
		result.entryPrefabId = p_reader.require<std::string>("entry");
		validateRoomPrefab(p_reader, p_reader.pathFor("entry"), p_prefabs, result.entryPrefabId, true);

		for (JsonReader roomReader : p_reader.childArray("rooms"))
		{
			roomReader.forbidUnknown({"prefab", "weight"});
			InteriorRoomOption option{
				.prefabId = roomReader.require<std::string>("prefab"),
				.weight = roomReader.optional<double>("weight", 1.0)};
			if (option.weight <= 0.0)
			{
				throw JsonError(roomReader.file(), roomReader.pathFor("weight"), "weight must be positive");
			}
			validateRoomPrefab(roomReader, roomReader.pathFor("prefab"), p_prefabs, option.prefabId, false);
			result.rooms.push_back(std::move(option));
		}

		result.minRooms = p_reader.optional<int>("minRooms", 1);
		result.maxRooms = p_reader.optional<int>("maxRooms", 3);
		if (result.minRooms < 0 || result.maxRooms < result.minRooms)
		{
			throw JsonError(p_reader.file(), p_reader.pathFor("minRooms"), "need 0 <= minRooms <= maxRooms");
		}
		if (result.maxRooms > 0 && result.rooms.empty())
		{
			throw JsonError(p_reader.file(), p_reader.pathFor("rooms"), "a growing interior needs at least one room");
		}
		return result;
	}
}
