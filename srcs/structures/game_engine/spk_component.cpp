#include "structures/game_engine/spk_component.hpp"

#include "structures/game_engine/spk_entity.hpp"

namespace spk
{
	Component::Component()
	{
		activate();
	}

	Component::~Component() = default;

	void Component::_attach(spk::Entity *p_entity)
	{
		_entity = p_entity;
	}

	void Component::_detach()
	{
		_entity = nullptr;
	}

	spk::Entity *Component::entity()
	{
		return _entity;
	}

	const spk::Entity *Component::entity() const
	{
		return _entity;
	}

	bool Component::hasEntity() const
	{
		return (_entity != nullptr);
	}

	bool Component::isProcessable() const
	{
		return (isActivated() == true && _entity != nullptr && _entity->isHierarchyActivated() == true);
	}
}
