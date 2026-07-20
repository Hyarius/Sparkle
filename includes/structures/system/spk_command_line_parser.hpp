#pragma once

#include <cstddef>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "structures/container/spk_json_object.hpp"

namespace spk
{
	class CommandLineParser
	{
	public:
		struct Option
		{
			enum class Type
			{
				Flag,
				Integer,
				Float,
				String
			};

			Type type = Type::Flag;
			std::size_t count = 1;
			std::string help;
		};

		class Error : public std::runtime_error
		{
		public:
			explicit Error(const std::string &p_message);
		};

		void addOption(std::string_view p_longName, const Option &p_option);
		void addOption(std::string_view p_longName, std::string_view p_shortName, const Option &p_option);
		void addOption(std::string_view p_longName, const Option &p_option, spk::JSON::Value p_default);
		void addOption(
			std::string_view p_longName, std::string_view p_shortName, const Option &p_option, spk::JSON::Value p_default);

		const spk::JSON::Value &parse(int p_argc, const char *const *p_argv);
		[[nodiscard]] const spk::JSON::Value &arguments() const noexcept;

		void setApplicationName(std::string p_name);
		[[nodiscard]] const std::string &applicationName() const noexcept;

		[[nodiscard]] std::string usage() const;

	private:
		struct Entry
		{
			std::string longName;
			std::string shortName;
			std::string key;
			Option option;
			bool hasDefault = false;
			spk::JSON::Value defaultValue;
		};

		std::vector<Entry> _entries;
		std::map<std::string, std::size_t, std::less<>> _byName;
		spk::JSON::Value _arguments = spk::JSON::Value::object();
		std::string _applicationName;
		bool _applicationNameExplicit = false;

		void _registerEntry(std::string_view p_longName, std::string_view p_shortName, const Option &p_option, bool p_hasDefault, spk::JSON::Value p_default);
		[[nodiscard]] const Entry *_lookup(std::string_view p_name) const;
		[[nodiscard]] spk::JSON::Value _readValues(
			const Entry &p_entry, int p_argc, const char *const *p_argv, int &p_index) const;
		[[nodiscard]] static spk::JSON::Value _convert(
			const Option &p_option, const std::string &p_token, const std::string &p_name);
	};
}
