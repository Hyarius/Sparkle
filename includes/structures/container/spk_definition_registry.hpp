#pragma once

#include "structures/container/spk_json_reader.hpp"

#include <algorithm>
#include <filesystem>
#include <functional>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace spk
{
	template <typename TDefinition>
	class DefinitionRegistry
	{
	private:
		std::map<std::string, TDefinition, std::less<>> _definitions;

	public:
		void add(std::string p_id, TDefinition p_definition)
		{
			if (_definitions.contains(p_id))
			{
				throw std::runtime_error("duplicate registry id '" + p_id + "'");
			}
			_definitions.emplace(std::move(p_id), std::move(p_definition));
		}

		[[nodiscard]] bool contains(std::string_view p_id) const noexcept
		{
			return _definitions.contains(p_id);
		}

		[[nodiscard]] const TDefinition &get(std::string_view p_id) const
		{
			const auto iterator = _definitions.find(p_id);
			if (iterator == _definitions.end())
			{
				throw std::out_of_range("unknown registry id '" + std::string(p_id) + "'");
			}
			return iterator->second;
		}

		[[nodiscard]] TDefinition &getMutable(std::string_view p_id)
		{
			const auto iterator = _definitions.find(p_id);
			if (iterator == _definitions.end())
			{
				throw std::out_of_range("unknown registry id '" + std::string(p_id) + "'");
			}
			return iterator->second;
		}

		[[nodiscard]] const TDefinition *tryGet(std::string_view p_id) const noexcept
		{
			const auto iterator = _definitions.find(p_id);
			return iterator == _definitions.end() ? nullptr : &iterator->second;
		}

		[[nodiscard]] std::vector<std::string> ids() const
		{
			std::vector<std::string> result;
			result.reserve(_definitions.size());
			for (const auto &[id, unused] : _definitions)
			{
				(void)unused;
				result.push_back(id);
			}
			return result;
		}

		[[nodiscard]] std::size_t size() const noexcept
		{
			return _definitions.size();
		}
		[[nodiscard]] auto begin() const noexcept
		{
			return _definitions.begin();
		}
		[[nodiscard]] auto end() const noexcept
		{
			return _definitions.end();
		}

		void merge(DefinitionRegistry &&p_other)
		{
			for (const auto &[id, unused] : p_other._definitions)
			{
				(void)unused;
				if (_definitions.contains(id))
				{
					throw std::runtime_error("duplicate registry id '" + id + "'");
				}
			}
			_definitions.merge(p_other._definitions);
		}
	};

	// Loads only direct .json children, in filename order. The operation is
	// transactional: p_registry is not changed when discovery or parsing fails.
	template <typename TDefinition, typename TParser>
	void loadJsonDirectory(
		spk::DefinitionRegistry<TDefinition> &p_registry,
		const std::filesystem::path &p_directory,
		TParser p_parser)
	{
		std::error_code error;
		if (!std::filesystem::exists(p_directory, error) || error)
		{
			throw spk::JSON::Error(p_directory, "$", "registry directory does not exist");
		}
		if (!std::filesystem::is_directory(p_directory, error) || error)
		{
			throw spk::JSON::Error(p_directory, "$", "registry path is not a directory");
		}

		std::vector<std::filesystem::path> files;
		std::filesystem::directory_iterator iterator(p_directory, error);
		const std::filesystem::directory_iterator end;
		if (error)
		{
			throw spk::JSON::Error(p_directory, "$", "unable to iterate registry directory: " + error.message());
		}
		while (iterator != end)
		{
			const bool regular = iterator->is_regular_file(error);
			if (error)
			{
				throw spk::JSON::Error(iterator->path(), "$", "unable to inspect registry entry: " + error.message());
			}
			if (regular && iterator->path().extension() == ".json")
			{
				files.push_back(iterator->path());
			}
			iterator.increment(error);
			if (error)
			{
				throw spk::JSON::Error(p_directory, "$", "unable to iterate registry directory: " + error.message());
			}
		}

		std::ranges::sort(files, {}, [](const std::filesystem::path &p_path) {
			return p_path.filename().generic_string();
		});

		spk::DefinitionRegistry<TDefinition> loaded;
		for (const std::filesystem::path &file : files)
		{
			const std::string id = file.stem().string();
			if (p_registry.contains(id) || loaded.contains(id))
			{
				throw spk::JSON::Error(file, "$", "duplicate registry id '" + id + "'");
			}

			const spk::JSON::Value json = spk::JSON::Loader::parseFile(file);
			spk::JSON::Reader reader(json, file);
			loaded.add(id, std::invoke(p_parser, std::string_view(id), reader));
		}

		p_registry.merge(std::move(loaded));
	}
}
