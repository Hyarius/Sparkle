#include "structures/application/spk_application.hpp"

#include <chrono>
#include <stdexcept>
#include <thread>

#include "structures/system/time/spk_chronometer.hpp"
#include "utils/spk_time_utils.hpp"

namespace spk
{
	std::shared_ptr<PlatformRuntime> Application::_createDefaultPlatformRuntime()
	{
		return std::make_shared<spk::PlatformRuntime>();
	}

	void Application::_bindOrValidateOwnerThread(const char *p_operation)
	{
		const std::thread::id currentThreadID = std::this_thread::get_id();

		if (_ownerThreadID != currentThreadID)
		{
			throw std::runtime_error(
				"Application::" + std::string(p_operation) +
				" must be called from the application construction thread");
		}
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

	void Application::_requestAllWindowsClosing()
	{
		for (const std::weak_ptr<spk::Window> &windowHandle : _windowRegistry._windowVector())
		{
			if (std::shared_ptr<spk::Window> window = windowHandle.lock(); window != nullptr)
			{
				window->requestClosure();
			}
		}
	}

	void Application::_runRenderLoop(std::stop_token p_stopToken)
	{
		while (p_stopToken.stop_requested() == false && _windowRegistry.size() != 0)
		{
			spk::Chronometer chronometer;
			chronometer.start();

			std::vector<std::weak_ptr<spk::Window>> windows = _windowRegistry._windowVector();

			if (windows.empty() == false)
			{
				for (const std::weak_ptr<spk::Window> &windowHandle : windows)
				{
					std::shared_ptr<spk::Window> window = windowHandle.lock();
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

	void Application::_runUpdateLoop(std::stop_token p_stopToken)
	{
		while (p_stopToken.stop_requested() == false && _windowRegistry.size() != 0)
		{
			spk::Chronometer chronometer;
			chronometer.start();

			std::vector<std::weak_ptr<spk::Window>> windows = _windowRegistry._windowVector();

			if (windows.empty() == false)
			{
				for (const std::weak_ptr<spk::Window> &windowHandle : windows)
				{
					std::shared_ptr<spk::Window> window = windowHandle.lock();
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

	void Application::_runEventLoop(std::stop_token p_stopToken)
	{
		while (p_stopToken.stop_requested() == false && _windowRegistry.size() != 0)
		{
			_platformRuntime->pollEvents();

			std::vector<std::weak_ptr<spk::Window>> windows = _windowRegistry._windowVector();
			for (const std::weak_ptr<spk::Window> &windowHandle : windows)
			{
				std::shared_ptr<spk::Window> window = windowHandle.lock();
				if (window != nullptr)
				{
					window->executePendingPlatformActions();
				}
			}

			_windowRegistry.removeClosedWindows();

			spk::TimeUtils::sleepFor(_configuration.eventPollingInterval);
		}
	}

	Application::Application() :
		_ownerThreadID(std::this_thread::get_id())
	{
	}

	Application::Application(Configuration p_configuration) :
		_configuration(std::move(p_configuration)),
		_platformRuntime(_configuration.platformRuntime),
		_ownerThreadID(std::this_thread::get_id())
	{
	}

	spk::WindowHandle Application::createWindow(const WindowID &p_id, spk::Window::Configuration p_configuration)
	{
		_bindOrValidateOwnerThread(__FUNCTION__);

		if (_platformRuntime == nullptr)
		{
			_platformRuntime = _createDefaultPlatformRuntime();
		}

		return _windowRegistry.createWindow(p_id, _platformRuntime, std::move(p_configuration));
	}

	spk::WindowHandle Application::window(const WindowID &p_id)
	{
		return _windowRegistry.window(p_id);
	}

	spk::WindowHandle Application::window(const WindowID &p_id) const
	{
		return _windowRegistry.window(p_id);
	}

	bool Application::containsWindow(const WindowID &p_id) const
	{
		return _windowRegistry.contains(p_id);
	}

	bool Application::isRunning() const
	{
		return _isRunning.load();
	}

	void Application::requestWindowClosing(const WindowID &p_id)
	{
		_windowRegistry.requestWindowClosing(p_id);
	}

	void Application::quit(int p_exitCode)
	{
		_exitCode.store(p_exitCode);
		_requestAllWindowsClosing();
	}

	void Application::stop()
	{
		quit(0);
	}

	int Application::run()
	{
		_bindOrValidateOwnerThread(__FUNCTION__);

		if (_platformRuntime == nullptr && _windowRegistry.size() != 0)
		{
			_platformRuntime = _createDefaultPlatformRuntime();
		}

		bool expectedValue = false;
		if (_isRunning.compare_exchange_strong(expectedValue, true) == false)
		{
			throw std::runtime_error("Application::run : Application is already running");
		}

		_stopSource = std::stop_source();
		_exitCode.store(0);
		_rethrowFailureIfAny();
		const std::stop_token stopToken = _stopSource.get_token();

		std::jthread renderThread([this, stopToken](std::stop_token) {
			try
			{
				_runRenderLoop(stopToken);
			} catch (...)
			{
				_recordFailure(std::current_exception());
				_stopSource.request_stop();
				_requestAllWindowsClosing();
			}
		});

		std::jthread updateThread([this, stopToken](std::stop_token) {
			try
			{
				_runUpdateLoop(stopToken);
			} catch (...)
			{
				_recordFailure(std::current_exception());
				_stopSource.request_stop();
				_requestAllWindowsClosing();
			}
		});

		try
		{
			_runEventLoop(stopToken);
		} catch (...)
		{
			_recordFailure(std::current_exception());
			_stopSource.request_stop();
			_requestAllWindowsClosing();
		}

		renderThread.join();
		updateThread.join();

		_isRunning.store(false);
		_rethrowFailureIfAny();

		return _exitCode.load();
	}
}
