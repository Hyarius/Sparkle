#pragma once

#include <algorithm>
#include <vector>

#include "structures/game_engine/spk_component.hpp"

namespace spk
{
	// Non-owning index of components that all share the same concrete type.
	// Components are owned by their entity; the container only references them.
	// The processable subset is recomputed by refreshProcessable().
	class ComponentContainer
	{
	private:
		std::vector<spk::Component *> _components;
		std::vector<spk::Component *> _processableComponents;

	public:
		ComponentContainer() = default;

		void add(spk::Component *p_component)
		{
			if (p_component == nullptr ||
				std::find(_components.begin(), _components.end(), p_component) != _components.end())
			{
				return;
			}

			_components.push_back(p_component);
		}

		void remove(spk::Component *p_component)
		{
			_components.erase(std::remove(_components.begin(), _components.end(), p_component), _components.end());
			_processableComponents.erase(
				std::remove(_processableComponents.begin(), _processableComponents.end(), p_component),
				_processableComponents.end());
		}

		void refreshProcessable()
		{
			_processableComponents.clear();

			for (spk::Component *component : _components)
			{
				if (component != nullptr && component->isProcessable() == true)
				{
					_processableComponents.push_back(component);
				}
			}
		}

		[[nodiscard]] bool empty() const
		{
			return _components.empty();
		}

		[[nodiscard]] const std::vector<spk::Component *> &components() const
		{
			return _components;
		}

		[[nodiscard]] const std::vector<spk::Component *> &processableComponents() const
		{
			return _processableComponents;
		}
	};
}
