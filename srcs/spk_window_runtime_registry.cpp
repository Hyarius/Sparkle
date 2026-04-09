#include "spk_window_runtime.hpp"

namespace spk
{
	void WindowRuntimeRegistry::_removeWindowRuntime(spk::WindowRuntime* p_windowRuntime)
	{
		if (p_windowRuntime == nullptr)
		{
			return;
		}

		for (auto iterator = _windowRuntimes.begin(); iterator != _windowRuntimes.end(); ++iterator)
		{
			if (iterator->second.runtime.get() == p_windowRuntime)
			{
				_windowRuntimes.erase(iterator);
				return;
			}
		}
	}

	std::shared_ptr<spk::WindowRuntime> WindowRuntimeRegistry::createWindowRuntime(const WindowID& p_id, spk::Window::Configuration p_configuration)
	{
		if (_windowRuntimes.contains(p_id) == true)
		{
			throw std::runtime_error("WindowRuntimeRegistry::" + std::string(__FUNCTION__) + " : Window ID [" + p_id + "] already exist");
		}

		std::shared_ptr<WindowRuntime> newWindowRuntime = std::make_shared<WindowRuntime>(std::move(p_configuration));
		WindowRuntime::ClosureContract newWindowClosureContract = newWindowRuntime->subscribeToClosure(
			[this](spk::WindowRuntime* p_windowRuntime)
			{
				_removeWindowRuntime(p_windowRuntime);
			});

		_windowRuntimes[p_id] = {
			.runtime = std::move(newWindowRuntime),
			.closureContract = std::move(newWindowClosureContract)
		};

		return _windowRuntimes[p_id].runtime;
	}

	void WindowRuntimeRegistry::requestWindowClosing(const WindowID& p_id)
	{
		if (_windowRuntimes.contains(p_id) == false)
		{
			throw std::runtime_error("WindowRuntimeRegistry::" + std::string(__FUNCTION__) + " : Window ID [" + p_id + "] doesn't exist while closing.");
		}

		_windowRuntimes[p_id].runtime->requestClosure();
	}
}
