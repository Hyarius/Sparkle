#include "structures/game_engine/spk_entity.hpp"

#include <algorithm>
#include <typeindex>
#include <typeinfo>

namespace
{
	std::type_index _componentType(const spk::Component *p_component)
	{
		return std::type_index(typeid(*p_component));
	}
}

namespace spk
{
	Entity::Entity(spk::Entity *p_parent) :
		spk::InherenceTrait<spk::Entity>(p_parent),
		_uuid(spk::UUID::generate())
	{
		_engineId = (parent() != nullptr ? parent()->_engineId : spk::UUID::null());

		_activationContract = subscribeToActivation([this]() { _refreshGlobalActivated(); });
		_deactivationContract = subscribeToDeactivation([this]() { _refreshGlobalActivated(); });

		activate();
	}

	Entity::Entity(const spk::UUID &p_uuid, spk::Entity *p_parent) :
		spk::InherenceTrait<spk::Entity>(p_parent),
		_uuid(p_uuid)
	{
		_engineId = (parent() != nullptr ? parent()->_engineId : spk::UUID::null());

		_activationContract = subscribeToActivation([this]() { _refreshGlobalActivated(); });
		_deactivationContract = subscribeToDeactivation([this]() { _refreshGlobalActivated(); });

		activate();
	}

	Entity::~Entity()
	{
		clearComponents();
	}

	void Entity::_refreshGlobalActivated()
	{
		const bool parentGlobal = (parent() != nullptr ? parent()->_globalActivated : true);
		const bool newGlobal = (isActivated() == true && parentGlobal == true);

		if (newGlobal == _globalActivated)
		{
			return;
		}

		_globalActivated = newGlobal;

		for (spk::Entity *child : children())
		{
			if (child != nullptr)
			{
				child->_refreshGlobalActivated();
			}
		}
	}

	void Entity::_setEngineId(const spk::UUID &p_engineId)
	{
		if (_engineId == p_engineId)
		{
			return;
		}

		const spk::UUID oldId = _engineId;
		_engineId = p_engineId;

		spk::ComponentStore &store = spk::ComponentStore::instance();

		for (const std::unique_ptr<spk::Component> &component : _components)
		{
			spk::Component *componentPtr = component.get();

			if (componentPtr == nullptr)
			{
				continue;
			}

			const std::type_index type = _componentType(componentPtr);

			if (oldId.isNull() == false && p_engineId.isNull() == false)
			{
				store.move(type, oldId, p_engineId, componentPtr);
			}
			else if (oldId.isNull() == false)
			{
				store.remove(type, oldId, componentPtr);
			}
			else if (p_engineId.isNull() == false)
			{
				store.add(type, p_engineId, componentPtr);
			}
		}

		for (spk::Entity *child : children())
		{
			if (child != nullptr)
			{
				child->_setEngineId(p_engineId);
			}
		}
	}

	void Entity::_onParentChanged(spk::Entity *p_oldParent, spk::Entity *p_newParent)
	{
		(void)p_oldParent;

		_setEngineId(p_newParent != nullptr ? p_newParent->_engineId : spk::UUID::null());

		_refreshGlobalActivated();
	}

	const spk::UUID &Entity::uuid() const
	{
		return _uuid;
	}

	bool Entity::isHierarchyActivated() const
	{
		return _globalActivated;
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

		if (_engineId.isNull() == false)
		{
			spk::Component *componentPtr = iterator->get();

			spk::ComponentStore::instance().remove(
				_componentType(componentPtr),
				_engineId,
				componentPtr
			);
		}

		(*iterator)->_detach();
		_components.erase(iterator);
	}

	void Entity::clearComponents()
	{
		spk::ComponentStore &store = spk::ComponentStore::instance();
		const bool registered = (_engineId.isNull() == false);

		for (const std::unique_ptr<spk::Component> &component : _components)
		{
			if (registered == true)
			{
				spk::Component *componentPtr = component.get();

				store.remove(_componentType(componentPtr), _engineId, componentPtr);
			}

			component->_detach();
		}

		_components.clear();
	}
}
