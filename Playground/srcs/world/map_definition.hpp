#pragma once

#include "voxel/voxel_grid.hpp"

#include <string>
#include <vector>

namespace pg
{
	class JsonReader;
	class VoxelRegistry;
	template <typename TDefinition>
	class Registry;
	struct PrefabDefinition;

	struct MapStamp
	{
		std::string prefab;
		spk::Vector3Int at{};
		VoxelOrientation orientation = VoxelOrientation::PositiveZ;
	};

	struct MapMarker
	{
		std::string name;
		spk::Vector3Int at{};
	};

	struct MapPortalTarget
	{
		std::string map;
		std::string portal;
	};

	struct MapPortal
	{
		std::string name;
		spk::Vector3Int at{};
		MapPortalTarget target;
	};

	struct MapDefinition
	{
		std::string id;
		VoxelGrid grid;
		std::vector<MapStamp> stamps;
		std::vector<MapMarker> markers;
		std::vector<MapPortal> portals;
		std::string biome;

		[[nodiscard]] const spk::Vector3Int &size() const noexcept { return grid.size(); }
		[[nodiscard]] const MapMarker *marker(const std::string &p_name) const noexcept;
	};

	[[nodiscard]] MapDefinition parseMapDefinition(
		JsonReader &p_reader,
		const VoxelRegistry &p_voxels,
		const Registry<PrefabDefinition> &p_prefabs);
}
