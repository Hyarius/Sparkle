#pragma once

#include <atomic>

#include "spk_duration.hpp"
#include "spk_window_registry.hpp"

namespace spk
{
	class Application
	{
	public:
		using WindowID = spk::WindowRegistry::WindowID;

		struct Configuration
		{
			spk::Duration renderInterval = 16_ms;
			spk::Duration updateInterval = 16_ms;
			spk::Duration eventPollingInterval = 1_ms;
			bool stopWhenNoWindows = true;
		};

	private:
		Configuration _configuration;
		spk::WindowRegistry _windowRegistry;
		std::atomic<bool> _isRunning = false;
		std::atomic<bool> _stopRequested = false;

	private:
		static void _sleep(const spk::Duration& p_duration);

		void _runRenderLoop();
		void _runUpdateLoop();
		void _runEventLoop();

	public:
		Application();
		explicit Application(Configuration p_configuration);

		spk::Window& createWindow(const WindowID& p_id, spk::WindowHost::Configuration p_configuration);

		[[nodiscard]] spk::Window& window(const WindowID& p_id);
		[[nodiscard]] const spk::Window& window(const WindowID& p_id) const;
		[[nodiscard]] bool containsWindow(const WindowID& p_id) const;
		[[nodiscard]] bool isRunning() const;

		void requestWindowClosing(const WindowID& p_id);
		void stop();
		void run();
	};
}
