#pragma once

#include <vector>

#include "structures/container/spk_uuid.hpp"
#include "structures/game_engine/spk_component_store.hpp"

namespace spk
{
	class Component;

	// A per-engine VIEW into the shared ComponentStore. It carries the engine's UUID and
	// hands back that engine's slice of components for a given type. It owns nothing:
	// storage lives in ComponentStore, membership is decided by each entity's engine UUID.
	class ComponentRegistry
	{
	private:
		spk::UUID _engineId;

	public:
		ComponentRegistry() = default;
		explicit ComponentRegistry(const spk::UUID &p_engineId) :
			_engineId(p_engineId)
		{
		}

		[[nodiscard]] const spk::UUID &engineId() const
		{
			return _engineId;
		}

		void setEngineId(const spk::UUID &p_engineId)
		{
			_engineId = p_engineId;
		}

		// This engine's components of type TComponent (all of them, active or not;
		// callers skip inactive via Component::isProcessable()).
		template <typename TComponent>
		[[nodiscard]] const std::vector<spk::Component *> &components() const
		{
			return spk::ComponentStore::instance().components<TComponent>(_engineId);
		}
	};
}
