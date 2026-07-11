#include "world/voxel_content_parser.hpp"

#include "core/deterministic_random.hpp"
#include "voxel/voxel_registry.hpp"
#include "world/weighted_id_parser.hpp"

#include <array>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace
{
	[[nodiscard]] std::uint64_t selectionSeed(
		const pg::JsonReader &p_reader,
		const std::string &p_path,
		const std::string &p_token,
		const spk::Vector3Int &p_position)
	{
		std::uint64_t hash = pg::deterministic::FnvOffset;
		pg::deterministic::mix(hash, p_reader.file().generic_string());
		pg::deterministic::mix(hash, p_path);
		pg::deterministic::mix(hash, p_token);
		pg::deterministic::mix(hash, p_position.x);
		pg::deterministic::mix(hash, p_position.y);
		pg::deterministic::mix(hash, p_position.z);
		return pg::deterministic::avalanche(hash);
	}

	[[nodiscard]] spk::VoxelCell cellFromId(
		const std::string &p_voxelId,
		const pg::JsonReader &p_reader,
		const std::string &p_path,
		const pg::VoxelRegistry &p_voxels)
	{
		const pg::VoxelDefinition *definition = p_voxels.tryGet(p_voxelId);
		if (definition == nullptr)
		{
			throw pg::JsonError(p_reader.file(), p_path, "unknown voxel id '" + p_voxelId + "'");
		}
		spk::VoxelCell cell;
		cell.id = p_voxels.numericId(p_voxelId);
		return cell;
	}

	[[nodiscard]] spk::VoxelCell pickCellFromPool(
		const pg::detail::VoxelCellPool &p_pool,
		const pg::JsonReader &p_reader,
		const std::string &p_path,
		std::uint64_t p_seed)
	{
		if (p_pool.empty())
		{
			throw pg::JsonError(p_reader.file(), p_path, "palette pool cannot be empty");
		}
		return p_pool.pick(pg::deterministic::unitInterval(p_seed));
	}

	[[nodiscard]] pg::detail::VoxelCellPool parseCellPool(
		const spk::JSON::Value &p_value,
		const pg::JsonReader &p_reader,
		const std::string &p_path,
		const pg::VoxelRegistry &p_voxels)
	{
		pg::detail::VoxelCellPool pool;
		for (pg::WeightedId &entry : pg::parseWeightedIds(p_value, p_reader, p_path))
		{
			spk::VoxelCell cell;
			if (entry.id.has_value())
			{
				cell = cellFromId(*entry.id, p_reader, entry.path, p_voxels);
			}
			pool.add(cell, entry.weight);
		}
		if (pool.empty())
		{
			throw pg::JsonError(p_reader.file(), p_path, "palette pool cannot be empty");
		}
		return pool;
	}

	void requireWithin(
		const spk::VoxelGrid &p_grid,
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
		if (value == "positiveY")
		{
			return VoxelFlip::PositiveY;
		}
		if (value == "negativeY")
		{
			return VoxelFlip::NegativeY;
		}
		throw JsonError(p_reader.file(), p_reader.pathFor(p_key), "unknown flip '" + value + "'");
	}

	VoxelPalette parsePalette(const JsonReader &p_reader, const VoxelRegistry &p_voxels)
	{
		const JsonReader paletteReader = p_reader.child("palette");
		if (!paletteReader.value().isObject())
		{
			throw JsonError(p_reader.file(), p_reader.pathFor("palette"), "expected an object");
		}

		VoxelPalette palette;
		for (const auto &[token, value] : paletteReader.value().asObject())
		{
			palette.emplace(token, parseCellPool(value, paletteReader, paletteReader.pathFor(token), p_voxels));
		}
		if (palette.empty())
		{
			throw JsonError(p_reader.file(), p_reader.pathFor("palette"), "palette cannot be empty");
		}
		return palette;
	}

	spk::VoxelCell pickPaletteToken(
		const VoxelPalette &p_palette,
		const std::string &p_token,
		const JsonReader &p_reader,
		const std::string &p_path,
		const spk::Vector3Int &p_position)
	{
		const auto iterator = p_palette.find(p_token);
		if (iterator == p_palette.end())
		{
			throw JsonError(p_reader.file(), p_path, "unknown palette key '" + p_token + "'");
		}
		return pickCellFromPool(
			iterator->second,
			p_reader,
			p_path,
			selectionSeed(p_reader, p_path, p_token, p_position));
	}

	spk::VoxelCell pickPaletteCell(
		const VoxelPalette &p_palette,
		const JsonReader &p_reader,
		const std::string &p_key,
		const spk::Vector3Int &p_position)
	{
		return pickPaletteToken(
			p_palette,
			p_reader.require<std::string>(p_key),
			p_reader,
			p_reader.pathFor(p_key),
			p_position);
	}

	void applyVoxelContent(
		const JsonReader &p_reader,
		spk::VoxelGrid &p_grid,
		const VoxelPalette &p_palette,
		const spk::Vector3Int &p_offset)
	{
		if (p_reader.contains("fill"))
		{
			for (JsonReader fill : p_reader.childArray("fill"))
			{
				fill.forbidUnknown({"min", "max", "voxel"});
				const spk::Vector3Int minimum = parseVector3(fill, "min") + p_offset;
				const spk::Vector3Int maximum = parseVector3(fill, "max") + p_offset;
				requireWithin(p_grid, minimum, fill, "min");
				requireWithin(p_grid, maximum, fill, "max");
				if (minimum.x > maximum.x || minimum.y > maximum.y || minimum.z > maximum.z)
				{
					throw JsonError(fill.file(), fill.pathFor("max"), "fill max must be greater than or equal to min");
				}
				for (int y = minimum.y; y <= maximum.y; ++y)
				{
					for (int z = minimum.z; z <= maximum.z; ++z)
					{
						for (int x = minimum.x; x <= maximum.x; ++x)
						{
							const spk::VoxelCell value = pickPaletteCell(p_palette, fill, "voxel", {x, y, z});
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
				const spk::Vector3Int at = parseVector3(cellReader, "at") + p_offset;
				requireWithin(p_grid, at, cellReader, "at");
				spk::VoxelCell value = pickPaletteCell(p_palette, cellReader, "voxel", at);
				value.orientation = parseOrientation(cellReader, "orientation");
				value.flip = parseFlip(cellReader, "flip");
				p_grid.cell(at) = value;
			}
		}
	}
}
