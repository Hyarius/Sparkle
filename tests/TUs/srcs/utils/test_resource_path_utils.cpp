#include "utils/test_resource_path_utils.hpp"

#include <system_error>

namespace spk::test
{
	namespace
	{
		[[nodiscard]] std::filesystem::path canonicalIfPresent(const std::filesystem::path& p_path)
		{
			std::error_code error;
			std::filesystem::path result = std::filesystem::weakly_canonical(p_path, error);
			if (error)
			{
				return p_path.lexically_normal();
			}
			return result;
		}

		[[nodiscard]] bool isSameOrChildPath(const std::filesystem::path& p_path, const std::filesystem::path& p_parent)
		{
			auto pathIterator = p_path.begin();
			auto parentIterator = p_parent.begin();
			for (; parentIterator != p_parent.end(); ++parentIterator, ++pathIterator)
			{
				if (pathIterator == p_path.end() || *pathIterator != *parentIterator)
				{
					return false;
				}
			}
			return true;
		}
	}

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

	std::filesystem::path expectedImagePath(const std::filesystem::path& p_category, const std::string& p_name)
	{
		return expectedDirectory() / p_category / (p_name + ".png");
	}

	std::filesystem::path resultImagePath(const std::filesystem::path& p_category, const std::string& p_name)
	{
		return resultDirectory() / p_category / (p_name + ".png");
	}

	void removeEmptyResultDirectories(const std::filesystem::path& p_startingPath)
	{
		const std::filesystem::path resultRoot = canonicalIfPresent(resultDirectory());
		std::filesystem::path cursor = canonicalIfPresent(p_startingPath);

		if (std::filesystem::is_regular_file(cursor) == true)
		{
			cursor = cursor.parent_path();
		}

		if (isSameOrChildPath(cursor, resultRoot) == false)
		{
			return;
		}

		while (cursor.empty() == false && isSameOrChildPath(cursor, resultRoot) == true)
		{
			std::error_code error;
			if (std::filesystem::is_empty(cursor, error) == false || error)
			{
				break;
			}
			std::filesystem::remove(cursor, error);
			if (error || cursor == resultRoot)
			{
				break;
			}
			cursor = cursor.parent_path();
		}
	}
}
