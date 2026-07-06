#include "tools/core/json_writer.hpp"

#include <algorithm>
#include <array>
#include <fstream>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace
{
	constexpr auto PreferredKeys = std::to_array<std::string_view>({
		"version", "id", "name", "description", "type", "kind", "enabled",
		"traversal", "tags", "shape", "textures", "top", "bottom", "side",
		"posX", "negX", "posZ", "negZ", "plane", "slope", "back", "riser",
		"sideLeft", "sideRight", "height", "stepCount", "orientation", "flip"});

	[[nodiscard]] std::size_t keyRank(std::string_view p_key)
	{
		const auto iterator = std::ranges::find(PreferredKeys, p_key);
		return iterator == PreferredKeys.end()
			? PreferredKeys.size()
			: static_cast<std::size_t>(std::distance(PreferredKeys.begin(), iterator));
	}
}

namespace pg::tools
{
	JsonWriter::OrderedJson JsonWriter::ordered(const nlohmann::json &p_value) const
	{
		if (p_value.is_object())
		{
			std::vector<std::string> keys;
			keys.reserve(p_value.size());
			for (const auto &[key, unused] : p_value.items())
			{
				(void)unused;
				keys.push_back(key);
			}
			std::ranges::sort(keys, [](const std::string &p_left, const std::string &p_right) {
				const std::size_t leftRank = keyRank(p_left);
				const std::size_t rightRank = keyRank(p_right);
				return leftRank == rightRank ? p_left < p_right : leftRank < rightRank;
			});

			OrderedJson result = OrderedJson::object();
			for (const std::string &key : keys)
			{
				result[key] = ordered(p_value.at(key));
			}
			return result;
		}
		if (p_value.is_array())
		{
			OrderedJson result = OrderedJson::array();
			for (const nlohmann::json &element : p_value)
			{
				result.push_back(ordered(element));
			}
			return result;
		}
		return OrderedJson(p_value);
	}

	std::string JsonWriter::serialize(const nlohmann::json &p_value) const
	{
		return ordered(p_value).dump(2) + '\n';
	}

	void JsonWriter::write(const std::filesystem::path &p_file, const nlohmann::json &p_value) const
	{
		std::error_code error;
		std::filesystem::create_directories(p_file.parent_path(), error);
		if (error)
		{
			throw std::runtime_error("Unable to create JSON directory '" +
				p_file.parent_path().generic_string() + "': " + error.message());
		}

		std::ofstream stream(p_file, std::ios::binary | std::ios::trunc);
		if (!stream.is_open())
		{
			throw std::runtime_error("Unable to open JSON file '" + p_file.generic_string() + "' for writing");
		}
		stream << serialize(p_value);
		if (!stream.good())
		{
			throw std::runtime_error("Unable to write JSON file '" + p_file.generic_string() + "'");
		}
	}
}
