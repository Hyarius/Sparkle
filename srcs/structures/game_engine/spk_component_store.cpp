#include "structures/game_engine/spk_component_store.hpp"

namespace spk
{
	ComponentStore &ComponentStore::instance()
	{
		static ComponentStore instance;
		return instance;
	}

	void ComponentStore::add(std::type_index p_type, const spk::UUID &p_engineId, spk::Component *p_component)
	{
		if (p_component == nullptr || p_engineId.isNull() == true)
		{
			return;
		}

		_containers[p_type].add(p_engineId, p_component);
	}

	void ComponentStore::remove(std::type_index p_type, const spk::UUID &p_engineId, spk::Component *p_component)
	{
		auto iterator = _containers.find(p_type);

		if (iterator == _containers.end())
		{
			return;
		}

		iterator->second.remove(p_engineId, p_component);

		if (iterator->second.empty() == true)
		{
			_containers.erase(iterator);
		}
	}

	void ComponentStore::move(std::type_index p_type, const spk::UUID &p_from, const spk::UUID &p_to, spk::Component *p_component)
	{
		if (p_from == p_to)
		{
			return;
		}

		remove(p_type, p_from, p_component);
		add(p_type, p_to, p_component);
	}

	void ComponentStore::clearEngine(const spk::UUID &p_engineId)
	{
		for (auto iterator = _containers.begin(); iterator != _containers.end();)
		{
			iterator->second.clearEngine(p_engineId);

			if (iterator->second.empty() == true)
			{
				iterator = _containers.erase(iterator);
			}
			else
			{
				++iterator;
			}
		}
	}

	const std::vector<spk::Component *> &ComponentStore::components(std::type_index p_type, const spk::UUID &p_engineId) const
	{
		static const std::vector<spk::Component *> empty;

		auto iterator = _containers.find(p_type);

		return (iterator == _containers.end() ? empty : iterator->second.components(p_engineId));
	}
}
