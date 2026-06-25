#pragma once

#include <algorithm>
#include <unordered_map>
#include <vector>

#include "structures/container/spk_uuid.hpp"

namespace spk
{
	class Component;

	// Non-owning index of components that all share the same concrete type, partitioned
	// by the UUID of the engine they are registered with. Components are owned by their
	// entity; the container only references them.
	//
	// Activation is NOT filtered here: every registered component of this type sits in
	// its engine's bucket regardless of activation state. Iterating code skips inactive
	// components inline via Component::isProcessable() (cheap, O(1) thanks to the
	// precomputed Entity global-activation flag).
	class ComponentContainer
	{
	private:
		std::unordered_map<spk::UUID, std::vector<spk::Component *>> _byEngine;

		[[nodiscard]] static const std::vector<spk::Component *> &_emptyBucket()
		{
			static const std::vector<spk::Component *> empty;
			return empty;
		}

	public:
		ComponentContainer() = default;

		void add(const spk::UUID &p_engineId, spk::Component *p_component)
		{
			if (p_component == nullptr)
			{
				return;
			}

			std::vector<spk::Component *> &bucket = _byEngine[p_engineId];

			if (std::find(bucket.begin(), bucket.end(), p_component) == bucket.end())
			{
				bucket.push_back(p_component);
			}
		}

		void remove(const spk::UUID &p_engineId, spk::Component *p_component)
		{
			auto iterator = _byEngine.find(p_engineId);

			if (iterator == _byEngine.end())
			{
				return;
			}

			std::vector<spk::Component *> &bucket = iterator->second;
			bucket.erase(std::remove(bucket.begin(), bucket.end(), p_component), bucket.end());

			if (bucket.empty() == true)
			{
				_byEngine.erase(iterator);
			}
		}

		void move(const spk::UUID &p_from, const spk::UUID &p_to, spk::Component *p_component)
		{
			if (p_from == p_to)
			{
				return;
			}

			remove(p_from, p_component);
			add(p_to, p_component);
		}

		void clearEngine(const spk::UUID &p_engineId)
		{
			_byEngine.erase(p_engineId);
		}

		[[nodiscard]] const std::vector<spk::Component *> &components(const spk::UUID &p_engineId) const
		{
			auto iterator = _byEngine.find(p_engineId);
			return (iterator == _byEngine.end() ? _emptyBucket() : iterator->second);
		}

		[[nodiscard]] bool empty() const
		{
			return _byEngine.empty();
		}
	};
}
