#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "spk_gpu_platform_runtime.hpp"
#include "spk_platform_runtime.hpp"
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
		};

		std::unordered_map<WindowID, Entry> _windows;

	public:
		std::shared_ptr<spk::Window> createWindow(const WindowID& p_id, std::shared_ptr<IPlatformRuntime> p_platformRuntime, std::shared_ptr<IGPUPlatformRuntime> p_gpuPlatformRuntime, spk::Window::Configuration p_configuration);
		[[nodiscard]] std::shared_ptr<spk::Window> window(const WindowID& p_id) const;
		[[nodiscard]] bool contains(const WindowID& p_id) const;
		[[nodiscard]] size_t size() const;
		[[nodiscard]] std::vector<std::shared_ptr<spk::Window>> windows() const;
		void removeClosedWindows();
		void requestWindowClosing(const WindowID& p_id);
	};
}
