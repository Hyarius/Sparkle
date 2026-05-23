#pragma once

#include <filesystem>
#include <string>

namespace spk::test
{
	[[nodiscard]] std::filesystem::path testResourcesRoot();
	[[nodiscard]] std::filesystem::path expectedDirectory();
	[[nodiscard]] std::filesystem::path resultDirectory();
	[[nodiscard]] std::filesystem::path expectedImagePath(
		const std::filesystem::path& p_category,
		const std::string& p_name);
	[[nodiscard]] std::filesystem::path resultImagePath(
		const std::filesystem::path& p_category,
		const std::string& p_name);
}
