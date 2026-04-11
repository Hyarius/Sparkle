#pragma once

#include <atomic>
#include <exception>
#include <memory>
#include <mutex>
#include <thread>

#include "spk_duration.hpp"
#include "spk_gpu_platform_runtime.hpp"
#include "spk_platform_runtime.hpp"
#include "spk_window_registry.hpp"

namespace spk
{
	class Application
	{
	public:
		using WindowID = spk::WindowRegistry::WindowID;

		struct Configuration
		{
			std::shared_ptr<IPlatformRuntime> platformRuntime = nullptr;
			std::shared_ptr<IGPUPlatformRuntime> gpuPlatformRuntime = nullptr;
			spk::Duration renderInterval = 16_ms;
			spk::Duration updateInterval = 16_ms;
			spk::Duration eventPollingInterval = 1_ms;
			bool stopWhenNoWindows = true;
		};

	private:
		Configuration _configuration;
		std::shared_ptr<IPlatformRuntime> _platformRuntime;
		std::shared_ptr<IGPUPlatformRuntime> _gpuPlatformRuntime;
		spk::WindowRegistry _windowRegistry;
		std::thread::id _ownerThreadID;
		std::atomic<bool> _isRunning = false;
		std::atomic<bool> _stopRequested = false;
		std::mutex _failureMutex;
		std::exception_ptr _failure = nullptr;

	private:
		static std::shared_ptr<IPlatformRuntime> _createDefaultPlatformRuntime();
		static std::shared_ptr<IGPUPlatformRuntime> _createDefaultGPUPlatformRuntime();
		void _bindOrValidateOwnerThread(const char* p_operation);
		void _recordFailure(std::exception_ptr p_failure);
		void _rethrowFailureIfAny();

		void _runRenderLoop();
		void _runUpdateLoop();
		void _runEventLoop();

	public:
		Application();
		explicit Application(Configuration p_configuration);

		std::shared_ptr<spk::Window> createWindow(const WindowID& p_id, spk::Window::Configuration p_configuration);

		[[nodiscard]] std::shared_ptr<spk::Window> window(const WindowID& p_id);
		[[nodiscard]] std::shared_ptr<const spk::Window> window(const WindowID& p_id) const;
		[[nodiscard]] bool containsWindow(const WindowID& p_id) const;
		[[nodiscard]] bool isRunning() const;

		void requestWindowClosing(const WindowID& p_id);
		void stop();
		void run();
	};
}
