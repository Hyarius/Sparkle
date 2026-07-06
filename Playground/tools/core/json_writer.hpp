#pragma once

#include <filesystem>
#include <string>

#include <nlohmann/json.hpp>

namespace pg::tools
{
	class JsonWriter
	{
	public:
		using OrderedJson = nlohmann::ordered_json;

		[[nodiscard]] OrderedJson ordered(const nlohmann::json &p_value) const;
		[[nodiscard]] std::string serialize(const nlohmann::json &p_value) const;
		void write(const std::filesystem::path &p_file, const nlohmann::json &p_value) const;
	};
}
