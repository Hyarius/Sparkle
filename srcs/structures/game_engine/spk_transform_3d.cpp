#include "structures/game_engine/spk_transform_3d.hpp"

#include <cmath>
#include <stdexcept>

#include "structures/game_engine/spk_entity.hpp"
#include "structures/math/spk_vector4.hpp"

namespace
{
	[[nodiscard]] spk::Quaternion normalizedFiniteQuaternion(const spk::Quaternion &p_quaternion)
	{
		if (std::isfinite(p_quaternion.x) == false ||
			std::isfinite(p_quaternion.y) == false ||
			std::isfinite(p_quaternion.z) == false ||
			std::isfinite(p_quaternion.w) == false)
		{
			throw std::invalid_argument("spk::Transform3D rotation quaternion must be finite");
		}

		const float squaredLength =
			p_quaternion.x * p_quaternion.x +
			p_quaternion.y * p_quaternion.y +
			p_quaternion.z * p_quaternion.z +
			p_quaternion.w * p_quaternion.w;

		if (std::isfinite(squaredLength) == false || squaredLength <= 0.0f)
		{
			throw std::invalid_argument("spk::Transform3D rotation quaternion must have a non-zero finite length");
		}

		const float inverseLength = 1.0f / std::sqrt(squaredLength);
		return {
			p_quaternion.x * inverseLength,
			p_quaternion.y * inverseLength,
			p_quaternion.z * inverseLength,
			p_quaternion.w * inverseLength};
	}

	[[nodiscard]] spk::Quaternion multiply(
		const spk::Quaternion &p_left,
		const spk::Quaternion &p_right)
	{
		return {
			p_left.w * p_right.x + p_left.x * p_right.w + p_left.y * p_right.z - p_left.z * p_right.y,
			p_left.w * p_right.y - p_left.x * p_right.z + p_left.y * p_right.w + p_left.z * p_right.x,
			p_left.w * p_right.z + p_left.x * p_right.y - p_left.y * p_right.x + p_left.z * p_right.w,
			p_left.w * p_right.w - p_left.x * p_right.x - p_left.y * p_right.y - p_left.z * p_right.z};
	}
}

namespace spk
{
	Transform3D::Transform3D() :
		_modelTransform([this]() {
			return _generateModelTransform();
		}),
		_inverseModelTransform([this]() {
			return _generateInverseModelTransform();
		})
	{
	}

	const Transform3D *Transform3D::_parentTransform() const
	{
		const spk::Entity *owner = entity();
		for (const spk::Entity *ancestor = owner == nullptr ? nullptr : owner->parent();
			 ancestor != nullptr;
			 ancestor = ancestor->parent())
		{
			if (const Transform3D *transform = ancestor->component<Transform3D>(); transform != nullptr)
			{
				return transform;
			}
		}
		return nullptr;
	}

	spk::Matrix4x4 Transform3D::_generateModelTransform() const
	{
		const spk::Matrix4x4 localTransform =
			spk::Matrix4x4::translation(_position) *
			spk::Matrix4x4::rotation(_rotation) *
			spk::Matrix4x4::scale(_scale);
		const Transform3D *parentTransform = _parentTransform();
		return parentTransform == nullptr
				   ? localTransform
				   : parentTransform->modelTransform() * localTransform;
	}

	spk::Matrix4x4 Transform3D::_generateInverseModelTransform() const
	{
		return modelTransform().inverse();
	}

	void Transform3D::_invalidateDescendants(spk::Entity &p_entity)
	{
		for (spk::Entity *child : p_entity.children())
		{
			if (child == nullptr)
			{
				continue;
			}
			if (Transform3D *transform = child->component<Transform3D>(); transform != nullptr)
			{
				transform->_modelTransform.release();
				transform->_inverseModelTransform.release();
			}
			_invalidateDescendants(*child);
		}
	}

	void Transform3D::_invalidate()
	{
		_modelTransform.release();
		_inverseModelTransform.release();
		if (spk::Entity *owner = entity(); owner != nullptr)
		{
			_invalidateDescendants(*owner);
		}
	}

	const spk::Vector3 &Transform3D::position() const
	{
		return _position;
	}

	const spk::Quaternion &Transform3D::rotation() const
	{
		return _rotation;
	}

	const spk::Vector3 &Transform3D::scale() const
	{
		return _scale;
	}

	void Transform3D::setPosition(const spk::Vector3 &p_position)
	{
		if (_position == p_position)
		{
			return;
		}
		_position = p_position;
		_invalidate();
	}

	void Transform3D::move(const spk::Vector3 &p_delta)
	{
		setPosition(_position + p_delta);
	}

	void Transform3D::setRotation(const spk::Quaternion &p_rotation)
	{
		const spk::Quaternion normalized = normalizedFiniteQuaternion(p_rotation);
		if (_rotation.x == normalized.x && _rotation.y == normalized.y &&
			_rotation.z == normalized.z && _rotation.w == normalized.w)
		{
			return;
		}
		_rotation = normalized;
		_invalidate();
	}

	void Transform3D::setScale(const spk::Vector3 &p_scale)
	{
		if (_scale == p_scale)
		{
			return;
		}
		_scale = p_scale;
		_invalidate();
	}

	const spk::Matrix4x4 &Transform3D::modelTransform() const
	{
		return _modelTransform.get();
	}

	const spk::Matrix4x4 &Transform3D::inverseModelTransform() const
	{
		return _inverseModelTransform.get();
	}

	spk::Quaternion Transform3D::worldRotation() const
	{
		const spk::Quaternion localRotation = normalizedFiniteQuaternion(_rotation);
		const spk::Transform3D *parentTransform = _parentTransform();
		if (parentTransform == nullptr)
		{
			return localRotation;
		}
		return normalizedFiniteQuaternion(
			multiply(parentTransform->worldRotation(), localRotation));
	}

	spk::Vector3 Transform3D::worldForward() const
	{
		const spk::Vector4 forward =
			spk::Matrix4x4::rotation(worldRotation()) *
			spk::Vector4(0.0f, 0.0f, -1.0f, 0.0f);
		return forward.xyz().normalized();
	}

	spk::Vector3 Transform3D::worldPosition() const
	{
		return modelTransform() * spk::Vector3::Zero;
	}
}
