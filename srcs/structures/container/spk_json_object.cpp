#include "structures/container/spk_json_object.hpp"

#include <charconv>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace spk::JSON
{
	namespace detail
	{
		[[noreturn]] void raise(std::string_view p_message)
		{
			throw std::runtime_error(std::string(p_message));
		}
	}

	Object::Object(std::nullptr_t)
	{
		set(nullptr);
	}

	Object::Object(bool p_value)
	{
		set(p_value);
	}

	Object::Object(const char *p_value)
	{
		set(p_value);
	}

	Object::Object(std::string p_value)
	{
		set(std::move(p_value));
	}

	Object::Object(std::string_view p_value)
	{
		set(p_value);
	}

	Object Object::null()
	{
		return Object(nullptr);
	}

	Object Object::object()
	{
		Object result;
		result.setAsObject();
		return result;
	}

	Object Object::array()
	{
		Object result;
		result.setAsArray();
		return result;
	}

	void Object::reset()
	{
		_storage = nullptr;
	}

	Object::Type Object::type() const
	{
		switch (_storage.index())
		{
		case 0:
			return Type::Null;
		case 1:
			return Type::Boolean;
		case 2:
			return Type::Integer;
		case 3:
			return Type::Floating;
		case 4:
			return Type::String;
		case 5:
			return Type::Object;
		case 6:
			return Type::Array;
		default:
			detail::raise("Invalid JSON storage state");
		}
	}

	bool Object::isNull() const
	{
		return std::holds_alternative<std::nullptr_t>(_storage);
	}

	bool Object::isBoolean() const
	{
		return std::holds_alternative<bool>(_storage);
	}

	bool Object::isInteger() const
	{
		return std::holds_alternative<std::int64_t>(_storage);
	}

	bool Object::isFloating() const
	{
		return std::holds_alternative<double>(_storage);
	}

	bool Object::isNumber() const
	{
		return isInteger() || isFloating();
	}

	bool Object::isString() const
	{
		return std::holds_alternative<std::string>(_storage);
	}

	bool Object::isObject() const
	{
		return std::holds_alternative<Members>(_storage);
	}

	bool Object::isArray() const
	{
		return std::holds_alternative<Array>(_storage);
	}

	void Object::setAsNull()
	{
		_storage = nullptr;
	}

	void Object::setAsObject()
	{
		_storage = Members{};
	}

	void Object::setAsArray()
	{
		_storage = Array{};
	}

	Object::Members &Object::asObject()
	{
		if (Members *members = std::get_if<Members>(&_storage))
		{
			return *members;
		}
		detail::raise("JSON value is not an object");
	}

	const Object::Members &Object::asObject() const
	{
		if (const Members *members = std::get_if<Members>(&_storage))
		{
			return *members;
		}
		detail::raise("JSON value is not an object");
	}

	Object::Array &Object::asArray()
	{
		if (Array *array = std::get_if<Array>(&_storage))
		{
			return *array;
		}
		detail::raise("JSON value is not an array");
	}

	const Object::Array &Object::asArray() const
	{
		if (const Array *array = std::get_if<Array>(&_storage))
		{
			return *array;
		}
		detail::raise("JSON value is not an array");
	}

	bool Object::contains(std::string_view p_key) const
	{
		const Members &members = asObject();
		return members.find(p_key) != members.end();
	}

	std::size_t Object::count(std::string_view p_key) const
	{
		return contains(p_key) ? 1u : 0u;
	}

	Object *Object::find(std::string_view p_key)
	{
		Members &members = asObject();
		auto it = members.find(p_key);
		return it == members.end() ? nullptr : &it->second;
	}

	const Object *Object::find(std::string_view p_key) const
	{
		const Members &members = asObject();
		auto it = members.find(p_key);
		return it == members.end() ? nullptr : &it->second;
	}

	Object &Object::at(std::string_view p_key)
	{
		if (Object *value = find(p_key))
		{
			return *value;
		}
		detail::raise("Missing JSON object member: " + std::string(p_key));
	}

	const Object &Object::at(std::string_view p_key) const
	{
		if (const Object *value = find(p_key))
		{
			return *value;
		}
		detail::raise("Missing JSON object member: " + std::string(p_key));
	}

	Object &Object::at(std::size_t p_index)
	{
		Array &array = asArray();
		if (p_index >= array.size())
		{
			detail::raise("JSON array index is out of range");
		}
		return array[p_index];
	}

	const Object &Object::at(std::size_t p_index) const
	{
		const Array &array = asArray();
		if (p_index >= array.size())
		{
			detail::raise("JSON array index is out of range");
		}
		return array[p_index];
	}

	Object &Object::operator[](std::string_view p_key)
	{
		if (isNull())
		{
			setAsObject();
		}
		return asObject()[std::string(p_key)];
	}

	const Object &Object::operator[](std::string_view p_key) const
	{
		return at(p_key);
	}

	Object &Object::operator[](std::size_t p_index)
	{
		return at(p_index);
	}

	const Object &Object::operator[](std::size_t p_index) const
	{
		return at(p_index);
	}

	Object &Object::append()
	{
		if (isNull())
		{
			setAsArray();
		}
		Array &array = asArray();
		array.emplace_back();
		return array.back();
	}

	void Object::pushBack(const Object &p_value)
	{
		append() = p_value;
	}

	void Object::pushBack(Object &&p_value)
	{
		if (isNull())
		{
			setAsArray();
		}
		asArray().push_back(std::move(p_value));
	}

	void Object::resize(std::size_t p_size)
	{
		if (isNull())
		{
			setAsArray();
		}
		asArray().resize(p_size);
	}

	std::size_t Object::size() const
	{
		if (const Members *members = std::get_if<Members>(&_storage))
		{
			return members->size();
		}
		if (const Array *array = std::get_if<Array>(&_storage))
		{
			return array->size();
		}
		detail::raise("Only JSON object and array values have a size");
	}

	bool Object::empty() const
	{
		return size() == 0;
	}

	Object &Object::operator=(std::nullptr_t)
	{
		set(nullptr);
		return *this;
	}

	Object &Object::operator=(bool p_value)
	{
		set(p_value);
		return *this;
	}

	Object &Object::operator=(const char *p_value)
	{
		set(p_value);
		return *this;
	}

	Object &Object::operator=(std::string p_value)
	{
		set(std::move(p_value));
		return *this;
	}

	Object &Object::operator=(std::string_view p_value)
	{
		set(p_value);
		return *this;
	}

	void Object::set(std::nullptr_t)
	{
		_storage = nullptr;
	}

	void Object::set(bool p_value)
	{
		_storage = p_value;
	}

	void Object::set(const char *p_value)
	{
		if (p_value == nullptr)
		{
			set(nullptr);
			return;
		}
		_storage = std::string(p_value);
	}

	void Object::set(std::string p_value)
	{
		_storage = std::move(p_value);
	}

	void Object::set(std::string_view p_value)
	{
		_storage = std::string(p_value);
	}

	namespace
	{
		class Parser
		{
		private:
			std::string_view _source;
			std::size_t _index = 0;

		public:
			explicit Parser(std::string_view p_source) :
				_source(p_source)
			{
			}

			Object parse()
			{
				skipWhitespaces();
				Object result = parseValue();
				skipWhitespaces();
				if (!isAtEnd())
				{
					error("Unexpected trailing characters after JSON root");
				}
				return result;
			}

		private:
			[[noreturn]] void error(std::string_view p_message) const
			{
				std::ostringstream stream;
				stream << p_message << " at line " << line() << ", column " << column();
				detail::raise(stream.str());
			}

			bool isAtEnd() const
			{
				return _index >= _source.size();
			}

			char peek() const
			{
				if (isAtEnd())
				{
					error("Unexpected end of JSON input");
				}
				return _source[_index];
			}

			char consume()
			{
				char result = peek();
				++_index;
				return result;
			}

			bool consumeIf(char p_expected)
			{
				if (!isAtEnd() && _source[_index] == p_expected)
				{
					++_index;
					return true;
				}
				return false;
			}

			void expect(char p_expected, std::string_view p_message)
			{
				if (!consumeIf(p_expected))
				{
					error(p_message);
				}
			}

			void skipWhitespaces()
			{
				while (!isAtEnd())
				{
					switch (_source[_index])
					{
					case ' ':
					case '\t':
					case '\n':
					case '\r':
						++_index;
						break;
					default:
						return;
					}
				}
			}

			std::size_t line() const
			{
				std::size_t result = 1;
				for (std::size_t i = 0; i < _index && i < _source.size(); ++i)
				{
					if (_source[i] == '\n')
					{
						++result;
					}
				}
				return result;
			}

			std::size_t column() const
			{
				std::size_t result = 1;
				for (std::size_t i = _index; i > 0; --i)
				{
					if (_source[i - 1] == '\n')
					{
						break;
					}
					++result;
				}
				return result;
			}

			Object parseValue()
			{
				skipWhitespaces();
				if (isAtEnd())
				{
					error("Unexpected end of JSON input");
				}

				switch (peek())
				{
				case 'n':
					parseLiteral("null");
					return Object(nullptr);
				case 't':
					parseLiteral("true");
					return Object(true);
				case 'f':
					parseLiteral("false");
					return Object(false);
				case '"':
					return Object(parseString());
				case '{':
					return parseObject();
				case '[':
					return parseArray();
				case '-':
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
					return parseNumber();
				default:
					error("Unexpected character while parsing JSON value");
				}
			}

			void parseLiteral(std::string_view p_literal)
			{
				for (char expected : p_literal)
				{
					if (consume() != expected)
					{
						error("Invalid JSON literal");
					}
				}
			}

			Object parseObject()
			{
				Object result = Object::object();
				expect('{', "Expected '{'");
				skipWhitespaces();
				if (consumeIf('}'))
				{
					return result;
				}

				while (true)
				{
					skipWhitespaces();
					if (peek() != '"')
					{
						error("Expected string object key");
					}

					std::string key = parseString();
					skipWhitespaces();
					expect(':', "Expected ':' after object key");
					Object value = parseValue();

					auto [it, inserted] = result.asObject().emplace(std::move(key), std::move(value));
					if (!inserted)
					{
						error("Duplicate JSON object key");
					}

					skipWhitespaces();
					if (consumeIf('}'))
					{
						return result;
					}
					expect(',', "Expected ',' or '}' after object member");
				}
			}

			Object parseArray()
			{
				Object result = Object::array();
				expect('[', "Expected '['");
				skipWhitespaces();
				if (consumeIf(']'))
				{
					return result;
				}

				while (true)
				{
					result.pushBack(parseValue());
					skipWhitespaces();
					if (consumeIf(']'))
					{
						return result;
					}
					expect(',', "Expected ',' or ']' after array value");
				}
			}

			std::string parseString()
			{
				expect('"', "Expected string opening quote");

				std::string result;
				while (!isAtEnd())
				{
					char current = consume();
					if (current == '"')
					{
						return result;
					}
					if (current == '\\')
					{
						result += parseEscapeSequence();
						continue;
					}
					if (current >= 0 && current < 0x20)
					{
						error("Control characters are not allowed in JSON strings");
					}
					result += current;
				}

				error("Unterminated JSON string");
			}

			std::string parseEscapeSequence()
			{
				char escaped = consume();
				switch (escaped)
				{
				case '"':
					return "\"";
				case '\\':
					return "\\";
				case '/':
					return "/";
				case 'b':
					return "\b";
				case 'f':
					return "\f";
				case 'n':
					return "\n";
				case 'r':
					return "\r";
				case 't':
					return "\t";
				case 'u':
					return parseUnicodeEscapeAsString();
				default:
					error("Invalid JSON escape sequence");
				}
			}

			static bool isHexDigit(char p_character)
			{
				return (p_character >= '0' && p_character <= '9') ||
					   (p_character >= 'a' && p_character <= 'f') ||
					   (p_character >= 'A' && p_character <= 'F');
			}

			static std::uint32_t hexValue(char p_character)
			{
				if (p_character >= '0' && p_character <= '9')
				{
					return static_cast<std::uint32_t>(p_character - '0');
				}
				if (p_character >= 'a' && p_character <= 'f')
				{
					return static_cast<std::uint32_t>(10 + p_character - 'a');
				}
				if (p_character >= 'A' && p_character <= 'F')
				{
					return static_cast<std::uint32_t>(10 + p_character - 'A');
				}
				return 0;
			}

			std::uint32_t parseUnicodeEscapeCodeUnit()
			{
				std::uint32_t result = 0;
				for (std::size_t i = 0; i < 4; ++i)
				{
					char c = consume();
					if (!isHexDigit(c))
					{
						error("Invalid JSON unicode escape sequence");
					}
					result = (result << 4) | hexValue(c);
				}
				return result;
			}

			std::string parseUnicodeEscapeAsString()
			{
				std::uint32_t codePoint = parseUnicodeEscapeCodeUnit();

				if (codePoint >= 0xD800 && codePoint <= 0xDBFF)
				{
					expect('\\', "Expected low surrogate unicode escape");
					expect('u', "Expected low surrogate unicode escape");

					const std::uint32_t low = parseUnicodeEscapeCodeUnit();
					if (low < 0xDC00 || low > 0xDFFF)
					{
						error("Invalid unicode surrogate pair");
					}

					codePoint = 0x10000 + ((codePoint - 0xD800) << 10) + (low - 0xDC00);
				}
				else if (codePoint >= 0xDC00 && codePoint <= 0xDFFF)
				{
					error("Unexpected low unicode surrogate");
				}

				return codePointToString(codePoint);
			}

			static std::string codePointToString(std::uint32_t p_codePoint)
			{
				std::string result;
				if (p_codePoint <= 0x7F)
				{
					result.push_back(static_cast<char>(p_codePoint));
				}
				else if (p_codePoint <= 0x7FF)
				{
					result.push_back(static_cast<char>(0xC0 | (p_codePoint >> 6)));
					result.push_back(static_cast<char>(0x80 | (p_codePoint & 0x3F)));
				}
				else if (p_codePoint <= 0xFFFF)
				{
					result.push_back(static_cast<char>(0xE0 | (p_codePoint >> 12)));
					result.push_back(static_cast<char>(0x80 | ((p_codePoint >> 6) & 0x3F)));
					result.push_back(static_cast<char>(0x80 | (p_codePoint & 0x3F)));
				}
				else
				{
					result.push_back(static_cast<char>(0xF0 | (p_codePoint >> 18)));
					result.push_back(static_cast<char>(0x80 | ((p_codePoint >> 12) & 0x3F)));
					result.push_back(static_cast<char>(0x80 | ((p_codePoint >> 6) & 0x3F)));
					result.push_back(static_cast<char>(0x80 | (p_codePoint & 0x3F)));
				}

				return result;
			}

			Object parseNumber()
			{
				const std::size_t start = _index;

				consumeIf('-');

				if (consumeIf('0'))
				{
					if (!isAtEnd() && peek() >= '0' && peek() <= '9')
					{
						error("Leading zeroes are not allowed in JSON numbers");
					}
				}
				else
				{
					consumeDigits("Expected digit in JSON number");
				}

				bool isFloating = false;

				if (consumeIf('.'))
				{
					isFloating = true;
					consumeDigits("Expected digit after decimal point");
				}

				if (!isAtEnd() && (peek() == 'e' || peek() == 'E'))
				{
					isFloating = true;
					consume();
					if (!isAtEnd() && (peek() == '+' || peek() == '-'))
					{
						consume();
					}
					consumeDigits("Expected digit in exponent");
				}

				const std::string number(_source.substr(start, _index - start));

				if (isFloating)
				{
					double value = 0.0;
					const auto [end, parseError] = std::from_chars(
						number.data(), number.data() + number.size(), value, std::chars_format::general);
					if (parseError != std::errc{} || end != number.data() + number.size() || !std::isfinite(value))
					{
						error("Invalid floating JSON number");
					}
					return Object(value);
				}

				std::int64_t value = 0;
				const auto [end, parseError] = std::from_chars(number.data(), number.data() + number.size(), value);
				if (parseError != std::errc{} || end != number.data() + number.size())
				{
					error("Invalid integer JSON number");
				}
				return Object(value);
			}

			void consumeDigits(std::string_view p_errorMessage)
			{
				if (isAtEnd() || peek() < '0' || peek() > '9')
				{
					error(p_errorMessage);
				}
				while (!isAtEnd() && peek() >= '0' && peek() <= '9')
				{
					consume();
				}
			}
		};

		static std::string escapeString(std::string_view p_value)
		{
			std::ostringstream stream;
			stream << '"';

			for (char c : p_value)
			{
				switch (c)
				{
				case '"':
					stream << "\\\"";
					break;
				case '\\':
					stream << "\\\\";
					break;
				case '\b':
					stream << "\\b";
					break;
				case '\f':
					stream << "\\f";
					break;
				case '\n':
					stream << "\\n";
					break;
				case '\r':
					stream << "\\r";
					break;
				case '\t':
					stream << "\\t";
					break;
				default:
					if (c >= 0 && c < 0x20)
					{
						stream << "\\u" << std::uppercase << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c)
							   << std::nouppercase << std::dec << std::setfill(' ');
					}
					else
					{
						stream << c;
					}
					break;
				}
			}

			stream << '"';
			return stream.str();
		}

		class Writer
		{
		private:
			std::ostream &_stream;
			FormatOptions _options;

		public:
			Writer(std::ostream &p_stream, const FormatOptions &p_options) :
				_stream(p_stream),
				_options(p_options)
			{
			}

			void write(const Object &p_value)
			{
				writeValue(p_value, 0);
			}

		private:
			void writeIndent(std::size_t p_depth)
			{
				if (!_options.pretty)
				{
					return;
				}
				for (std::size_t i = 0; i < p_depth * _options.indentationSize; ++i)
				{
					_stream << ' ';
				}
			}

			void writeNewLine()
			{
				if (_options.pretty)
				{
					_stream << '\n';
				}
			}

			void writeValue(const Object &p_value, std::size_t p_depth)
			{
				switch (p_value.type())
				{
				case Object::Type::Null:
					_stream << "null";
					break;
				case Object::Type::Boolean:
					_stream << (p_value.as<bool>() ? "true" : "false");
					break;
				case Object::Type::Integer:
					_stream << p_value.as<std::int64_t>();
					break;
				case Object::Type::Floating:
					_stream << std::setprecision(std::numeric_limits<double>::max_digits10) << p_value.as<double>();
					break;
				case Object::Type::String:
					_stream << escapeString(p_value.as<std::string>());
					break;
				case Object::Type::Object:
					writeObject(p_value.asObject(), p_depth);
					break;
				case Object::Type::Array:
					writeArray(p_value.asArray(), p_depth);
					break;
				}
			}

			void writeObject(const Object::Members &p_members, std::size_t p_depth)
			{
				_stream << '{';
				if (p_members.empty())
				{
					_stream << '}';
					return;
				}

				writeNewLine();

				std::size_t index = 0;
				for (const auto &[key, value] : p_members)
				{
					writeIndent(p_depth + 1);
					_stream << escapeString(key) << (_options.pretty ? ": " : ":");
					writeValue(value, p_depth + 1);
					if (++index != p_members.size())
					{
						_stream << ',';
					}
					writeNewLine();
				}

				writeIndent(p_depth);
				_stream << '}';
			}

			void writeArray(const Object::Array &p_array, std::size_t p_depth)
			{
				_stream << '[';
				if (p_array.empty())
				{
					_stream << ']';
					return;
				}

				writeNewLine();

				for (std::size_t i = 0; i < p_array.size(); ++i)
				{
					writeIndent(p_depth + 1);
					writeValue(p_array[i], p_depth + 1);
					if (i + 1 != p_array.size())
					{
						_stream << ',';
					}
					writeNewLine();
				}

				writeIndent(p_depth);
				_stream << ']';
			}
		};
	}

	Object Object::fromString(std::string_view p_content)
	{
		return Parser(p_content).parse();
	}

	Object Object::loadFromFile(const std::filesystem::path &p_path)
	{
		std::ifstream file(p_path);
		if (!file.is_open())
		{
			detail::raise("Unable to open JSON file");
		}

		std::stringstream stream;
		stream << file.rdbuf();
		return fromString(stream.str());
	}

	void Object::saveToFile(const std::filesystem::path &p_path, const FormatOptions &p_options) const
	{
		std::ofstream file(p_path);
		if (!file.is_open())
		{
			detail::raise("Unable to open JSON file for writing");
		}
		write(file, p_options);
	}

	std::string Object::toString(const FormatOptions &p_options) const
	{
		std::ostringstream stream;
		write(stream, p_options);
		return stream.str();
	}

	void Object::write(std::ostream &p_stream, const FormatOptions &p_options) const
	{
		Writer(p_stream, p_options).write(*this);
	}

	std::ostream &operator<<(std::ostream &p_stream, const Object &p_object)
	{
		p_object.write(p_stream);
		return p_stream;
	}
}
