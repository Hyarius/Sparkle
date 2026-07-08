#include "world/prefab_definition.hpp"

#include "core/json.hpp"
#include "voxel/voxel_registry.hpp"
#include "world/voxel_content_parser.hpp"

#include <algorithm>
#include <set>

namespace pg
{
	PrefabDefinition parsePrefabDefinition(JsonReader &p_reader, const VoxelRegistry &p_voxels)
	{
		p_reader.forbidUnknown({"version", "pivot", "palette", "fill", "cells", "anchors"});
		if (p_reader.require<int>("version") != 1)
		{
			throw JsonError(p_reader.file(), p_reader.pathFor("version"), "unsupported prefab version");
		}

		// The prefab box is deduced from the authored content. Coordinates are relative
		// to the prefab's pivot and may be negative: layers below y = 0 embed under the
		// stamp destination (a floor slab, a POI pedestal) while y = 0 stays the ground
		// level.
		bool hasContent = false;
		spk::Vector3Int lowest{};
		spk::Vector3Int highest{};
		const auto grow = [&](const spk::Vector3Int &p_position) {
			if (!hasContent)
			{
				lowest = p_position;
				highest = p_position;
				hasContent = true;
				return;
			}
			lowest = {std::min(lowest.x, p_position.x), std::min(lowest.y, p_position.y), std::min(lowest.z, p_position.z)};
			highest = {std::max(highest.x, p_position.x), std::max(highest.y, p_position.y), std::max(highest.z, p_position.z)};
		};
		if (p_reader.contains("fill"))
		{
			for (JsonReader fill : p_reader.childArray("fill"))
			{
				grow(detail::parseVector3(fill, "min"));
				grow(detail::parseVector3(fill, "max"));
			}
		}
		if (p_reader.contains("cells"))
		{
			for (JsonReader cellReader : p_reader.childArray("cells"))
			{
				grow(detail::parseVector3(cellReader, "at"));
			}
		}
		if (!hasContent)
		{
			throw JsonError(p_reader.file(), p_reader.pathFor("version"), "prefab needs at least one fill or cell");
		}
		const spk::Vector3Int contentOffset{-lowest.x, -lowest.y, -lowest.z};

		VoxelGrid grid(highest - lowest + spk::Vector3Int{1, 1, 1});
		const detail::VoxelPalette palette = detail::parsePalette(p_reader, p_voxels);
		detail::applyVoxelContent(p_reader, grid, palette, contentOffset);
		// The grid constructor lists the whole box, empties included, so stamping the
		// prefab carves interiors and the air above ramps clear even against a cliff.
		PrefabDefinition result{.prefab = spk::Prefab(grid, lowest)};
		if (p_reader.contains("pivot"))
		{
			result.prefab.setPivot(detail::parseVector3(p_reader, "pivot"));
		}

		std::set<std::string> names;
		if (p_reader.contains("anchors"))
		{
			for (JsonReader anchorReader : p_reader.childArray("anchors"))
			{
				anchorReader.forbidUnknown({"name", "at"});
				PrefabAnchor anchor{
					.name = anchorReader.require<std::string>("name"),
					.at = detail::parseVector3(anchorReader, "at")};
				if (!names.insert(anchor.name).second)
				{
					throw JsonError(anchorReader.file(), anchorReader.pathFor("name"), "duplicate anchor name");
				}
				result.anchors.push_back(std::move(anchor));
			}
		}
		return result;
	}
}
