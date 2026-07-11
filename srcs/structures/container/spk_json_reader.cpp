#include "structures/container/spk_json_reader.hpp"

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

namespace spk::JSON
{
	Error::Error(std::filesystem::path p_file, std::string p_path, std::string p_message) :
		std::runtime_error(makeErrorMessage(p_file, p_path, p_message)),
		_file(std::move(p_file)),
		_path(std::move(p_path)),
		_message(std::move(p_message))
	{
	}

	const std::filesystem::path &Error::file() const noexcept
	{
		return _file;
	}

	const std::string &Error::path() const noexcept
	{
		return _path;
	}

	const std::string &Error::message() const noexcept
	{
		return _message;
	}

	spk::JSON::Value Loader::parseFile(const std::filesystem::path &p_file)
	{
		try
		{
			return spk::JSON::Value::loadFromFile(p_file);
		} catch (const std::exception &exception)
		{
			throw Error(p_file, "$", std::string("invalid JSON: ") + exception.what());
		}
	}

	Reader::Reader(const spk::JSON::Value &p_value, std::filesystem::path p_file, std::string p_path) :
		_value(p_value),
		_file(std::move(p_file)),
		_path(std::move(p_path))
	{
	}

	const std::filesystem::path &Reader::file() const noexcept
	{
		return _file;
	}

	const std::string &Reader::path() const noexcept
	{
		return _path;
	}

	const spk::JSON::Value &Reader::value() const noexcept
	{
		return _value;
	}

	std::string Reader::pathFor(const std::string &p_key) const
	{
		return _path + "." + p_key;
	}

	bool Reader::contains(const std::string &p_key) const
	{
		_requireObject();
		return _value.contains(p_key);
	}

	void Reader::_requireObject() const
	{
		if (!_value.isObject())
		{
			throw Error(_file, _path, "expected an object");
		}
	}

	const spk::JSON::Value &Reader::_requireMember(const std::string &p_key) const
	{
		_requireObject();
		const spk::JSON::Value *member = _value.find(p_key);
		if (member == nullptr)
		{
			throw Error(_file, pathFor(p_key), "missing required field");
		}
		return *member;
	}

	Reader Reader::child(const std::string &p_key) const
	{
		const spk::JSON::Value &member = _requireMember(p_key);
		if (!member.isObject())
		{
			throw Error(_file, pathFor(p_key), "expected an object");
		}
		return Reader(member, _file, pathFor(p_key));
	}

	std::vector<Reader> Reader::childArray(const std::string &p_key) const
	{
		const spk::JSON::Value &member = _requireMember(p_key);
		if (!member.isArray())
		{
			throw Error(_file, pathFor(p_key), "expected an array");
		}

		const spk::JSON::Value::Array &array = member.asArray();
		std::vector<Reader> result;
		result.reserve(array.size());
		for (std::size_t index = 0; index < array.size(); ++index)
		{
			const std::string elementPath = pathFor(p_key) + "[" + std::to_string(index) + "]";
			if (!array[index].isObject())
			{
				throw Error(_file, elementPath, "expected an object");
			}
			result.emplace_back(array[index], _file, elementPath);
		}
		return result;
	}

	void Reader::forbidUnknown(std::initializer_list<std::string_view> p_allowedKeys) const
	{
		_requireObject();
		const std::unordered_set<std::string_view> allowedKeys(p_allowedKeys);
		for (const auto &[key, unused] : _value.asObject())
		{
			(void)unused;
			if (!allowedKeys.contains(key))
			{
				throw Error(_file, pathFor(key), "unknown field");
			}
		}
	}
}
