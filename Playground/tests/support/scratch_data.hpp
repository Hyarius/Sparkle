#pragma once

#include "core/paths.hpp"

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

namespace pgtest
{
	// A scratch copy of the shipped resources/data tree that a case may edit freely. It exists so a
	// registry-level case can author exactly the one broken file it is about while keeping the real
	// content around it to reference - and so a failed load can be proved not to have published
	// anything, against a Registries value that already holds the real content.
	class ScratchData
	{
	private:
		std::filesystem::path _path;

	public:
		explicit ScratchData(std::string_view p_name) :
			_path(std::filesystem::temp_directory_path() / "sparkle-playground-scratch" / p_name)
		{
			std::filesystem::remove_all(_path);
			std::filesystem::create_directories(_path);
			std::filesystem::copy(
				pg::resourceRoot() / "data",
				_path,
				std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
		}

		~ScratchData()
		{
			std::error_code error;
			std::filesystem::remove_all(_path, error);
		}

		ScratchData(const ScratchData &) = delete;
		ScratchData &operator=(const ScratchData &) = delete;

		[[nodiscard]] const std::filesystem::path &path() const noexcept { return _path; }

		// Writes <directory>/<id>.json, replacing any shipped file of that name.
		void write(std::string_view p_directory, std::string_view p_id, std::string_view p_content) const
		{
			const std::filesystem::path target =
				_path / std::filesystem::path(p_directory) / (std::string(p_id) + ".json");
			std::filesystem::create_directories(target.parent_path());
			std::ofstream stream(target, std::ios::binary | std::ios::trunc);
			stream << p_content;
		}

		// There is deliberately no "empty this registry directory" helper. Since step 04 the graph
		// is closed - the shipped species needs the shipped board and ability, the encounters need
		// the species, the biomes need the encounters, and the new game needs all of it - so a tree
		// with an emptied registry is not a tree the loader accepts. A case writes over the file it
		// is about instead.
		void writeFile(std::string_view p_relativePath, std::string_view p_content) const
		{
			const std::filesystem::path target = _path / std::filesystem::path(p_relativePath);
			std::filesystem::create_directories(target.parent_path());
			std::ofstream stream(target, std::ios::binary | std::ios::trunc);
			stream << p_content;
		}
	};
}
