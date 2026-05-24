#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "opengl/spk_opengl_runtime.hpp"
#include "application/spk_platform_runtime.hpp"
#include "window/spk_window.hpp"
#include "window/spk_window_handle.hpp"

namespace spk
{
	class Application;

	class WindowRegistry
	{
	public:
		using WindowID = std::string;

	private:
		mutable std::mutex _mutex;

		struct Entry
		{
			std::shared_ptr<spk::Window> window;
		};

		std::unordered_map<WindowID, Entry> _windows;

		[[nodiscard]] std::vector<std::weak_ptr<spk::Window>> windows() const;

		friend class spk::Application;

	public:
		spk::WindowHandle createWindow(const WindowID& p_id, std::shared_ptr<PlatformRuntime> p_platformRuntime, std::shared_ptr<GPUPlatformRuntime> p_gpuPlatformRuntime, spk::Window::Configuration p_configuration);
		[[nodiscard]] spk::WindowHandle window(const WindowID& p_id) const;
		[[nodiscard]] bool contains(const WindowID& p_id) const;
		[[nodiscard]] size_t size() const;
		void removeClosedWindows();
		void requestWindowClosing(const WindowID& p_id);
	};
}
