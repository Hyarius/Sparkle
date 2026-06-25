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

	// Process-wide component store, shared by every GameEngine.
	//
	// Components are bucketed first by concrete type (typeid) and then by the UUID of
	// the engine they are registered with. An engine processes a phase by asking for
	// its own slice, e.g. components(typeid(Movement), engineId). Entities mutate the
	// store as they gain/lose components or change engine membership; activation never
	// touches it (see ComponentContainer).
	//
	// It is a singleton because engine membership is keyed by value (UUID): an entity
	// holds no pointer to any engine or registry, so nothing dangles, and re-parenting
	// a sub-tree between engines is just a bucket move.
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

		// Drop every bucket belonging to p_engineId (called when an engine is destroyed).
		void clearEngine(const spk::UUID &p_engineId);

		[[nodiscard]] const std::vector<spk::Component *> &components(std::type_index p_type, const spk::UUID &p_engineId) const;

		template <typename TComponent>
		[[nodiscard]] const std::vector<spk::Component *> &components(const spk::UUID &p_engineId) const
		{
			return components(std::type_index(typeid(TComponent)), p_engineId);
		}
	};
}
