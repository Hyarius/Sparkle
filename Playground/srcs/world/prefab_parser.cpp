#include "world/prefab_definition.hpp"

#include "core/json.hpp"
#include "voxel/voxel_registry.hpp"
#include "world/voxel_content_parser.hpp"

#include <algorithm>
#include <set>
#include <string>
#include <vector>

namespace pg
{
	PrefabDefinition parsePrefabDefinition(JsonReader &p_reader, const VoxelRegistry &p_voxels)
	{
		p_reader.forbidUnknown({"version", "pivot", "palette", "fill", "cells", "stairRuns", "anchors", "carve", "clearance", "interior"});
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
		struct StairRun
		{
			spk::Vector3Int from{};
			int steps = 0;
			int width = 0;
			VoxelOrientation direction = VoxelOrientation::PositiveZ;
			std::string voxel;
			std::string voxelPath;
		};
		std::vector<StairRun> stairRuns;
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
		if (p_reader.contains("stairRuns"))
		{
			for (JsonReader runReader : p_reader.childArray("stairRuns"))
			{
				runReader.forbidUnknown({"from", "steps", "width", "direction", "voxel"});
				StairRun run{
					.from = detail::parseVector3(runReader, "from"),
					.steps = runReader.require<int>("steps"),
					.width = runReader.require<int>("width"),
					.direction = detail::parseOrientation(runReader, "direction"),
					.voxel = runReader.require<std::string>("voxel"),
					.voxelPath = runReader.pathFor("voxel")};
				if (run.steps < 1)
				{
					throw JsonError(runReader.file(), runReader.pathFor("steps"), "steps must be at least 1");
				}
				if (run.width < 1)
				{
					throw JsonError(runReader.file(), runReader.pathFor("width"), "width must be at least 1");
				}

				spk::Vector3Int advance{};
				spk::Vector3Int lateral{};
				switch (run.direction)
				{
				case VoxelOrientation::PositiveX:
					advance = {1, 1, 0};
					lateral = {0, 0, 1};
					break;
				case VoxelOrientation::NegativeX:
					advance = {-1, 1, 0};
					lateral = {0, 0, 1};
					break;
				case VoxelOrientation::PositiveZ:
					advance = {0, 1, 1};
					lateral = {1, 0, 0};
					break;
				case VoxelOrientation::NegativeZ:
					advance = {0, 1, -1};
					lateral = {1, 0, 0};
					break;
				}
				for (int step = 0; step < run.steps; ++step)
				{
					for (int width = 0; width < run.width; ++width)
					{
						grow(run.from + advance * step + lateral * width);
					}
				}
				stairRuns.push_back(std::move(run));
			}
		}
		if (!hasContent)
		{
			throw JsonError(
				p_reader.file(),
				p_reader.pathFor("version"),
				"prefab needs at least one fill, cell, or stair run");
		}
		const spk::Vector3Int contentOffset{-lowest.x, -lowest.y, -lowest.z};

		VoxelGrid grid(highest - lowest + spk::Vector3Int{1, 1, 1});
		const detail::VoxelPalette palette = detail::parsePalette(p_reader, p_voxels);
		detail::applyVoxelContent(p_reader, grid, palette, contentOffset);
		for (const StairRun &run : stairRuns)
		{
			spk::Vector3Int advance{};
			spk::Vector3Int lateral{};
			switch (run.direction)
			{
			case VoxelOrientation::PositiveX:
				advance = {1, 1, 0};
				lateral = {0, 0, 1};
				break;
			case VoxelOrientation::NegativeX:
				advance = {-1, 1, 0};
				lateral = {0, 0, 1};
				break;
			case VoxelOrientation::PositiveZ:
				advance = {0, 1, 1};
				lateral = {1, 0, 0};
				break;
			case VoxelOrientation::NegativeZ:
				advance = {0, 1, -1};
				lateral = {1, 0, 0};
				break;
			}
			for (int step = 0; step < run.steps; ++step)
			{
				for (int width = 0; width < run.width; ++width)
				{
					const spk::Vector3Int position = run.from + advance * step + lateral * width + contentOffset;
					VoxelCell cell = detail::pickPaletteToken(palette, run.voxel, p_reader, run.voxelPath, position);
					cell.orientation = run.direction;
					grid.cell(position) = cell;
				}
			}
		}
		// The grid constructor lists the whole box, empties included, so stamping the
		// prefab carves interiors and the air above ramps clear even against a cliff.
		const bool carve = p_reader.optional<bool>("carve", true);
		spk::Prefab prefab;
		if (carve)
		{
			prefab = spk::Prefab(grid, lowest);
		}
		else
		{
			for (int y = 0; y < grid.size().y; ++y)
			{
				for (int z = 0; z < grid.size().z; ++z)
				{
					for (int x = 0; x < grid.size().x; ++x)
					{
						const VoxelCell &cell = grid.cell(x, y, z);
						if (!cell.isEmpty())
						{
							prefab.addVoxel(lowest + spk::Vector3Int{x, y, z}, cell);
						}
					}
				}
			}
			if (prefab.voxels().empty())
			{
				throw JsonError(
					p_reader.file(),
					p_reader.pathFor("carve"),
					"a non-carving prefab needs at least one non-empty voxel");
			}
		}
		PrefabDefinition result{.prefab = std::move(prefab)};
		if (p_reader.contains("pivot"))
		{
			result.prefab.setPivot(detail::parseVector3(p_reader, "pivot"));
		}

		if (p_reader.contains("clearance"))
		{
			JsonReader clearanceReader = p_reader.child("clearance");
			clearanceReader.forbidUnknown({"min", "max"});
			const spk::Vector3Int low = detail::parseVector3(clearanceReader, "min");
			const spk::Vector3Int high = detail::parseVector3(clearanceReader, "max");
			if (low.x > high.x || low.y > high.y || low.z > high.z)
			{
				throw JsonError(clearanceReader.file(), clearanceReader.pathFor("min"), "clearance min exceeds max");
			}
			result.clearance = PrefabClearance{.min = low, .max = high};
		}
		result.interiorId = p_reader.optional<std::string>("interior", "");

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
