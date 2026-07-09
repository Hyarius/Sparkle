#pragma once

#include "core/json.hpp"

#include <algorithm>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace pg
{
	class Registries;

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

	template <typename TBase>
	class PolymorphicFactory
	{
	private:
		using StoredParseFunction = std::function<std::unique_ptr<TBase>(JsonReader &, const Registries *)>;
		std::map<std::string, StoredParseFunction> _parsers;

		[[nodiscard]] std::unique_ptr<TBase> _parse(JsonReader &p_reader, const Registries *p_registries) const
		{
			const std::string type = p_reader.require<std::string>("type");
			const auto iterator = _parsers.find(type);
			if (iterator == _parsers.end())
			{
				std::string knownTypes;
				for (const auto &[name, unused] : _parsers)
				{
					(void)unused;
					if (!knownTypes.empty())
					{
						knownTypes += ", ";
					}
					knownTypes += name;
				}
				throw JsonError(
					p_reader.file(),
					p_reader.pathFor("type"),
					"unknown polymorphic type '" + type + "' (known types: " + knownTypes + ")");
			}
			return iterator->second(p_reader, p_registries);
		}

	public:
		template <typename TParseFunction>
		void registerType(std::string p_name, TParseFunction p_parse)
		{
			StoredParseFunction wrapper;
			if constexpr (std::is_invocable_r_v<std::unique_ptr<TBase>, TParseFunction &, JsonReader &, const Registries &>)
			{
				wrapper = [parse = std::move(p_parse)](JsonReader &p_reader, const Registries *p_registries) mutable {
					if (p_registries == nullptr)
					{
						throw std::logic_error("this polymorphic parser requires registries");
					}
					return std::invoke(parse, p_reader, *p_registries);
				};
			}
			else
			{
				static_assert(
					std::is_invocable_r_v<std::unique_ptr<TBase>, TParseFunction &, JsonReader &>,
					"A polymorphic parser must accept JsonReader&, optionally followed by const Registries&");
				wrapper = [parse = std::move(p_parse)](JsonReader &p_reader, const Registries *) mutable {
					return std::invoke(parse, p_reader);
				};
			}

			const auto [unused, inserted] = _parsers.emplace(std::move(p_name), std::move(wrapper));
			(void)unused;
			if (!inserted)
			{
				throw std::logic_error("polymorphic type is already registered");
			}
		}

		[[nodiscard]] std::unique_ptr<TBase> parse(JsonReader &p_reader) const
		{
			return _parse(p_reader, nullptr);
		}

		[[nodiscard]] std::unique_ptr<TBase> parse(JsonReader &p_reader, const Registries &p_registries) const
		{
			return _parse(p_reader, &p_registries);
		}
	};
}
