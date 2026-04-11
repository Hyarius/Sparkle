#include "spk_application.hpp"

#include <chrono>
#include <stdexcept>
#include <thread>

#include "spk_chronometer.hpp"
#include "spk_time_utils.hpp"

namespace spk
{
	std::shared_ptr<IPlatformRuntime> Application::_createDefaultPlatformRuntime()
	{
		throw std::runtime_error("No default spk::IPlatformRuntime is implemented for this platform yet");
	}

	std::shared_ptr<IGPUPlatformRuntime> Application::_createDefaultGPUPlatformRuntime()
	{
		throw std::runtime_error("No default spk::IGPUPlatformRuntime is implemented for this platform yet");
	}

	void Application::_recordFailure(std::exception_ptr p_failure)
	{
		std::scoped_lock lock(_failureMutex);

		if (_failure == nullptr)
		{
			_failure = std::move(p_failure);
		}
	}

	void Application::_rethrowFailureIfAny()
	{
		std::exception_ptr failure = nullptr;

		{
			std::scoped_lock lock(_failureMutex);
			failure = _failure;
			_failure = nullptr;
		}

		if (failure != nullptr)
		{
			std::rethrow_exception(failure);
		}
	}

	void Application::_runRenderLoop()
	{

		while (_stopRequested.load() == false)
		{
			spk::Chronometer chronometer;
			chronometer.start();

			std::vector<std::shared_ptr<spk::Window>> windows = _windowRegistry.windows();

			if (windows.empty() == true)
			{
				if (_configuration.stopWhenNoWindows == true)
				{
					_stopRequested.store(true);
					return;
				}
			}
			else
			{
				for (const auto& window : windows)
				{
					if (_stopRequested.load() == true)
					{
						return;
					}
					
					if (window != nullptr)
					{
						window->render();
					}
				}
			}

			const spk::Duration elapsedTime = chronometer.elapsedTime();
			const spk::Duration remainingTime = _configuration.renderInterval - elapsedTime;

			if (remainingTime > 0_ns)
			{
				spk::TimeUtils::sleepFor(remainingTime);
			}
		}
	}

	void Application::_runUpdateLoop()
	{
		while (_stopRequested.load() == false)
		{
			spk::Chronometer chronometer;
			chronometer.start();

			std::vector<std::shared_ptr<spk::Window>> windows = _windowRegistry.windows();

			if (windows.empty() == true)
			{
				if (_configuration.stopWhenNoWindows == true)
				{
					_stopRequested.store(true);
					return;
				}
			}
			else
			{
				for (const auto& window : windows)
				{
					if (_stopRequested.load() == true)
					{
						return;
					}

					if (window != nullptr)
					{
						window->update();
					}
				}	
			}

			const spk::Duration elapsedTime = chronometer.elapsedTime();
			const spk::Duration remainingTime = _configuration.updateInterval - elapsedTime;

			if (remainingTime > 0_ns)
			{
				spk::TimeUtils::sleepFor(remainingTime);
			}
		}
	}

	void Application::_runEventLoop()
	{
		while (_stopRequested.load() == false)
		{
			if (_windowRegistry.size() == 0)
			{
				if (_configuration.stopWhenNoWindows == true)
				{
					_stopRequested.store(true);
					return;
				}

				spk::TimeUtils::sleepFor(_configuration.eventPollingInterval);
				continue;
			}

			_platformRuntime->pollEvents();

			spk::TimeUtils::sleepFor(_configuration.eventPollingInterval);
		}
	}

	Application::Application() = default;

	Application::Application(Configuration p_configuration) :
		_configuration(std::move(p_configuration)),
		_platformRuntime(_configuration.platformRuntime),
		_gpuPlatformRuntime(_configuration.gpuPlatformRuntime)
	{
	}

	std::shared_ptr<spk::Window> Application::createWindow(const WindowID& p_id, spk::Window::Configuration p_configuration)
	{
		if (_platformRuntime == nullptr)
		{
			_platformRuntime = _createDefaultPlatformRuntime();
		}

		if (_gpuPlatformRuntime == nullptr)
		{
			_gpuPlatformRuntime = _createDefaultGPUPlatformRuntime();
		}

		return _windowRegistry.createWindow(p_id, _platformRuntime, _gpuPlatformRuntime, std::move(p_configuration));
	}

	std::shared_ptr<spk::Window> Application::window(const WindowID& p_id)
	{
		return _windowRegistry.window(p_id);
	}

	std::shared_ptr<const spk::Window> Application::window(const WindowID& p_id) const
	{
		return _windowRegistry.window(p_id);
	}

	bool Application::containsWindow(const WindowID& p_id) const
	{
		return _windowRegistry.contains(p_id);
	}

	bool Application::isRunning() const
	{
		return _isRunning.load();
	}

	void Application::requestWindowClosing(const WindowID& p_id)
	{
		_windowRegistry.requestWindowClosing(p_id);
	}

	void Application::stop()
	{
		_stopRequested.store(true);
	}

	void Application::run()
	{
		if (_platformRuntime == nullptr && _windowRegistry.size() != 0)
		{
			_platformRuntime = _createDefaultPlatformRuntime();
		}

		if (_gpuPlatformRuntime == nullptr && _windowRegistry.size() != 0)
		{
			_gpuPlatformRuntime = _createDefaultGPUPlatformRuntime();
		}

		bool expectedValue = false;
		if (_isRunning.compare_exchange_strong(expectedValue, true) == false)
		{
			throw std::runtime_error("Application::run : Application is already running");
		}

		_stopRequested.store(false);
		_rethrowFailureIfAny();

		std::jthread renderThread([this]()
		{
			try
			{
				_runRenderLoop();
			}
			catch (...)
			{
				_recordFailure(std::current_exception());
				_stopRequested.store(true);
			}
		});

		std::jthread updateThread([this]()
		{
			try
			{
				_runUpdateLoop();
			}
			catch (...)
			{
				_recordFailure(std::current_exception());
				_stopRequested.store(true);
			}
		});

		try
		{
			_runEventLoop();
		}
		catch (...)
		{
			_recordFailure(std::current_exception());
			_stopRequested.store(true);
		}

		renderThread.join();
		updateThread.join();

		_isRunning.store(false);
		_rethrowFailureIfAny();
	}
}
