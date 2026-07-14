#pragma once

#include "core/json.hpp"

#include <filesystem>
#include <string_view>
#include <utility>

namespace pgtest
{
	// A parsed fixture document and a reader over it. The reader borrows the value, so the
	// document has to outlive every parse call made through it.
	class Document
	{
	private:
		spk::JSON::Value _value;
		std::filesystem::path _file;

	public:
		explicit Document(std::string_view p_content, std::filesystem::path p_file = "fixture.json") :
			_value(spk::JSON::Value::fromString(p_content)),
			_file(std::move(p_file))
		{
		}

		[[nodiscard]] pg::JsonReader reader() const
		{
			return pg::JsonReader(_value, _file);
		}

		[[nodiscard]] spk::JSON::Value &value() noexcept
		{
			return _value;
		}
	};
}
