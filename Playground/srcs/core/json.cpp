#include "core/json.hpp"

#include <fstream>
#include <unordered_set>

namespace
{
	std::string makeErrorMessage(
		const std::filesystem::path &p_file,
		const std::string &p_path,
		const std::string &p_message)
	{
		return p_file.generic_string() + ":" + p_path + ": " + p_message;
	}
}

namespace pg
{
	JsonError::JsonError(std::filesystem::path p_file, std::string p_path, std::string p_message) :
		std::runtime_error(makeErrorMessage(p_file, p_path, p_message)),
		_file(std::move(p_file)),
		_path(std::move(p_path)),
		_message(std::move(p_message))
	{
	}

	const std::filesystem::path &JsonError::file() const noexcept
	{
		return _file;
	}

	const std::string &JsonError::path() const noexcept
	{
		return _path;
	}

	const std::string &JsonError::message() const noexcept
	{
		return _message;
	}

	nlohmann::json JsonLoader::parseFile(const std::filesystem::path &p_file)
	{
		std::ifstream stream(p_file);
		if (!stream.is_open())
		{
			throw JsonError(p_file, "$", "unable to open file");
		}

		try
		{
			return nlohmann::json::parse(stream);
		} catch (const nlohmann::json::parse_error &exception)
		{
			throw JsonError(p_file, "$", std::string("invalid JSON: ") + exception.what());
		}
	}

	JsonReader::JsonReader(const nlohmann::json &p_value, std::filesystem::path p_file, std::string p_path) :
		_value(p_value),
		_file(std::move(p_file)),
		_path(std::move(p_path))
	{
	}

	const std::filesystem::path &JsonReader::file() const noexcept
	{
		return _file;
	}

	const std::string &JsonReader::path() const noexcept
	{
		return _path;
	}

	std::string JsonReader::pathFor(const std::string &p_key) const
	{
		return _path + "." + p_key;
	}

	void JsonReader::_requireObject() const
	{
		if (!_value.is_object())
		{
			throw JsonError(_file, _path, "expected an object");
		}
	}

	const nlohmann::json &JsonReader::_requireMember(const std::string &p_key) const
	{
		_requireObject();
		const auto iterator = _value.find(p_key);
		if (iterator == _value.end())
		{
			throw JsonError(_file, pathFor(p_key), "missing required field");
		}
		return *iterator;
	}

	JsonReader JsonReader::child(const std::string &p_key) const
	{
		const nlohmann::json &member = _requireMember(p_key);
		if (!member.is_object())
		{
			throw JsonError(_file, pathFor(p_key), "expected an object");
		}
		return JsonReader(member, _file, pathFor(p_key));
	}

	std::vector<JsonReader> JsonReader::childArray(const std::string &p_key) const
	{
		const nlohmann::json &member = _requireMember(p_key);
		if (!member.is_array())
		{
			throw JsonError(_file, pathFor(p_key), "expected an array");
		}

		std::vector<JsonReader> result;
		result.reserve(member.size());
		for (std::size_t index = 0; index < member.size(); ++index)
		{
			const std::string elementPath = pathFor(p_key) + "[" + std::to_string(index) + "]";
			if (!member[index].is_object())
			{
				throw JsonError(_file, elementPath, "expected an object");
			}
			result.emplace_back(member[index], _file, elementPath);
		}
		return result;
	}

	void JsonReader::forbidUnknown(std::initializer_list<std::string_view> p_allowedKeys) const
	{
		_requireObject();
		const std::unordered_set<std::string_view> allowedKeys(p_allowedKeys);
		for (const auto &[key, unused] : _value.items())
		{
			(void)unused;
			if (!allowedKeys.contains(key))
			{
				throw JsonError(_file, pathFor(key), "unknown field");
			}
		}
	}
}
