#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "spk_window.hpp"

namespace spk
{
	class WindowRegistry
	{
	public:
		using WindowID = std::string;

	private:
		struct Entry
		{
			std::shared_ptr<spk::Window> window;
			spk::Window::ClosureContract closureContract;
		};

		std::unordered_map<WindowID, Entry> _windows;

		void _removeWindow(spk::Window* p_window);

	public:
		std::shared_ptr<spk::Window> createWindow(const WindowID& p_id, spk::WindowHost::Configuration p_configuration);
		void requestWindowClosing(const WindowID& p_id);
	};
}
