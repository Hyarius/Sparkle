#include "world/voxel_content_parser.hpp"

#include "voxel/voxel_registry.hpp"

#include <array>
#include <stdexcept>
#include <vector>

namespace
{
	[[nodiscard]] pg::VoxelCell paletteCell(
		const pg::detail::VoxelPalette &p_palette,
		const pg::JsonReader &p_reader,
		const std::string &p_key)
	{
		const std::string token = p_reader.require<std::string>(p_key);
		const auto iterator = p_palette.find(token);
		if (iterator == p_palette.end())
		{
			throw pg::JsonError(p_reader.file(), p_reader.pathFor(p_key), "unknown palette key '" + token + "'");
		}
		return iterator->second;
	}

	void requireWithin(
		const pg::VoxelGrid &p_grid,
		const spk::Vector3Int &p_position,
		const pg::JsonReader &p_reader,
		const std::string &p_key)
	{
		if (!p_grid.isWithinBounds(p_position))
		{
			throw pg::JsonError(p_reader.file(), p_reader.pathFor(p_key), "position is outside the declared size");
		}
	}
}

namespace pg::detail
{
	spk::Vector3Int parseVector3(const JsonReader &p_reader, const std::string &p_key)
	{
		const std::vector<int> values = p_reader.require<std::vector<int>>(p_key);
		if (values.size() != 3)
		{
			throw JsonError(p_reader.file(), p_reader.pathFor(p_key), "expected exactly three integer coordinates");
		}
		return {values[0], values[1], values[2]};
	}

	VoxelOrientation parseOrientation(
		const JsonReader &p_reader,
		const std::string &p_key,
		VoxelOrientation p_default)
	{
		if (!p_reader.contains(p_key))
		{
			return p_default;
		}
		const std::string value = p_reader.require<std::string>(p_key);
		static const std::map<std::string, VoxelOrientation> values = {
			{"positiveX", VoxelOrientation::PositiveX},
			{"positiveZ", VoxelOrientation::PositiveZ},
			{"negativeX", VoxelOrientation::NegativeX},
			{"negativeZ", VoxelOrientation::NegativeZ}};
		const auto iterator = values.find(value);
		if (iterator == values.end())
		{
			throw JsonError(p_reader.file(), p_reader.pathFor(p_key), "unknown orientation '" + value + "'");
		}
		return iterator->second;
	}

	VoxelFlip parseFlip(const JsonReader &p_reader, const std::string &p_key, VoxelFlip p_default)
	{
		if (!p_reader.contains(p_key))
		{
			return p_default;
		}
		const std::string value = p_reader.require<std::string>(p_key);
		if (value == "positiveY") return VoxelFlip::PositiveY;
		if (value == "negativeY") return VoxelFlip::NegativeY;
		throw JsonError(p_reader.file(), p_reader.pathFor(p_key), "unknown flip '" + value + "'");
	}

	VoxelPalette parsePalette(const JsonReader &p_reader, const VoxelRegistry &p_voxels)
	{
		const JsonReader paletteReader = p_reader.child("palette");
		if (!paletteReader.value().is_object())
		{
			throw JsonError(p_reader.file(), p_reader.pathFor("palette"), "expected an object");
		}

		VoxelPalette palette;
		for (const auto &[token, value] : paletteReader.value().items())
		{
			VoxelCell cell;
			if (!value.is_null())
			{
				if (!value.is_string())
				{
					throw JsonError(p_reader.file(), paletteReader.pathFor(token), "expected voxel id string or null");
				}
				const std::string voxelId = value.get<std::string>();
				const VoxelDefinition *definition = p_voxels.tryGet(voxelId);
				if (definition == nullptr)
				{
					throw JsonError(p_reader.file(), paletteReader.pathFor(token), "unknown voxel id '" + voxelId + "'");
				}
				cell.id = p_voxels.numericId(voxelId);
			}
			palette.emplace(token, cell);
		}
		if (palette.empty())
		{
			throw JsonError(p_reader.file(), p_reader.pathFor("palette"), "palette cannot be empty");
		}
		return palette;
	}

	void applyVoxelContent(
		const JsonReader &p_reader,
		VoxelGrid &p_grid,
		const VoxelPalette &p_palette)
	{
		if (p_reader.contains("fill"))
		{
			for (JsonReader fill : p_reader.childArray("fill"))
			{
				fill.forbidUnknown({"min", "max", "voxel"});
				const spk::Vector3Int minimum = parseVector3(fill, "min");
				const spk::Vector3Int maximum = parseVector3(fill, "max");
				requireWithin(p_grid, minimum, fill, "min");
				requireWithin(p_grid, maximum, fill, "max");
				if (minimum.x > maximum.x || minimum.y > maximum.y || minimum.z > maximum.z)
				{
					throw JsonError(fill.file(), fill.pathFor("max"), "fill max must be greater than or equal to min");
				}
				const VoxelCell value = paletteCell(p_palette, fill, "voxel");
				for (int y = minimum.y; y <= maximum.y; ++y)
				{
					for (int z = minimum.z; z <= maximum.z; ++z)
					{
						for (int x = minimum.x; x <= maximum.x; ++x)
						{
							p_grid.cell(x, y, z) = value;
						}
					}
				}
			}
		}

		if (p_reader.contains("cells"))
		{
			for (JsonReader cellReader : p_reader.childArray("cells"))
			{
				cellReader.forbidUnknown({"at", "voxel", "orientation", "flip"});
				const spk::Vector3Int at = parseVector3(cellReader, "at");
				requireWithin(p_grid, at, cellReader, "at");
				VoxelCell value = paletteCell(p_palette, cellReader, "voxel");
				value.orientation = parseOrientation(cellReader, "orientation");
				value.flip = parseFlip(cellReader, "flip");
				p_grid.cell(at) = value;
			}
		}
	}
}
