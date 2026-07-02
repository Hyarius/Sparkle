#include "world/map_definition.hpp"

#include "core/json.hpp"
#include "core/registry.hpp"
#include "voxel/voxel_registry.hpp"
#include "world/prefab_definition.hpp"
#include "world/voxel_content_parser.hpp"

#include <algorithm>
#include <array>
#include <set>

namespace
{
	[[nodiscard]] int orientationTurns(pg::VoxelOrientation p_orientation)
	{
		switch (p_orientation)
		{
		case pg::VoxelOrientation::PositiveZ: return 0;
		case pg::VoxelOrientation::PositiveX: return 1;
		case pg::VoxelOrientation::NegativeZ: return 2;
		case pg::VoxelOrientation::NegativeX: return 3;
		}
		return 0;
	}

	[[nodiscard]] pg::VoxelOrientation orientationFromTurns(int p_turns)
	{
		static constexpr std::array values = {
			pg::VoxelOrientation::PositiveZ,
			pg::VoxelOrientation::PositiveX,
			pg::VoxelOrientation::NegativeZ,
			pg::VoxelOrientation::NegativeX};
		return values[static_cast<std::size_t>(p_turns % 4)];
	}

	[[nodiscard]] spk::Vector3Int rotatePosition(
		const spk::Vector3Int &p_position,
		const spk::Vector3Int &p_size,
		pg::VoxelOrientation p_orientation)
	{
		switch (p_orientation)
		{
		case pg::VoxelOrientation::PositiveZ: return p_position;
		case pg::VoxelOrientation::PositiveX: return {p_position.z, p_position.y, p_size.x - 1 - p_position.x};
		case pg::VoxelOrientation::NegativeZ: return {p_size.x - 1 - p_position.x, p_position.y, p_size.z - 1 - p_position.z};
		case pg::VoxelOrientation::NegativeX: return {p_size.z - 1 - p_position.z, p_position.y, p_position.x};
		}
		return p_position;
	}

	void applyStamp(
		pg::MapDefinition &p_map,
		const pg::MapStamp &p_stamp,
		const pg::PrefabDefinition &p_prefab,
		const pg::JsonReader &p_reader)
	{
		for (int y = 0; y < p_prefab.size().y; ++y)
		{
			for (int z = 0; z < p_prefab.size().z; ++z)
			{
				for (int x = 0; x < p_prefab.size().x; ++x)
				{
					const spk::Vector3Int source{x, y, z};
					const spk::Vector3Int target = p_stamp.at + rotatePosition(source, p_prefab.size(), p_stamp.orientation);
					if (!p_map.grid.isWithinBounds(target))
					{
						throw pg::JsonError(p_reader.file(), p_reader.pathFor("at"), "rotated prefab extends outside the map");
					}
					pg::VoxelCell value = p_prefab.grid.cell(source);
					if (!value.isEmpty())
					{
						value.orientation = orientationFromTurns(
							orientationTurns(value.orientation) + orientationTurns(p_stamp.orientation));
					}
					p_map.grid.cell(target) = value;
				}
			}
		}
	}
}

namespace pg
{
	const MapMarker *MapDefinition::marker(const std::string &p_name) const noexcept
	{
		const auto iterator = std::ranges::find(markers, p_name, &MapMarker::name);
		return iterator == markers.end() ? nullptr : &*iterator;
	}

	MapDefinition parseMapDefinition(
		JsonReader &p_reader,
		const VoxelRegistry &p_voxels,
		const Registry<PrefabDefinition> &p_prefabs)
	{
		p_reader.forbidUnknown({"version", "size", "palette", "fill", "cells", "stamps", "markers", "portals", "biome"});
		if (p_reader.require<int>("version") != 1)
		{
			throw JsonError(p_reader.file(), p_reader.pathFor("version"), "unsupported map version");
		}
		const spk::Vector3Int size = detail::parseVector3(p_reader, "size");
		if (size.x <= 0 || size.y <= 0 || size.z <= 0)
		{
			throw JsonError(p_reader.file(), p_reader.pathFor("size"), "map dimensions must be positive");
		}

		MapDefinition result{.grid = VoxelGrid(size), .biome = p_reader.optional<std::string>("biome", "")};
		const detail::VoxelPalette palette = detail::parsePalette(p_reader, p_voxels);
		detail::applyVoxelContent(p_reader, result.grid, palette);

		if (p_reader.contains("stamps"))
		{
			for (JsonReader stampReader : p_reader.childArray("stamps"))
			{
				stampReader.forbidUnknown({"prefab", "at", "orientation"});
				MapStamp stamp{
					.prefab = stampReader.require<std::string>("prefab"),
					.at = detail::parseVector3(stampReader, "at"),
					.orientation = detail::parseOrientation(stampReader, "orientation")};
				const PrefabDefinition *prefab = p_prefabs.tryGet(stamp.prefab);
				if (prefab == nullptr)
				{
					throw JsonError(stampReader.file(), stampReader.pathFor("prefab"), "unknown prefab id '" + stamp.prefab + "'");
				}
				applyStamp(result, stamp, *prefab, stampReader);
				result.stamps.push_back(std::move(stamp));
			}
		}

		std::set<std::string> markerNames;
		if (p_reader.contains("markers"))
		{
			for (JsonReader markerReader : p_reader.childArray("markers"))
			{
				markerReader.forbidUnknown({"name", "at"});
				MapMarker marker{
					.name = markerReader.require<std::string>("name"),
					.at = detail::parseVector3(markerReader, "at")};
				if (!result.grid.isWithinBounds(marker.at))
				{
					throw JsonError(markerReader.file(), markerReader.pathFor("at"), "marker is outside the map");
				}
				if (!markerNames.insert(marker.name).second)
				{
					throw JsonError(markerReader.file(), markerReader.pathFor("name"), "duplicate marker name");
				}
				result.markers.push_back(std::move(marker));
			}
		}

		std::set<std::string> portalNames;
		if (p_reader.contains("portals"))
		{
			for (JsonReader portalReader : p_reader.childArray("portals"))
			{
				portalReader.forbidUnknown({"name", "at", "target"});
				JsonReader targetReader = portalReader.child("target");
				targetReader.forbidUnknown({"map", "portal"});
				MapPortal portal{
					.name = portalReader.require<std::string>("name"),
					.at = detail::parseVector3(portalReader, "at"),
					.target = {
						.map = targetReader.require<std::string>("map"),
						.portal = targetReader.require<std::string>("portal")}};
				if (!result.grid.isWithinBounds(portal.at))
				{
					throw JsonError(portalReader.file(), portalReader.pathFor("at"), "portal is outside the map");
				}
				if (!portalNames.insert(portal.name).second)
				{
					throw JsonError(portalReader.file(), portalReader.pathFor("name"), "duplicate portal name");
				}
				result.portals.push_back(std::move(portal));
			}
		}
		return result;
	}
}
