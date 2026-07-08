#pragma once

#include "structures/container/spk_json_object.hpp"

#include <array>
#include <cstddef>
#include <filesystem>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
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
		[[nodiscard]] static spk::JSON::Value parseFile(const std::filesystem::path &p_file);
	};

	namespace detail
	{
		template <typename TType>
		struct JsonSequence : std::false_type
		{
		};
		template <typename TElement, typename TAllocator>
		struct JsonSequence<std::vector<TElement, TAllocator>> : std::true_type
		{
			using Element = TElement;
		};

		template <typename TType>
		struct JsonFixedArray : std::false_type
		{
		};
		template <typename TElement, std::size_t TCount>
		struct JsonFixedArray<std::array<TElement, TCount>> : std::true_type
		{
			using Element = TElement;
			static constexpr std::size_t Count = TCount;
		};

		// Converts a spk::JSON::Value into TType (scalars via Value::as, plus
		// std::vector / std::array of convertible elements). Throws std::runtime_error
		// on mismatch; JsonReader wraps that with file/path context.
		template <typename TType>
		[[nodiscard]] TType readJsonValue(const spk::JSON::Value &p_value)
		{
			if constexpr (JsonSequence<TType>::value)
			{
				using Element = typename JsonSequence<TType>::Element;
				TType result;
				const spk::JSON::Value::Array &array = p_value.asArray();
				result.reserve(array.size());
				for (const spk::JSON::Value &element : array)
				{
					result.push_back(readJsonValue<Element>(element));
				}
				return result;
			}
			else if constexpr (JsonFixedArray<TType>::value)
			{
				using Element = typename JsonFixedArray<TType>::Element;
				const spk::JSON::Value::Array &array = p_value.asArray();
				if (array.size() != JsonFixedArray<TType>::Count)
				{
					throw std::runtime_error(
						"expected exactly " + std::to_string(JsonFixedArray<TType>::Count) + " elements, got " +
						std::to_string(array.size()));
				}
				TType result{};
				for (std::size_t index = 0; index < array.size(); ++index)
				{
					result[index] = readJsonValue<Element>(array[index]);
				}
				return result;
			}
			else
			{
				return p_value.as<TType>();
			}
		}
	}

	class JsonReader
	{
	private:
		const spk::JSON::Value &_value;
		std::filesystem::path _file;
		std::string _path;

		void _requireObject() const;
		[[nodiscard]] const spk::JSON::Value &_requireMember(const std::string &p_key) const;

	public:
		JsonReader(const spk::JSON::Value &p_value, std::filesystem::path p_file, std::string p_path = "$");

		[[nodiscard]] const std::filesystem::path &file() const noexcept;
		[[nodiscard]] const std::string &path() const noexcept;
		[[nodiscard]] const spk::JSON::Value &value() const noexcept;
		[[nodiscard]] std::string pathFor(const std::string &p_key) const;
		[[nodiscard]] bool contains(const std::string &p_key) const;

		template <typename TType>
		[[nodiscard]] TType require(const std::string &p_key) const
		{
			const spk::JSON::Value &member = _requireMember(p_key);

			try
			{
				return detail::readJsonValue<TType>(member);
			} catch (const std::exception &exception)
			{
				throw JsonError(_file, pathFor(p_key), std::string("invalid value: ") + exception.what());
			}
		}

		template <typename TType>
		[[nodiscard]] TType optional(const std::string &p_key, TType p_default) const
		{
			_requireObject();
			const spk::JSON::Value *member = _value.find(p_key);
			if (member == nullptr)
			{
				return p_default;
			}

			try
			{
				return detail::readJsonValue<TType>(*member);
			} catch (const std::exception &exception)
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
