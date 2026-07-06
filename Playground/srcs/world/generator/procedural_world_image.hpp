#pragma once

#include <filesystem>

namespace pg
{
	class ProceduralWorld;

	class ProceduralWorldImage
	{
	public:
		static void writePng(const ProceduralWorld &p_world, const std::filesystem::path &p_output);
	};
}
