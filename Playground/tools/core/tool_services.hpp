#pragma once

#include "tools/core/json_writer.hpp"

#include <filesystem>
#include <functional>
#include <string>
#include <utility>

namespace pg::tools
{
	class ToolServices
	{
	private:
		std::filesystem::path _dataDirectory;
		JsonWriter _writer;
		std::function<void(const std::string &)> _statusSink;

	public:
		explicit ToolServices(std::filesystem::path p_dataDirectory) :
			_dataDirectory(std::move(p_dataDirectory))
		{
		}

		[[nodiscard]] const std::filesystem::path &dataDirectory() const noexcept { return _dataDirectory; }
		[[nodiscard]] JsonWriter &writer() noexcept { return _writer; }

		void setStatusSink(std::function<void(const std::string &)> p_sink)
		{
			_statusSink = std::move(p_sink);
		}

		void report(std::string p_message) const
		{
			if (_statusSink)
				_statusSink(p_message);
		}
	};
}
