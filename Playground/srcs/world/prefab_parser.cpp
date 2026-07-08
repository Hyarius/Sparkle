#include "world/prefab_definition.hpp"

#include "core/json.hpp"
#include "voxel/voxel_registry.hpp"
#include "world/voxel_content_parser.hpp"

#include <set>

namespace pg
{
	PrefabDefinition parsePrefabDefinition(JsonReader &p_reader, const VoxelRegistry &p_voxels)
	{
		p_reader.forbidUnknown({"version", "size", "palette", "fill", "cells", "anchors"});
		if (p_reader.require<int>("version") != 1)
		{
			throw JsonError(p_reader.file(), p_reader.pathFor("version"), "unsupported prefab version");
		}

		const spk::Vector3Int size = detail::parseVector3(p_reader, "size");
		if (size.x <= 0 || size.y <= 0 || size.z <= 0)
		{
			throw JsonError(p_reader.file(), p_reader.pathFor("size"), "prefab dimensions must be positive");
		}
		VoxelGrid grid(size);
		const detail::VoxelPalette palette = detail::parsePalette(p_reader, p_voxels);
		detail::applyVoxelContent(p_reader, grid, palette);
		// The grid constructor lists the whole box, empties included, so stamping the
		// prefab carves interiors and the air above ramps clear even against a cliff.
		PrefabDefinition result{.prefab = spk::Prefab(grid)};

		std::set<std::string> names;
		if (p_reader.contains("anchors"))
		{
			for (JsonReader anchorReader : p_reader.childArray("anchors"))
			{
				anchorReader.forbidUnknown({"name", "at"});
				PrefabAnchor anchor{
					.name = anchorReader.require<std::string>("name"),
					.at = detail::parseVector3(anchorReader, "at")};
				if (!grid.isWithinBounds(anchor.at))
				{
					throw JsonError(anchorReader.file(), anchorReader.pathFor("at"), "anchor is outside the declared size");
				}
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
