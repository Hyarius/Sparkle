#pragma once

#include "core/registries.hpp"
#include "tools/core/json_writer.hpp"

#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace pg::tools
{
	class ToolServices
	{
	private:
		std::filesystem::path _dataDirectory;
		std::unique_ptr<pg::Registries> _registries;
		// Editor previews can retain pointers into the registry they were built from.
		// Keep prior immutable snapshots alive across a hot reload; pages rebuild from
		// the latest snapshot when they are reloaded or saved.
		std::vector<std::unique_ptr<pg::Registries>> _retiredRegistries;
		JsonWriter _writer;
		std::function<void(const std::string &)> _statusSink;

	public:
		explicit ToolServices(std::filesystem::path p_dataDirectory) :
			_dataDirectory(std::move(p_dataDirectory)),
			_registries(std::make_unique<pg::Registries>())
		{
		}

		void load()
		{
			auto loaded = std::make_unique<pg::Registries>();
			loaded->loadAll(_dataDirectory);
			if (_registries != nullptr)
			{
				_retiredRegistries.push_back(std::move(_registries));
			}
			_registries = std::move(loaded);
		}

		[[nodiscard]] const std::filesystem::path &dataDirectory() const noexcept
		{
			return _dataDirectory;
		}

		[[nodiscard]] pg::Registries &registries() noexcept
		{
			return *_registries;
		}

		[[nodiscard]] const pg::Registries &registries() const noexcept
		{
			return *_registries;
		}

		[[nodiscard]] JsonWriter &writer() noexcept
		{
			return _writer;
		}

		void setStatusSink(std::function<void(const std::string &)> p_sink)
		{
			_statusSink = std::move(p_sink);
		}

		void report(std::string p_message) const
		{
			if (_statusSink)
			{
				_statusSink(p_message);
			}
		}
	};
}
