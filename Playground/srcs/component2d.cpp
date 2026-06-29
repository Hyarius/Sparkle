#include "component2d.hpp"

#include <stdexcept>

#include "entity2d.hpp"
#include "structures/game_engine/spk_entity.hpp"

namespace pg
{
	void Component2D::_onAttached(spk::Entity &p_entity)
	{
		if (dynamic_cast<Entity2D *>(&p_entity) == nullptr)
		{
			throw std::invalid_argument("pg::Component2D can only be attached to pg::Entity2D");
		}
	}

	Entity2D *Component2D::entity()
	{
		return static_cast<Entity2D *>(spk::Component::entity());
	}

	const Entity2D *Component2D::entity() const
	{
		return static_cast<const Entity2D *>(spk::Component::entity());
	}
}
