#pragma once

#include "components/transform3d.hpp"
#include "structures/game_engine/spk_entity.hpp"
#include "structures/math/spk_vector3.hpp"

namespace pg
{
	// spk::Entity that owns a Transform3D from construction (mirror of spk::Entity2D).
	// The cached pointer stays valid: components are heap-owned, so adding more never
	// moves existing ones.
	class Entity3D : public spk::Entity
	{
	private:
		Transform3D *_transform = nullptr;

	public:
		explicit Entity3D(spk::Entity *p_parent = nullptr) :
			spk::Entity(p_parent),
			_transform(&addComponent<Transform3D>())
		{
		}

		[[nodiscard]] Transform3D &transform() { return *_transform; }
		[[nodiscard]] const Transform3D &transform() const { return *_transform; }
		[[nodiscard]] const spk::Vector3 &position() const { return _transform->position(); }
	};
}
