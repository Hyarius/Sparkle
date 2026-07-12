#include "structures/game_engine/spk_component_3d.hpp"

#include <stdexcept>

#include "structures/game_engine/spk_entity.hpp"
#include "structures/game_engine/spk_entity_3d.hpp"

namespace spk
{
	void Component3D::_onAttached(spk::Entity &p_entity)
	{
		if (dynamic_cast<spk::Entity3D *>(&p_entity) == nullptr)
		{
			throw std::invalid_argument("spk::Component3D can only be attached to spk::Entity3D");
		}
	}

	spk::Entity3D *Component3D::entity()
	{
		return static_cast<spk::Entity3D *>(spk::Component::entity());
	}

	const spk::Entity3D *Component3D::entity() const
	{
		return static_cast<const spk::Entity3D *>(spk::Component::entity());
	}
}
