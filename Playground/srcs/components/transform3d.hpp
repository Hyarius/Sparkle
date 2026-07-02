#pragma once

#include "structures/container/spk_cached_data.hpp"
#include "structures/game_engine/spk_component.hpp"
#include "structures/game_engine/spk_entity.hpp"
#include "structures/math/spk_matrix.hpp"
#include "structures/math/spk_quaternion.hpp"
#include "structures/math/spk_vector3.hpp"

namespace pg
{
	// 3D counterpart of spk::Transform2D: position (Vector3) + rotation (Quaternion) +
	// scale (Vector3), producing a cached model matrix (T * R * S) composed with any
	// ancestor Transform3D. Data-only spk::Component; header-only.
	class Transform3D : public spk::Component
	{
	private:
		spk::Vector3 _position{0.0f, 0.0f, 0.0f};
		spk::Quaternion _rotation = spk::Quaternion::identity();
		spk::Vector3 _scale{1.0f, 1.0f, 1.0f};

		mutable spk::CachedData<spk::Matrix4x4> _modelTransform;
		mutable spk::CachedData<spk::Matrix4x4> _inverseModelTransform;

		[[nodiscard]] spk::Matrix4x4 _generateModelTransform() const
		{
			const spk::Matrix4x4 localTransform =
				spk::Matrix4x4::translation(_position) *
				spk::Matrix4x4::rotation(_rotation) *
				spk::Matrix4x4::scale(_scale);

			const spk::Entity *owner = entity();
			const Transform3D *parentTransform = nullptr;
			for (const spk::Entity *ancestor = (owner != nullptr ? owner->parent() : nullptr);
				 ancestor != nullptr;
				 ancestor = ancestor->parent())
			{
				parentTransform = ancestor->component<Transform3D>();
				if (parentTransform != nullptr)
				{
					break;
				}
			}

			if (parentTransform != nullptr)
			{
				return parentTransform->modelTransform() * localTransform;
			}

			return localTransform;
		}

		[[nodiscard]] spk::Matrix4x4 _generateInverseModelTransform() const
		{
			return modelTransform().inverse();
		}

		void _invalidate()
		{
			_modelTransform.release();
			_inverseModelTransform.release();
		}

	public:
		Transform3D() :
			_modelTransform([this]() { return _generateModelTransform(); }),
			_inverseModelTransform([this]() { return _generateInverseModelTransform(); })
		{
		}

		[[nodiscard]] const spk::Vector3 &position() const { return _position; }
		[[nodiscard]] const spk::Quaternion &rotation() const { return _rotation; }
		[[nodiscard]] const spk::Vector3 &scale() const { return _scale; }

		void setPosition(const spk::Vector3 &p_position)
		{
			_position = p_position;
			_invalidate();
		}

		void move(const spk::Vector3 &p_delta)
		{
			setPosition(_position + p_delta);
		}

		void setRotation(const spk::Quaternion &p_rotation)
		{
			_rotation = p_rotation;
			_invalidate();
		}

		void setScale(const spk::Vector3 &p_scale)
		{
			_scale = p_scale;
			_invalidate();
		}

		[[nodiscard]] const spk::Matrix4x4 &modelTransform() const { return _modelTransform.get(); }
		[[nodiscard]] const spk::Matrix4x4 &inverseModelTransform() const { return _inverseModelTransform.get(); }
	};
}
