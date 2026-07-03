#pragma once

#include "structures/game_engine/spk_entity.hpp"
#include "structures/game_engine/spk_transform_3d.hpp"
#include "structures/math/spk_vector3.hpp"

namespace spk
{
	class Entity3D : public spk::Entity
	{
	private:
		spk::Transform3D *_transform = nullptr;

	protected:
		void _onParentChanged(spk::Entity *p_oldParent, spk::Entity *p_newParent) override
		{
			spk::Entity::_onParentChanged(p_oldParent, p_newParent);
			if (_transform != nullptr)
			{
				_transform->_invalidate();
			}
		}

	public:
		explicit Entity3D(spk::Entity *p_parent = nullptr) :
			spk::Entity(p_parent),
			_transform(&addComponent<spk::Transform3D>())
		{
		}

		[[nodiscard]] spk::Transform3D &transform()
		{
			return *_transform;
		}

		[[nodiscard]] const spk::Transform3D &transform() const
		{
			return *_transform;
		}

		[[nodiscard]] const spk::Vector3 &position() const
		{
			return _transform->position();
		}
	};
}
