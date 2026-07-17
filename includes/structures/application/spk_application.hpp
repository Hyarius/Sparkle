#pragma once

#include <atomic>
#include <exception>
#include <memory>
#include <mutex>
#include <stop_token>
#include <thread>

#include "structures/system/device/runtime/spk_platform_runtime.hpp"
#include "structures/system/device/window/spk_window_handle.hpp"
#include "structures/system/device/window/spk_window_registry.hpp"
#include "structures/system/time/spk_duration.hpp"

namespace spk
{
	class Application
	{
	public:
		using WindowID = spk::WindowRegistry::WindowID;

		struct Configuration
		{
			std::shared_ptr<PlatformRuntime> platformRuntime = nullptr;
			spk::Duration renderInterval = 16_ms;
			spk::Duration updateInterval = 16_ms;
			spk::Duration eventPollingInterval = 1_ms;
		};

	private:
		Configuration _configuration;
		std::shared_ptr<PlatformRuntime> _platformRuntime;
		spk::WindowRegistry _windowRegistry;
		std::thread::id _ownerThreadID;
		std::stop_source _stopSource;
		std::atomic<bool> _isRunning = false;
		std::atomic<int> _exitCode = 0;
		std::mutex _failureMutex;
		std::exception_ptr _failure = nullptr;

	private:
		static std::shared_ptr<PlatformRuntime> _createDefaultPlatformRuntime();
		void _bindOrValidateOwnerThread(const char *p_operation);
		void _recordFailure(std::exception_ptr p_failure);
		void _rethrowFailureIfAny();
		void _requestAllWindowsClosing();

		void _runRenderLoop(std::stop_token p_stopToken);
		void _runUpdateLoop(std::stop_token p_stopToken);
		void _runEventLoop(std::stop_token p_stopToken);

	public:
		Application();
		explicit Application(Configuration p_configuration);

		spk::WindowHandle createWindow(const WindowID &p_id, spk::Window::Configuration p_configuration);

		[[nodiscard]] spk::WindowHandle window(const WindowID &p_id);
		[[nodiscard]] spk::WindowHandle window(const WindowID &p_id) const;
		[[nodiscard]] bool containsWindow(const WindowID &p_id) const;
		[[nodiscard]] bool isRunning() const;

		void requestWindowClosing(const WindowID &p_id);
		void quit(int p_exitCode);
		void stop();
		int run();
	};
}
