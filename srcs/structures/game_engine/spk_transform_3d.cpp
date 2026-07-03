#include "structures/game_engine/spk_transform_3d.hpp"

#include "structures/game_engine/spk_entity.hpp"

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
		if (_rotation.x == p_rotation.x && _rotation.y == p_rotation.y &&
			_rotation.z == p_rotation.z && _rotation.w == p_rotation.w)
		{
			return;
		}
		_rotation = p_rotation;
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
}
