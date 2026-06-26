#pragma once

#include <vector>

#include "structures/container/spk_uuid.hpp"
#include "structures/game_engine/spk_component_store.hpp"

namespace spk
{
	class Component;

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

		template <typename TComponent>
		[[nodiscard]] const std::vector<spk::Component *> &components() const
		{
			return spk::ComponentStore::instance().components<TComponent>(_engineId);
		}
	};
}
