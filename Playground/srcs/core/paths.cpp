#include "core/paths.hpp"

#include <utility>

#include <Windows.h>

namespace
{
	std::filesystem::path resourceRootOverride;
}

namespace pg
{
	std::filesystem::path executableDirectory()
	{
		wchar_t buffer[MAX_PATH];
		const DWORD length = ::GetModuleFileNameW(nullptr, buffer, MAX_PATH);
		// A zero return or a full buffer means the path could not be retrieved reliably;
		// the working directory is the best remaining approximation.
		if (length == 0 || length >= MAX_PATH)
		{
			return std::filesystem::current_path();
		}
		return std::filesystem::path(buffer, buffer + length).parent_path();
	}

	std::filesystem::path resourceRoot()
	{
		if (!resourceRootOverride.empty())
		{
			return resourceRootOverride;
		}
		return executableDirectory() / "resources";
	}

	void setResourceRootOverride(std::filesystem::path p_path)
	{
		resourceRootOverride = std::move(p_path);
	}
}
