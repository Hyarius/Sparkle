#pragma once

#include <filesystem>

namespace pg
{
	// Directory holding the running executable. Resources and generated outputs (world
	// map previews) resolve relative to it, never relative to the working directory or
	// the source tree.
	[[nodiscard]] std::filesystem::path executableDirectory();

	// Root of the resource tree (data/, textures/, shaders/): the --resource-path
	// override when one was set, otherwise a "resources" folder next to the executable.
	[[nodiscard]] std::filesystem::path resourceRoot();

	// Installs the --resource-path override. Pass an absolute path: a relative one would
	// silently depend on the working directory. An empty path restores the default.
	void setResourceRootOverride(std::filesystem::path p_path);
}
