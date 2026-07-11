#pragma once

#include "core/json.hpp"

#include <algorithm>
#include <filesystem>
#include <functional>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace pg
{
	template <typename TDefinition>
	class Registry
	{
	private:
		std::map<std::string, TDefinition> _definitions;

	public:
		template <typename TParseFunction>
		void load(const std::filesystem::path &p_directory, TParseFunction p_parse)
		{
			std::error_code error;
			if (!std::filesystem::exists(p_directory, error) || error)
			{
				throw JsonError(p_directory, "$", "registry directory does not exist");
			}
			if (!std::filesystem::is_directory(p_directory, error) || error)
			{
				throw JsonError(p_directory, "$", "registry path is not a directory");
			}

			std::vector<std::filesystem::path> files;
			std::filesystem::directory_iterator iterator(p_directory, error);
			const std::filesystem::directory_iterator end;
			if (error)
			{
				throw JsonError(p_directory, "$", "unable to iterate registry directory: " + error.message());
			}
			while (iterator != end)
			{
				const bool isRegularFile = iterator->is_regular_file(error);
				if (error)
				{
					throw JsonError(iterator->path(), "$", "unable to inspect registry entry: " + error.message());
				}
				if (isRegularFile && iterator->path().extension() == ".json")
				{
					files.push_back(iterator->path());
				}

				iterator.increment(error);
				if (error)
				{
					throw JsonError(p_directory, "$", "unable to iterate registry directory: " + error.message());
				}
			}

			std::sort(files.begin(), files.end(), [](const auto &p_left, const auto &p_right) {
				return p_left.filename().generic_string() < p_right.filename().generic_string();
			});

			std::map<std::string, TDefinition> loadedDefinitions;
			for (const std::filesystem::path &file : files)
			{
				const std::string id = file.stem().string();
				if (_definitions.contains(id) || loadedDefinitions.contains(id))
				{
					throw JsonError(file, "$", "duplicate registry id '" + id + "'");
				}

				const spk::JSON::Value json = JsonLoader::parseFile(file);
				JsonReader reader(json, file);
				TDefinition definition = std::invoke(p_parse, reader);
				if constexpr (requires { definition.id = id; })
				{
					definition.id = id;
				}
				loadedDefinitions.emplace(id, std::move(definition));
			}

			_definitions.merge(loadedDefinitions);
		}

		// Inserts a definition built outside the JSON directory sweep (synthesized at load
		// time, e.g. climb prefabs generated from a biome's palette voxels). Throws on a
		// duplicate id and stamps the definition's own id field when it has one.
		void add(const std::string &p_id, TDefinition p_definition)
		{
			if (_definitions.contains(p_id))
			{
				throw std::runtime_error("duplicate registry id '" + p_id + "'");
			}
			if constexpr (requires { p_definition.id = p_id; })
			{
				p_definition.id = p_id;
			}
			_definitions.emplace(p_id, std::move(p_definition));
		}

		[[nodiscard]] const TDefinition &get(const std::string &p_id) const
		{
			const auto iterator = _definitions.find(p_id);
			if (iterator == _definitions.end())
			{
				throw std::out_of_range("unknown registry id '" + p_id + "'");
			}
			return iterator->second;
		}

		[[nodiscard]] TDefinition &getMutable(const std::string &p_id)
		{
			const auto iterator = _definitions.find(p_id);
			if (iterator == _definitions.end())
			{
				throw std::out_of_range("unknown registry id '" + p_id + "'");
			}
			return iterator->second;
		}

		[[nodiscard]] const TDefinition *tryGet(const std::string &p_id) const noexcept
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
	};

}
