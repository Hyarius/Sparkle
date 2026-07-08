#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <map>
#include <ostream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "structures/container/spk_json_concepts.hpp"

namespace spk::JSON
{
	namespace detail
	{
		[[noreturn]] void raise(std::string_view p_message);
	}

	struct FormatOptions
	{
		bool pretty = true;
		std::size_t indentationSize = 4;
	};

	class Object
	{
	public:
		using Members = std::map<std::string, Object, std::less<>>;
		using Array = std::vector<Object>;

		enum class Type
		{
			Null,
			Boolean,
			Integer,
			Floating,
			String,
			Object,
			Array
		};

	private:
		using Storage = std::variant<
			std::nullptr_t,
			bool,
			std::int64_t,
			double,
			std::string,
			Members,
			Array>;

		Storage _storage = nullptr;

	public:
		Object() = default;
		Object(std::nullptr_t);
		Object(bool p_value);
		Object(const char *p_value);
		Object(std::string p_value);
		Object(std::string_view p_value);

		template <native_integer T>
		Object(T p_value)
		{
			set(p_value);
		}

		template <native_floating T>
		Object(T p_value)
		{
			set(p_value);
		}

		template <json_writable T>
		Object(const T &p_value)
		{
			set(p_value);
		}

		static Object null();
		static Object object();
		static Object array();

		static Object fromString(std::string_view p_content);
		static Object loadFromFile(const std::filesystem::path &p_path);

		void saveToFile(const std::filesystem::path &p_path, const FormatOptions &p_options = {}) const;
		std::string toString(const FormatOptions &p_options = {}) const;

		void reset();

		Type type() const;
		bool isNull() const;
		bool isBoolean() const;
		bool isInteger() const;
		bool isFloating() const;
		bool isNumber() const;
		bool isString() const;
		bool isObject() const;
		bool isArray() const;

		void setAsNull();
		void setAsObject();
		void setAsArray();

		Members &asObject();
		const Members &asObject() const;

		Array &asArray();
		const Array &asArray() const;

		bool contains(std::string_view p_key) const;
		std::size_t count(std::string_view p_key) const;

		Object *find(std::string_view p_key);
		const Object *find(std::string_view p_key) const;

		Object &at(std::string_view p_key);
		const Object &at(std::string_view p_key) const;

		Object &at(std::size_t p_index);
		const Object &at(std::size_t p_index) const;

		Object &operator[](std::string_view p_key);
		const Object &operator[](std::string_view p_key) const;

		Object &operator[](std::size_t p_index);
		const Object &operator[](std::size_t p_index) const;

		Object &append();
		void pushBack(const Object &p_value);
		void pushBack(Object &&p_value);
		void resize(std::size_t p_size);
		std::size_t size() const;
		bool empty() const;

		Object &operator=(std::nullptr_t);
		Object &operator=(bool p_value);
		Object &operator=(const char *p_value);
		Object &operator=(std::string p_value);
		Object &operator=(std::string_view p_value);

		template <native_integer T>
		Object &operator=(T p_value)
		{
			set(p_value);
			return *this;
		}

		template <native_floating T>
		Object &operator=(T p_value)
		{
			set(p_value);
			return *this;
		}

		template <json_writable T>
		Object &operator=(const T &p_value)
		{
			set(p_value);
			return *this;
		}

		void set(std::nullptr_t);
		void set(bool p_value);
		void set(const char *p_value);
		void set(std::string p_value);
		void set(std::string_view p_value);

		template <native_integer T>
		void set(T p_value)
		{
			if constexpr (std::signed_integral<T>)
			{
				_storage = static_cast<std::int64_t>(p_value);
			}
			else
			{
				if (p_value > static_cast<std::make_unsigned_t<std::int64_t>>(std::numeric_limits<std::int64_t>::max()))
				{
					detail::raise("Unsigned integer value is too large for JSON integer storage");
				}
				_storage = static_cast<std::int64_t>(p_value);
			}
		}

		template <native_floating T>
		void set(T p_value)
		{
			_storage = static_cast<double>(p_value);
		}

		template <json_writable T>
		void set(const T &p_value)
		{
			Object serialized = toJSON(p_value);
			_storage = std::move(serialized._storage);
		}

		template <typename T>
		bool holds() const
		{
			using CleanType = std::remove_cvref_t<T>;

			if constexpr (std::same_as<CleanType, std::nullptr_t>)
			{
				return isNull();
			}
			else if constexpr (std::same_as<CleanType, bool>)
			{
				return isBoolean();
			}
			else if constexpr (native_integer<CleanType>)
			{
				return isInteger();
			}
			else if constexpr (native_floating<CleanType>)
			{
				return isFloating() || isInteger();
			}
			else if constexpr (std::same_as<CleanType, std::string>)
			{
				return isString();
			}
			else
			{
				return false;
			}
		}

		template <typename T>
		T as() const
		{
			using CleanType = std::remove_cvref_t<T>;

			if constexpr (std::same_as<CleanType, std::nullptr_t>)
			{
				if (!isNull())
				{
					detail::raise("Wrong JSON type requested: expected null");
				}
				return nullptr;
			}
			else if constexpr (std::same_as<CleanType, bool>)
			{
				if (const bool *value = std::get_if<bool>(&_storage))
				{
					return *value;
				}
				detail::raise("Wrong JSON type requested: expected boolean");
			}
			else if constexpr (native_integer<CleanType>)
			{
				const std::int64_t *value = std::get_if<std::int64_t>(&_storage);
				if (value == nullptr)
				{
					detail::raise("Wrong JSON type requested: expected integer");
				}

				if constexpr (std::signed_integral<CleanType>)
				{
					if (*value < static_cast<std::int64_t>(std::numeric_limits<CleanType>::min()) ||
						*value > static_cast<std::int64_t>(std::numeric_limits<CleanType>::max()))
					{
						detail::raise("JSON integer value is outside of requested integer type range");
					}
				}
				else
				{
					if (*value < 0 || static_cast<std::uint64_t>(*value) > static_cast<std::uint64_t>(std::numeric_limits<CleanType>::max()))
					{
						detail::raise("JSON integer value is outside of requested unsigned integer type range");
					}
				}
				return static_cast<CleanType>(*value);
			}
			else if constexpr (native_floating<CleanType>)
			{
				if (const double *value = std::get_if<double>(&_storage))
				{
					return static_cast<CleanType>(*value);
				}
				if (const std::int64_t *value = std::get_if<std::int64_t>(&_storage))
				{
					return static_cast<CleanType>(*value);
				}
				detail::raise("Wrong JSON type requested: expected number");
			}
			else if constexpr (std::same_as<CleanType, std::string>)
			{
				if (const std::string *value = std::get_if<std::string>(&_storage))
				{
					return *value;
				}
				detail::raise("Wrong JSON type requested: expected string");
			}
			else if constexpr (json_readable<CleanType>)
			{
				CleanType result{};
				fromJSON(*this, result);
				return result;
			}
			else
			{
				static_assert(sizeof(CleanType) == 0, "Type is not readable from spk::JSON::Object");
			}
		}

		void write(std::ostream &p_stream, const FormatOptions &p_options = {}) const;

		friend std::ostream &operator<<(std::ostream &p_stream, const Object &p_object);
	};
}
