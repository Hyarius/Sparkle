#include "world/weighted_id_parser.hpp"

#include <cmath>
#include <cstddef>

namespace
{
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

	[[nodiscard]] std::optional<std::string> parseId(
		const spk::JSON::Value &p_value,
		const pg::JsonReader &p_reader,
		const std::string &p_path)
	{
		if (p_value.isNull())
		{
			return std::nullopt;
		}
		if (!p_value.isString())
		{
			throw pg::JsonError(p_reader.file(), p_path, "expected an id string or null");
		}
		return p_value.as<std::string>();
	}

	void appendOne(
		std::vector<pg::WeightedId> &p_result,
		const spk::JSON::Value &p_entry,
		const pg::JsonReader &p_reader,
		const std::string &p_path)
	{
		if (!p_entry.isObject())
		{
			p_result.push_back({.id = parseId(p_entry, p_reader, p_path), .weight = 1.0, .path = p_path});
			return;
		}

		const spk::JSON::Value *voxelValue = p_entry.find("voxel");
		const spk::JSON::Value *idValue = p_entry.find("id");
		if (voxelValue == nullptr && idValue == nullptr)
		{
			for (const auto &[id, weightValue] : p_entry.asObject())
			{
				const std::string path = p_path + "." + id;
				p_result.push_back({.id = id, .weight = parseWeight(weightValue, p_reader, path), .path = path});
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

		const spk::JSON::Value *weightValue = p_entry.find("weight");
		const spk::JSON::Value *chanceValue = p_entry.find("chance");
		if (weightValue != nullptr && chanceValue != nullptr)
		{
			throw pg::JsonError(p_reader.file(), p_path, "use either 'weight' or 'chance', not both");
		}
		const double weight = weightValue != nullptr   ? parseWeight(*weightValue, p_reader, p_path + ".weight")
							  : chanceValue != nullptr ? parseWeight(*chanceValue, p_reader, p_path + ".chance")
													   : 1.0;
		const spk::JSON::Value &selectedId = voxelValue != nullptr ? *voxelValue : *idValue;
		p_result.push_back({.id = parseId(selectedId, p_reader, p_path), .weight = weight, .path = p_path});
	}
}

namespace pg
{
	std::vector<WeightedId> parseWeightedIds(
		const spk::JSON::Value &p_value,
		const JsonReader &p_reader,
		const std::string &p_path)
	{
		std::vector<WeightedId> result;
		if (p_value.isArray())
		{
			const spk::JSON::Value::Array &entries = p_value.asArray();
			result.reserve(entries.size());
			for (std::size_t index = 0; index < entries.size(); ++index)
			{
				appendOne(result, entries[index], p_reader, p_path + "[" + std::to_string(index) + "]");
			}
		}
		else
		{
			appendOne(result, p_value, p_reader, p_path);
		}
		return result;
	}
}
