#include "structures/game_engine/spk_entity.hpp"

#include <algorithm>

namespace spk
{
	Entity::Entity(spk::Entity *p_parent) :
		spk::InherenceTrait<spk::Entity>(p_parent),
		_uuid(spk::UUID::generate())
	{
		activate();
		_activationContract = subscribeToActivation([this]() { _invalidateRegistryProcessable(); });
		_deactivationContract = subscribeToDeactivation([this]() { _invalidateRegistryProcessable(); });
	}

	Entity::Entity(const spk::UUID &p_uuid, spk::Entity *p_parent) :
		spk::InherenceTrait<spk::Entity>(p_parent),
		_uuid(p_uuid)
	{
		activate();
		_activationContract = subscribeToActivation([this]() { _invalidateRegistryProcessable(); });
		_deactivationContract = subscribeToDeactivation([this]() { _invalidateRegistryProcessable(); });
	}

	Entity::~Entity()
	{
		clearComponents();
	}

	void Entity::_setRegistry(spk::ComponentRegistry *p_registry)
	{
		if (_registry == p_registry)
		{
			return;
		}

		if (_registry != nullptr)
		{
			for (const std::unique_ptr<spk::Component> &component : _components)
			{
				_registry->remove(component.get());
			}
		}

		_registry = p_registry;

		if (_registry != nullptr)
		{
			for (const std::unique_ptr<spk::Component> &component : _components)
			{
				_registry->add(component.get());
			}
		}

		for (spk::Entity *child : children())
		{
			if (child != nullptr)
			{
				child->_setRegistry(p_registry);
			}
		}
	}

	void Entity::_invalidateRegistryProcessable()
	{
		if (_registry != nullptr)
		{
			_registry->invalidateProcessable();
		}
	}

	const spk::UUID &Entity::uuid() const
	{
		return _uuid;
	}

	bool Entity::isHierarchyActivated() const
	{
		const spk::Entity *current = this;

		while (current != nullptr)
		{
			if (current->isActivated() == false)
			{
				return false;
			}

			current = current->parent();
		}

		return true;
	}

	const std::vector<std::unique_ptr<spk::Component>> &Entity::components() const
	{
		return _components;
	}

	void Entity::removeComponent(spk::Component &p_component)
	{
		auto iterator = std::find_if(
			_components.begin(),
			_components.end(),
			[&p_component](const std::unique_ptr<spk::Component> &p_entry) {
				return (p_entry.get() == &p_component);
			});

		if (iterator == _components.end())
		{
			return;
		}

		if (_registry != nullptr)
		{
			_registry->remove(iterator->get());
		}

		(*iterator)->_detach();
		_components.erase(iterator);
	}

	void Entity::clearComponents()
	{
		for (const std::unique_ptr<spk::Component> &component : _components)
		{
			if (_registry != nullptr)
			{
				_registry->remove(component.get());
			}

			component->_detach();
		}

		_components.clear();
	}
}
