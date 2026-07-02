#pragma once

#include <filesystem>
#include <initializer_list>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace pg
{
	class JsonError : public std::runtime_error
	{
	private:
		std::filesystem::path _file;
		std::string _path;
		std::string _message;

	public:
		JsonError(std::filesystem::path p_file, std::string p_path, std::string p_message);

		[[nodiscard]] const std::filesystem::path &file() const noexcept;
		[[nodiscard]] const std::string &path() const noexcept;
		[[nodiscard]] const std::string &message() const noexcept;
	};

	class JsonLoader
	{
	public:
		[[nodiscard]] static nlohmann::json parseFile(const std::filesystem::path &p_file);
	};

	class JsonReader
	{
	private:
		const nlohmann::json &_value;
		std::filesystem::path _file;
		std::string _path;

		void _requireObject() const;
		[[nodiscard]] const nlohmann::json &_requireMember(const std::string &p_key) const;

	public:
		JsonReader(const nlohmann::json &p_value, std::filesystem::path p_file, std::string p_path = "$");

		[[nodiscard]] const std::filesystem::path &file() const noexcept;
		[[nodiscard]] const std::string &path() const noexcept;
		[[nodiscard]] std::string pathFor(const std::string &p_key) const;
		[[nodiscard]] bool contains(const std::string &p_key) const;

		template <typename TType>
		[[nodiscard]] TType require(const std::string &p_key) const
		{
			const nlohmann::json &member = _requireMember(p_key);

			try
			{
				return member.template get<TType>();
			} catch (const nlohmann::json::exception &exception)
			{
				throw JsonError(_file, pathFor(p_key), std::string("invalid value: ") + exception.what());
			}
		}

		template <typename TType>
		[[nodiscard]] TType optional(const std::string &p_key, TType p_default) const
		{
			_requireObject();
			const auto iterator = _value.find(p_key);
			if (iterator == _value.end())
			{
				return p_default;
			}

			try
			{
				return iterator->template get<TType>();
			} catch (const nlohmann::json::exception &exception)
			{
				throw JsonError(_file, pathFor(p_key), std::string("invalid value: ") + exception.what());
			}
		}

		template <typename TType, typename TMap>
		[[nodiscard]] TType requireEnum(const std::string &p_key, const TMap &p_values) const
		{
			const std::string value = require<std::string>(p_key);
			const auto iterator = p_values.find(value);
			if (iterator == p_values.end())
			{
				std::string knownValues;
				for (const auto &[name, unused] : p_values)
				{
					(void)unused;
					if (!knownValues.empty())
					{
						knownValues += ", ";
					}
					knownValues += name;
				}

				throw JsonError(
					_file,
					pathFor(p_key),
					"unknown enum value '" + value + "' (expected one of: " + knownValues + ")");
			}

			return iterator->second;
		}

		[[nodiscard]] JsonReader child(const std::string &p_key) const;
		[[nodiscard]] std::vector<JsonReader> childArray(const std::string &p_key) const;
		void forbidUnknown(std::initializer_list<std::string_view> p_allowedKeys) const;
	};
}
