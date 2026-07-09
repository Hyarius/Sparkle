#include "world/voxel_content_parser.hpp"

#include "voxel/voxel_registry.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace
{
	[[nodiscard]] std::uint64_t avalanche(std::uint64_t p_value) noexcept
	{
		p_value ^= p_value >> 30U;
		p_value *= 0xbf58476d1ce4e5b9ULL;
		p_value ^= p_value >> 27U;
		p_value *= 0x94d049bb133111ebULL;
		p_value ^= p_value >> 31U;
		return p_value;
	}

	void mix(std::uint64_t &p_hash, std::string_view p_value)
	{
		for (const char character : p_value)
		{
			p_hash ^= static_cast<std::uint8_t>(character);
			p_hash *= 1099511628211ULL;
		}
	}

	void mix(std::uint64_t &p_hash, int p_value)
	{
		p_hash ^= static_cast<std::uint32_t>(p_value);
		p_hash *= 1099511628211ULL;
	}

	[[nodiscard]] std::uint64_t selectionSeed(
		const pg::JsonReader &p_reader,
		const std::string &p_path,
		const std::string &p_token,
		const spk::Vector3Int &p_position)
	{
		std::uint64_t hash = 1469598103934665603ULL;
		mix(hash, p_reader.file().generic_string());
		mix(hash, p_path);
		mix(hash, p_token);
		mix(hash, p_position.x);
		mix(hash, p_position.y);
		mix(hash, p_position.z);
		return avalanche(hash);
	}

	[[nodiscard]] double unitInterval(std::uint64_t p_hash) noexcept
	{
		return static_cast<double>(p_hash >> 11U) * (1.0 / 9007199254740992.0);
	}

	[[nodiscard]] double parseWeight(
		const spk::JSON::Value &p_value,
		const pg::JsonReader &p_reader,
		const std::string &p_path)
	{
		if (!p_value.isNumber())
		{
			throw pg::JsonError(p_reader.file(), p_path, "expected a numeric weight");
		}
		const double weight = p_value.as<double>();
		if (!std::isfinite(weight) || weight <= 0.0)
		{
			throw pg::JsonError(p_reader.file(), p_path, "weight must be greater than 0");
		}
		return weight;
	}

	[[nodiscard]] pg::VoxelCell cellFromValue(
		const spk::JSON::Value &p_value,
		const pg::JsonReader &p_reader,
		const std::string &p_path,
		const pg::VoxelRegistry &p_voxels)
	{
		pg::VoxelCell cell;
		if (p_value.isNull())
		{
			return cell;
		}
		if (!p_value.isString())
		{
			throw pg::JsonError(p_reader.file(), p_path, "expected voxel id string or null");
		}

		const std::string voxelId = p_value.as<std::string>();
		const pg::VoxelDefinition *definition = p_voxels.tryGet(voxelId);
		if (definition == nullptr)
		{
			throw pg::JsonError(p_reader.file(), p_path, "unknown voxel id '" + voxelId + "'");
		}
		cell.id = p_voxels.numericId(voxelId);
		return cell;
	}

	[[nodiscard]] pg::VoxelCell cellFromId(
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
		pg::VoxelCell cell;
		cell.id = p_voxels.numericId(p_voxelId);
		return cell;
	}

	[[nodiscard]] pg::VoxelCell pickCellFromPool(
		const pg::detail::VoxelCellPool &p_pool,
		const pg::JsonReader &p_reader,
		const std::string &p_path,
		std::uint64_t p_seed)
	{
		if (p_pool.empty())
		{
			throw pg::JsonError(p_reader.file(), p_path, "palette pool cannot be empty");
		}
		if (p_pool.size() == 1)
		{
			return p_pool.front().cell;
		}

		double totalWeight = 0.0;
		for (const pg::detail::WeightedVoxelCell &entry : p_pool)
		{
			totalWeight += entry.weight;
		}

		double target = unitInterval(p_seed) * totalWeight;
		for (const pg::detail::WeightedVoxelCell &entry : p_pool)
		{
			if (target < entry.weight)
			{
				return entry.cell;
			}
			target -= entry.weight;
		}
		return p_pool.back().cell;
	}

	void appendPaletteEntry(
		pg::detail::VoxelCellPool &p_pool,
		const spk::JSON::Value &p_entry,
		const pg::JsonReader &p_reader,
		const std::string &p_path,
		const pg::VoxelRegistry &p_voxels)
	{
		if (!p_entry.isObject())
		{
			p_pool.push_back(
				{.cell = cellFromValue(p_entry, p_reader, p_path, p_voxels), .weight = 1.0});
			return;
		}

		const spk::JSON::Value *voxelValue = p_entry.find("voxel");
		const spk::JSON::Value *idValue = p_entry.find("id");
		if (voxelValue == nullptr && idValue == nullptr)
		{
			for (const auto &[voxelId, weightValue] : p_entry.asObject())
			{
				p_pool.push_back(
					{.cell = cellFromId(voxelId, p_reader, p_path + "." + voxelId, p_voxels),
					 .weight = parseWeight(weightValue, p_reader, p_path + "." + voxelId)});
			}
			return;
		}
		if (voxelValue != nullptr && idValue != nullptr)
		{
			throw pg::JsonError(p_reader.file(), p_path, "use either 'voxel' or 'id', not both");
		}
		for (const auto &[key, unused] : p_entry.asObject())
		{
			(void)unused;
			if (key != "voxel" && key != "id" && key != "weight" && key != "chance")
			{
				throw pg::JsonError(p_reader.file(), p_path + "." + key, "unknown field");
			}
		}

		const spk::JSON::Value &selectedVoxel = voxelValue != nullptr ? *voxelValue : *idValue;
		const spk::JSON::Value *weightValue = p_entry.find("weight");
		const spk::JSON::Value *chanceValue = p_entry.find("chance");
		if (weightValue != nullptr && chanceValue != nullptr)
		{
			throw pg::JsonError(p_reader.file(), p_path, "use either 'weight' or 'chance', not both");
		}
		const double weight = weightValue != nullptr   ? parseWeight(*weightValue, p_reader, p_path + ".weight")
							  : chanceValue != nullptr ? parseWeight(*chanceValue, p_reader, p_path + ".chance")
													   : 1.0;
		p_pool.push_back(
			{.cell = cellFromValue(selectedVoxel, p_reader, p_path, p_voxels), .weight = weight});
	}

	[[nodiscard]] pg::detail::VoxelCellPool parseCellPool(
		const spk::JSON::Value &p_value,
		const pg::JsonReader &p_reader,
		const std::string &p_path,
		const pg::VoxelRegistry &p_voxels)
	{
		pg::detail::VoxelCellPool pool;
		if (p_value.isArray())
		{
			const spk::JSON::Value::Array &entries = p_value.asArray();
			for (std::size_t index = 0; index < entries.size(); ++index)
			{
				appendPaletteEntry(pool, entries[index], p_reader, p_path + "[" + std::to_string(index) + "]", p_voxels);
			}
		}
		else if (p_value.isObject())
		{
			appendPaletteEntry(pool, p_value, p_reader, p_path, p_voxels);
		}
		else
		{
			appendPaletteEntry(pool, p_value, p_reader, p_path, p_voxels);
		}
		if (pool.empty())
		{
			throw pg::JsonError(p_reader.file(), p_path, "palette pool cannot be empty");
		}
		return pool;
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

	VoxelCell pickPaletteToken(
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

	VoxelCell pickPaletteCell(
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
		VoxelGrid &p_grid,
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
							const VoxelCell value = pickPaletteCell(p_palette, fill, "voxel", {x, y, z});
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
				VoxelCell value = pickPaletteCell(p_palette, cellReader, "voxel", at);
				value.orientation = parseOrientation(cellReader, "orientation");
				value.flip = parseFlip(cellReader, "flip");
				p_grid.cell(at) = value;
			}
		}
	}
}
