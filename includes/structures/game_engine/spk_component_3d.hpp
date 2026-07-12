#pragma once

#include "structures/game_engine/spk_component.hpp"

namespace spk
{
	class Entity3D;
	class Transform3D;

	class Component3D : public spk::Component
	{
	protected:
		Component3D() = default;
		void _onAttached(spk::Entity &p_entity) override;

	public:
		~Component3D() override = default;

		[[nodiscard]] spk::Entity3D *entity();
		[[nodiscard]] const spk::Entity3D *entity() const;
		[[nodiscard]] spk::Transform3D &transform();
		[[nodiscard]] const spk::Transform3D &transform() const;
	};
}
