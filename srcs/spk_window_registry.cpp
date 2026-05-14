#include "spk_window_registry.hpp"

namespace spk
{
	std::weak_ptr<spk::Window> WindowRegistry::createWindow(const WindowID& p_id, std::shared_ptr<IPlatformRuntime> p_platformRuntime, std::shared_ptr<IGPUPlatformRuntime> p_gpuPlatformRuntime, spk::Window::Configuration p_configuration)
	{
		{
			std::scoped_lock lock(_mutex);
			if (_windows.contains(p_id) == true)
			{
				throw std::runtime_error("WindowRegistry::" + std::string(__FUNCTION__) + " : Window ID [" + p_id + "] already exist");
			}
		}

		std::shared_ptr<Window> newWindow = std::make_shared<Window>(std::move(p_platformRuntime), std::move(p_gpuPlatformRuntime), std::move(p_configuration));

		std::scoped_lock lock(_mutex);
		if (_windows.contains(p_id) == true)
		{
			throw std::runtime_error("WindowRegistry::" + std::string(__FUNCTION__) + " : Window ID [" + p_id + "] already exist");
		}

		auto [iterator, inserted] = _windows.emplace(
			p_id,
			Entry{
				.window = std::move(newWindow)
			});

		(void)inserted;
		return std::weak_ptr<spk::Window>(iterator->second.window);
	}

	std::weak_ptr<spk::Window> WindowRegistry::window(const WindowID& p_id) const
	{
		std::scoped_lock lock(_mutex);

		auto iterator = _windows.find(p_id);
		if (iterator == _windows.end())
		{
			throw std::runtime_error("WindowRegistry::" + std::string(__FUNCTION__) + " : Window ID [" + p_id + "] doesn't exist");
		}

		return std::weak_ptr<spk::Window>(iterator->second.window);
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

	std::vector<std::weak_ptr<spk::Window>> WindowRegistry::windows() const
	{
		std::scoped_lock lock(_mutex);

		std::vector<std::weak_ptr<spk::Window>> result;
		result.reserve(_windows.size());

		for (const auto& [id, entry] : _windows)
		{
			(void)id;
			result.push_back(entry.window);
		}

		return result;
	}

	void WindowRegistry::removeClosedWindows()
	{
		std::scoped_lock lock(_mutex);

		for (auto iterator = _windows.begin(); iterator != _windows.end();)
		{
			if (iterator->second.window == nullptr ||
				iterator->second.window->isReadyForDisposal() == true)
			{
				iterator = _windows.erase(iterator);
			}
			else
			{
				++iterator;
			}
		}
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
