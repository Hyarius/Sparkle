#pragma once

#include "components/transform2d.hpp"
#include "structures/game_engine/spk_entity.hpp"
#include "structures/math/spk_vector2.hpp"

namespace pg
{
	class Entity2D : public spk::Entity
	{
	private:
		Transform2D *_transform = nullptr;

	protected:
		void _onParentChanged(spk::Entity *p_oldParent, spk::Entity *p_newParent) override
		{
			spk::Entity::_onParentChanged(p_oldParent, p_newParent);

			if (_transform != nullptr)
			{
				_transform->_notifyModelTransformEdition();
			}
		}

	public:
		explicit Entity2D(spk::Entity *p_parent = nullptr) :
			spk::Entity(p_parent),
			_transform(&addComponent<Transform2D>())
		{
		}

		[[nodiscard]] Transform2D &transform()
		{
			return *_transform;
		}

		[[nodiscard]] const Transform2D &transform() const
		{
			return *_transform;
		}

		[[nodiscard]] const spk::Vector2 &position() const
		{
			return _transform->position();
		}
	};
}
