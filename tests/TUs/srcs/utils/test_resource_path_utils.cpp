#include "test_resource_path_utils.hpp"

namespace spk::test
{
	std::filesystem::path testResourcesRoot()
	{
		auto cursor = std::filesystem::current_path();
		for (int i = 0; i < 8; ++i)
		{
			auto candidate = cursor / "tests" / "TUs" / "resources";
			if (std::filesystem::exists(candidate) == true)
			{
				return candidate;
			}
			if (cursor.has_parent_path() == false)
			{
				break;
			}
			cursor = cursor.parent_path();
		}

		return std::filesystem::current_path() / "tests" / "TUs" / "resources";
	}

	std::filesystem::path expectedDirectory()
	{
		return testResourcesRoot() / "expectedImages";
	}

	std::filesystem::path resultDirectory()
	{
		return testResourcesRoot() / "imageResults";
	}

	std::filesystem::path expectedImagePath(
		const std::filesystem::path& p_category,
		const std::string& p_name)
	{
		std::filesystem::path directory = expectedDirectory() / p_category;
		std::filesystem::create_directories(directory);
		return directory / (p_name + ".png");
	}

	std::filesystem::path resultImagePath(
		const std::filesystem::path& p_category,
		const std::string& p_name)
	{
		std::filesystem::path directory = resultDirectory() / p_category;
		std::filesystem::create_directories(directory);
		return directory / (p_name + ".png");
	}
}
