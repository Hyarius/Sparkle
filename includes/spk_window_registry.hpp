#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "spk_window.hpp"

namespace spk
{
	class WindowRegistry
	{
	public:
		using WindowID = std::string;

	private:
		mutable std::mutex _mutex;

		struct Entry
		{
			std::shared_ptr<spk::Window> window;
			spk::Window::ClosureContract closureContract;
		};

		std::unordered_map<WindowID, Entry> _windows;

		void _removeWindow(spk::Window* p_window);

	public:
		std::shared_ptr<spk::Window> createWindow(const WindowID& p_id, spk::WindowHost::Configuration p_configuration);
		[[nodiscard]] std::shared_ptr<spk::Window> window(const WindowID& p_id) const;
		[[nodiscard]] bool contains(const WindowID& p_id) const;
		[[nodiscard]] size_t size() const;
		[[nodiscard]] std::vector<std::shared_ptr<spk::Window>> windows() const;
		void requestWindowClosing(const WindowID& p_id);
	};
}
