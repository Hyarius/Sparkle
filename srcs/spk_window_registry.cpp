#include "spk_window_registry.hpp"

namespace spk
{
	void WindowRegistry::_removeWindow(spk::Window* p_window)
	{
		if (p_window == nullptr)
		{
			return;
		}

		std::scoped_lock lock(_mutex);

		for (auto iterator = _windows.begin(); iterator != _windows.end(); ++iterator)
		{
			if (iterator->second.window.get() == p_window)
			{
				_windows.erase(iterator);
				return;
			}
		}
	}

	std::shared_ptr<spk::Window> WindowRegistry::createWindow(const WindowID& p_id, std::shared_ptr<IPlatformRuntime> p_platformRuntime, std::shared_ptr<IGPUPlatformRuntime> p_gpuPlatformRuntime, spk::Window::Configuration p_configuration)
	{
		{
			std::scoped_lock lock(_mutex);
			if (_windows.contains(p_id) == true)
			{
				throw std::runtime_error("WindowRegistry::" + std::string(__FUNCTION__) + " : Window ID [" + p_id + "] already exist");
			}
		}

		std::shared_ptr<Window> newWindow = std::make_shared<Window>(std::move(p_platformRuntime), std::move(p_gpuPlatformRuntime), std::move(p_configuration));
		Window::ClosureContract newWindowClosureContract = newWindow->subscribeToClosure(
			[this](spk::Window* p_window)
			{
				_removeWindow(p_window);
			});

		std::scoped_lock lock(_mutex);
		if (_windows.contains(p_id) == true)
		{
			throw std::runtime_error("WindowRegistry::" + std::string(__FUNCTION__) + " : Window ID [" + p_id + "] already exist");
		}

		auto [iterator, inserted] = _windows.emplace(
			p_id,
			Entry{
				.window = std::move(newWindow),
				.closureContract = std::move(newWindowClosureContract)
			});

		(void)inserted;
		return iterator->second.window;
	}

	std::shared_ptr<spk::Window> WindowRegistry::window(const WindowID& p_id) const
	{
		std::scoped_lock lock(_mutex);

		auto iterator = _windows.find(p_id);
		if (iterator == _windows.end())
		{
			throw std::runtime_error("WindowRegistry::" + std::string(__FUNCTION__) + " : Window ID [" + p_id + "] doesn't exist");
		}

		return iterator->second.window;
	}

	bool WindowRegistry::contains(const WindowID& p_id) const
	{
		std::scoped_lock lock(_mutex);
		return _windows.contains(p_id);
	}

	size_t WindowRegistry::size() const
	{
		std::scoped_lock lock(_mutex);
		return _windows.size();
	}

	std::vector<std::shared_ptr<spk::Window>> WindowRegistry::windows() const
	{
		std::scoped_lock lock(_mutex);

		std::vector<std::shared_ptr<spk::Window>> result;
		result.reserve(_windows.size());

		for (const auto& [id, entry] : _windows)
		{
			(void)id;
			result.push_back(entry.window);
		}

		return result;
	}

	void WindowRegistry::requestWindowClosing(const WindowID& p_id)
	{
		std::shared_ptr<spk::Window> windowToClose;

		{
			std::scoped_lock lock(_mutex);
			auto iterator = _windows.find(p_id);
			if (iterator == _windows.end())
			{
				throw std::runtime_error("WindowRegistry::" + std::string(__FUNCTION__) + " : Window ID [" + p_id + "] doesn't exist while closing.");
			}

			windowToClose = iterator->second.window;
		}

		windowToClose->requestClosure();
	}
}
