#include "structures/system/spk_command_line_parser.hpp"

#include <algorithm>
#include <cstdint>
#include <utility>
#include <vector>

namespace spk
{
	namespace
	{
		std::string deriveKey(std::string_view p_longName)
		{
			std::size_t start = 0;
			while (start < p_longName.size() && p_longName[start] == '-')
			{
				++start;
			}
			return std::string(p_longName.substr(start));
		}

		std::string typePlaceholder(CommandLineParser::Option::Type p_type)
		{
			switch (p_type)
			{
			case CommandLineParser::Option::Type::Integer:
				return "<int>";
			case CommandLineParser::Option::Type::Float:
				return "<float>";
			case CommandLineParser::Option::Type::String:
				return "<string>";
			case CommandLineParser::Option::Type::Flag:
				break;
			}
			return "";
		}

		std::string valueSuffix(const CommandLineParser::Option &p_option)
		{
			const std::string placeholder = typePlaceholder(p_option.type);
			std::string suffix;
			for (std::size_t element = 0; element < p_option.count; ++element)
			{
				suffix += " " + placeholder;
			}
			return suffix;
		}

		std::string deriveApplicationName(std::string_view p_arg0)
		{
			const std::size_t slash = p_arg0.find_last_of("/\\");
			std::string_view name = (slash == std::string_view::npos) ? p_arg0 : p_arg0.substr(slash + 1);
			if (name.size() >= 4)
			{
				const std::string_view tail = name.substr(name.size() - 4);
				if ((tail[0] == '.') && (tail[1] == 'e' || tail[1] == 'E') && (tail[2] == 'x' || tail[2] == 'X') &&
					(tail[3] == 'e' || tail[3] == 'E'))
				{
					name = name.substr(0, name.size() - 4);
				}
			}
			return std::string(name);
		}
	}

	CommandLineParser::Error::Error(const std::string &p_message) :
		std::runtime_error(p_message)
	{
	}

	void CommandLineParser::addOption(std::string_view p_longName, const Option &p_option)
	{
		_registerEntry(p_longName, {}, p_option, false, spk::JSON::Value());
	}

	void CommandLineParser::addOption(std::string_view p_longName, std::string_view p_shortName, const Option &p_option)
	{
		_registerEntry(p_longName, p_shortName, p_option, false, spk::JSON::Value());
	}

	void CommandLineParser::addOption(std::string_view p_longName, const Option &p_option, spk::JSON::Value p_default)
	{
		_registerEntry(p_longName, {}, p_option, true, std::move(p_default));
	}

	void CommandLineParser::addOption(
		std::string_view p_longName, std::string_view p_shortName, const Option &p_option, spk::JSON::Value p_default)
	{
		_registerEntry(p_longName, p_shortName, p_option, true, std::move(p_default));
	}

	void CommandLineParser::_registerEntry(std::string_view p_longName, std::string_view p_shortName,
		const Option &p_option, bool p_hasDefault, spk::JSON::Value p_default)
	{
		if (p_longName.empty() || p_longName.front() != '-')
		{
			throw Error("CommandLineParser: option name must start with '-' (got '" + std::string(p_longName) + "')");
		}
		if (!p_shortName.empty() && p_shortName.front() != '-')
		{
			throw Error("CommandLineParser: short name must start with '-' (got '" + std::string(p_shortName) + "')");
		}

		Entry entry;
		entry.longName = std::string(p_longName);
		entry.shortName = std::string(p_shortName);
		entry.key = deriveKey(p_longName);
		entry.option = p_option;
		entry.hasDefault = p_hasDefault;
		entry.defaultValue = std::move(p_default);

		if (entry.key.empty())
		{
			throw Error("CommandLineParser: option '" + entry.longName + "' has no name beyond its dashes");
		}
		if (entry.option.type == Option::Type::Flag)
		{
			entry.option.count = 0;
		}
		else if (entry.option.count == 0)
		{
			throw Error("CommandLineParser: option '" + entry.longName + "' must accept at least one value");
		}
		if (_byName.contains(entry.longName) || (!entry.shortName.empty() && _byName.contains(entry.shortName)))
		{
			throw Error("CommandLineParser: option '" + entry.longName + "' is already registered");
		}

		const std::size_t index = _entries.size();
		_entries.push_back(std::move(entry));
		_byName.emplace(_entries.back().longName, index);
		if (!_entries.back().shortName.empty())
		{
			_byName.emplace(_entries.back().shortName, index);
		}
	}

	const CommandLineParser::Entry *CommandLineParser::_lookup(std::string_view p_name) const
	{
		const auto found = _byName.find(p_name);
		if (found == _byName.end())
		{
			return nullptr;
		}
		return &_entries[found->second];
	}

	spk::JSON::Value CommandLineParser::_convert(
		const Option &p_option, const std::string &p_token, const std::string &p_name)
	{
		try
		{
			std::size_t consumed = 0;
			switch (p_option.type)
			{
			case Option::Type::Integer:
			{
				const long long value = std::stoll(p_token, &consumed);
				if (consumed != p_token.size())
				{
					throw std::invalid_argument("trailing characters");
				}
				return spk::JSON::Value(static_cast<std::int64_t>(value));
			}
			case Option::Type::Float:
			{
				const double value = std::stod(p_token, &consumed);
				if (consumed != p_token.size())
				{
					throw std::invalid_argument("trailing characters");
				}
				return spk::JSON::Value(value);
			}
			case Option::Type::String:
				return spk::JSON::Value(p_token);
			case Option::Type::Flag:
				break;
			}
		} catch (const std::exception &)
		{
			throw Error(p_name + " expects " + typePlaceholder(p_option.type) + ", got '" + p_token + "'");
		}
		return spk::JSON::Value();
	}

	spk::JSON::Value CommandLineParser::_readValues(
		const Entry &p_entry, int p_argc, const char *const *p_argv, int &p_index) const
	{
		if (p_index + static_cast<int>(p_entry.option.count) >= p_argc)
		{
			throw Error(
				p_entry.longName + " expects " + std::to_string(p_entry.option.count) + " value(s)");
		}

		if (p_entry.option.count == 1)
		{
			return _convert(p_entry.option, p_argv[++p_index], p_entry.longName);
		}

		spk::JSON::Value values = spk::JSON::Value::array();
		for (std::size_t element = 0; element < p_entry.option.count; ++element)
		{
			values.pushBack(_convert(p_entry.option, p_argv[++p_index], p_entry.longName));
		}
		return values;
	}

	const spk::JSON::Value &CommandLineParser::parse(int p_argc, const char *const *p_argv)
	{
		if (!_applicationNameExplicit && p_argc >= 1 && p_argv[0] != nullptr)
		{
			_applicationName = deriveApplicationName(p_argv[0]);
		}

		_arguments = spk::JSON::Value::object();

		for (const Entry &entry : _entries)
		{
			if (entry.hasDefault)
			{
				_arguments[entry.key] = entry.defaultValue;
			}
		}

		for (int index = 1; index < p_argc; ++index)
		{
			const std::string token = p_argv[index];
			const Entry *entry = _lookup(token);
			if (entry == nullptr)
			{
				throw Error("CommandLineParser: unknown argument '" + token + "'");
			}

			if (entry->option.type == Option::Type::Flag)
			{
				_arguments[entry->key] = true;
			}
			else
			{
				_arguments[entry->key] = _readValues(*entry, p_argc, p_argv, index);
			}
		}

		return _arguments;
	}

	const spk::JSON::Value &CommandLineParser::arguments() const noexcept
	{
		return _arguments;
	}

	void CommandLineParser::setApplicationName(std::string p_name)
	{
		_applicationName = std::move(p_name);
		_applicationNameExplicit = true;
	}

	const std::string &CommandLineParser::applicationName() const noexcept
	{
		return _applicationName;
	}

	std::string CommandLineParser::usage() const
	{
		std::string synopsis = "Usage: " + (_applicationName.empty() ? std::string("program") : _applicationName);
		for (const Entry &entry : _entries)
		{
			synopsis += " [" + entry.longName + valueSuffix(entry.option) + "]";
		}

		std::string result = synopsis + "\n";
		if (_entries.empty())
		{
			return result;
		}

		std::vector<std::string> invocations;
		invocations.reserve(_entries.size());
		std::size_t widest = 0;
		for (const Entry &entry : _entries)
		{
			std::string invocation = entry.longName;
			if (!entry.shortName.empty())
			{
				invocation += ", " + entry.shortName;
			}
			invocation += valueSuffix(entry.option);
			widest = std::max(widest, invocation.size());
			invocations.push_back(std::move(invocation));
		}

		result += "\nOptions:\n";
		for (std::size_t index = 0; index < _entries.size(); ++index)
		{
			result += "  " + invocations[index];
			if (!_entries[index].option.help.empty())
			{
				result += std::string(widest - invocations[index].size() + 2, ' ') + _entries[index].option.help;
			}
			result += "\n";
		}
		return result;
	}
}
