#include "spk_window_registry.hpp"

namespace spk
{
	void WindowRegistry::_removeWindow(spk::Window* p_window)
	{
		if (p_window == nullptr)
		{
			return;
		}

		for (auto iterator = _windows.begin(); iterator != _windows.end(); ++iterator)
		{
			if (iterator->second.window.get() == p_window)
			{
				_windows.erase(iterator);
				return;
			}
		}
	}

	std::shared_ptr<spk::Window> WindowRegistry::createWindow(const WindowID& p_id, spk::WindowHost::Configuration p_configuration)
	{
		if (_windows.contains(p_id) == true)
		{
			throw std::runtime_error("WindowRegistry::" + std::string(__FUNCTION__) + " : Window ID [" + p_id + "] already exist");
		}

		std::shared_ptr<Window> newWindow = std::make_shared<Window>(std::move(p_configuration));
		Window::ClosureContract newWindowClosureContract = newWindow->subscribeToClosure(
			[this](spk::Window* p_window)
			{
				_removeWindow(p_window);
			});

		_windows[p_id] = {
			.window = std::move(newWindow),
			.closureContract = std::move(newWindowClosureContract)
		};

		return _windows[p_id].window;
	}

	void WindowRegistry::requestWindowClosing(const WindowID& p_id)
	{
		if (_windows.contains(p_id) == false)
		{
			throw std::runtime_error("WindowRegistry::" + std::string(__FUNCTION__) + " : Window ID [" + p_id + "] doesn't exist while closing.");
		}

		_windows[p_id].window->requestClosure();
	}
}
