#pragma once

#include <atomic>
#include <exception>
#include <memory>
#include <mutex>
#include <thread>

#include "time/spk_duration.hpp"
#include "opengl/spk_opengl_runtime.hpp"
#include "application/spk_platform_runtime.hpp"
#include "window/spk_window_handle.hpp"
#include "window/spk_window_registry.hpp"

namespace spk
{
	class Application
	{
	public:
		using WindowID = spk::WindowRegistry::WindowID;

		struct Configuration
		{
			std::shared_ptr<PlatformRuntime> platformRuntime = nullptr;
			std::shared_ptr<GPUPlatformRuntime> gpuPlatformRuntime = nullptr;
			spk::Duration renderInterval = 16_ms;
			spk::Duration updateInterval = 16_ms;
			spk::Duration eventPollingInterval = 1_ms;
			bool stopWhenNoWindows = true;
		};

	private:
		Configuration _configuration;
		std::shared_ptr<PlatformRuntime> _platformRuntime;
		std::shared_ptr<GPUPlatformRuntime> _gpuPlatformRuntime;
		spk::WindowRegistry _windowRegistry;
		std::thread::id _ownerThreadID;
		std::atomic<bool> _isRunning = false;
		std::atomic<bool> _shutdownRequested = false;
		std::atomic<bool> _stopRequested = false;
		std::atomic<int> _exitCode = 0;
		std::mutex _failureMutex;
		std::exception_ptr _failure = nullptr;

	private:
		static std::shared_ptr<PlatformRuntime> _createDefaultPlatformRuntime();
		static std::shared_ptr<GPUPlatformRuntime> _createDefaultGPUPlatformRuntime();
		void _bindOrValidateOwnerThread(const char* p_operation);
		void _recordFailure(std::exception_ptr p_failure);
		void _rethrowFailureIfAny();

		void _runRenderLoop();
		void _runUpdateLoop();
		void _runEventLoop();

	public:
		Application();
		explicit Application(Configuration p_configuration);

		spk::WindowHandle createWindow(const WindowID& p_id, spk::Window::Configuration p_configuration);

		[[nodiscard]] spk::WindowHandle window(const WindowID& p_id);
		[[nodiscard]] spk::WindowHandle window(const WindowID& p_id) const;
		[[nodiscard]] bool containsWindow(const WindowID& p_id) const;
		[[nodiscard]] bool isRunning() const;

		void requestWindowClosing(const WindowID& p_id);
		void quit(int p_exitCode);
		void stop();
		int run();
	};
}
