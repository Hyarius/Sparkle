#pragma once

#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <vector>

#include "structures/container/spk_uuid.hpp"
#include "structures/game_engine/spk_component_container.hpp"

namespace spk
{
	class Component;

	class ComponentStore
	{
	private:
		std::unordered_map<std::type_index, spk::ComponentContainer> _containers;

		ComponentStore() = default;

	public:
		ComponentStore(const ComponentStore &) = delete;
		ComponentStore &operator=(const ComponentStore &) = delete;
		ComponentStore(ComponentStore &&) = delete;
		ComponentStore &operator=(ComponentStore &&) = delete;

		[[nodiscard]] static ComponentStore &instance();

		void add(std::type_index p_type, const spk::UUID &p_engineId, spk::Component *p_component);
		void remove(std::type_index p_type, const spk::UUID &p_engineId, spk::Component *p_component);
		void move(std::type_index p_type, const spk::UUID &p_from, const spk::UUID &p_to, spk::Component *p_component);

		void clearEngine(const spk::UUID &p_engineId);

		[[nodiscard]] const std::vector<spk::Component *> &components(std::type_index p_type, const spk::UUID &p_engineId) const;

		template <typename TComponent>
		[[nodiscard]] const std::vector<spk::Component *> &components(const spk::UUID &p_engineId) const
		{
			return components(std::type_index(typeid(TComponent)), p_engineId);
		}
	};
}
