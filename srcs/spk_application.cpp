#include "spk_application.hpp"

#include <chrono>
#include <stdexcept>
#include <thread>

#include "spk_time_utils.hpp"

namespace spk
{
	void Application::_sleep(const spk::Duration& p_duration)
	{
		if (p_duration.nanoseconds() <= 0)
		{
			std::this_thread::yield();
			return;
		}

		std::this_thread::sleep_for(std::chrono::nanoseconds(p_duration.nanoseconds()));
	}

	void Application::_runRenderLoop()
	{
		while (_stopRequested.load() == false)
		{
			std::vector<std::shared_ptr<spk::Window>> windows = _windowRegistry.windows();

			if (windows.empty() == true)
			{
				if (_configuration.stopWhenNoWindows == true)
				{
					_stopRequested.store(true);
					return;
				}

				_sleep(_configuration.renderInterval);
				continue;
			}

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

			_sleep(_configuration.renderInterval);
		}
	}

	void Application::_runUpdateLoop()
	{
		while (_stopRequested.load() == false)
		{
			std::vector<std::shared_ptr<spk::Window>> windows = _windowRegistry.windows();

			if (windows.empty() == true)
			{
				if (_configuration.stopWhenNoWindows == true)
				{
					_stopRequested.store(true);
					return;
				}

				_sleep(_configuration.updateInterval);
				continue;
			}

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

			_sleep(_configuration.updateInterval);
		}
	}

	void Application::_runEventLoop()
	{
		while (_stopRequested.load() == false)
		{
			std::vector<std::shared_ptr<spk::Window>> windows = _windowRegistry.windows();

			if (windows.empty() == true)
			{
				if (_configuration.stopWhenNoWindows == true)
				{
					_stopRequested.store(true);
					return;
				}

				_sleep(_configuration.eventPollingInterval);
				continue;
			}

			for (const auto& window : windows)
			{
				if (_stopRequested.load() == true)
				{
					return;
				}

				if (window != nullptr)
				{
					window->pollEvents();
				}
			}

			_sleep(_configuration.eventPollingInterval);
		}
	}

	Application::Application() = default;

	Application::Application(Configuration p_configuration) :
		_configuration(std::move(p_configuration))
	{
	}

	spk::Window& Application::createWindow(const WindowID& p_id, spk::WindowHost::Configuration p_configuration)
	{
		return *_windowRegistry.createWindow(p_id, std::move(p_configuration));
	}

	spk::Window& Application::window(const WindowID& p_id)
	{
		return *_windowRegistry.window(p_id);
	}

	const spk::Window& Application::window(const WindowID& p_id) const
	{
		return *_windowRegistry.window(p_id);
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
		bool expectedValue = false;
		if (_isRunning.compare_exchange_strong(expectedValue, true) == false)
		{
			throw std::runtime_error("Application::run : Application is already running");
		}

		_stopRequested.store(false);

		try
		{
			std::jthread renderThread([this]()
			{
				_runRenderLoop();
			});

			std::jthread updateThread([this]()
			{
				_runUpdateLoop();
			});

			std::jthread eventThread([this]()
			{
				_runEventLoop();
			});

			renderThread.join();
			updateThread.join();
			eventThread.join();
		}
		catch (...)
		{
			_stopRequested.store(true);
			_isRunning.store(false);
			throw;
		}

		_isRunning.store(false);
	}
}
