#pragma once

#include <typeindex>
#include <typeinfo>
#include <unordered_map>

#include "structures/game_engine/spk_component_container.hpp"

namespace spk
{
	// Index of every component currently owned by the engine, bucketed by concrete type.
	//
	// Registration is INCREMENTAL: components are added/removed as entities attach/detach
	// (no per-frame rebuild). Buckets are keyed by the component's runtime type
	// (typeid(*component)). The "processable" subset is recomputed only when marked dirty.
	class ComponentRegistry
	{
	private:
		std::unordered_map<std::type_index, spk::ComponentContainer> _containers;
		bool _processableDirty = true;

		[[nodiscard]] static std::type_index _key(const spk::Component *p_component)
		{
			return std::type_index(typeid(*p_component));
		}

	public:
		ComponentRegistry() = default;
		~ComponentRegistry() = default;

		ComponentRegistry(const ComponentRegistry &) = delete;
		ComponentRegistry &operator=(const ComponentRegistry &) = delete;

		ComponentRegistry(ComponentRegistry &&) noexcept = default;
		ComponentRegistry &operator=(ComponentRegistry &&) noexcept = default;

		void add(spk::Component *p_component)
		{
			if (p_component == nullptr)
			{
				return;
			}

			_containers[_key(p_component)].add(p_component);
			_processableDirty = true;
		}

		void remove(spk::Component *p_component)
		{
			if (p_component == nullptr)
			{
				return;
			}

			auto iterator = _containers.find(_key(p_component));

			if (iterator != _containers.end())
			{
				iterator->second.remove(p_component);
				_processableDirty = true;
			}
		}

		void clear()
		{
			_containers.clear();
			_processableDirty = true;
		}

		void invalidateProcessable()
		{
			_processableDirty = true;
		}

		void refreshProcessable()
		{
			for (auto &[typeIndex, container] : _containers)
			{
				(void)typeIndex;
				container.refreshProcessable();
			}

			_processableDirty = false;
		}

		void refreshProcessableIfDirty()
		{
			if (_processableDirty == true)
			{
				refreshProcessable();
			}
		}

		// Returns (creating if needed) the container for component type TComponent.
		template <typename TComponent>
		[[nodiscard]] spk::ComponentContainer &container()
		{
			return _containers[std::type_index(typeid(TComponent))];
		}
	};
}
