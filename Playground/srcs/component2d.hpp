#pragma once

#include "structures/game_engine/spk_component.hpp"

namespace pg
{
	class Entity2D;

	class Component2D : public spk::Component
	{
	protected:
		Component2D() = default;
		void _onAttached(spk::Entity &p_entity) override;

	public:
		~Component2D() override = default;

		[[nodiscard]] Entity2D *entity();
		[[nodiscard]] const Entity2D *entity() const;
	};
}
