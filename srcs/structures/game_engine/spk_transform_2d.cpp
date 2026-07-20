#include "structures/game_engine/spk_transform_2d.hpp"

#include <utility>

#include "structures/game_engine/spk_entity.hpp"
#include "structures/game_engine/spk_entity_2d.hpp"

namespace spk
{
	Transform2D::Transform2D() :
		_modelTransform([this]() {
			return _generateModelTransform();
		}),
		_inverseModelTransform([this]() {
			return _generateInverseModelTransform();
		})
	{
	}

	void Transform2D::_notifyDescendantModelTransformEditions(spk::Entity &p_entity)
	{
		for (spk::Entity *child : p_entity.children())
		{
			if (child == nullptr)
			{
				continue;
			}

			if (Transform2D *transform = child->component<Transform2D>(); transform != nullptr)
			{
				transform->_modelTransform.release();
				transform->_inverseModelTransform.release();
				transform->_modelTransformEditionProvider.trigger(transform);
			}

			_notifyDescendantModelTransformEditions(*child);
		}
	}

	void Transform2D::_notifyModelTransformEdition()
	{
		_modelTransform.release();
		_inverseModelTransform.release();
		_modelTransformEditionProvider.trigger(this);

		if (spk::Entity *owner = entity(); owner != nullptr)
		{
			_notifyDescendantModelTransformEditions(*owner);
		}
	}

	void Transform2D::setPosition(const spk::Vector2 &p_position)
	{
		if (_position == p_position)
		{
			return;
		}

		_position = p_position;
		_notifyModelTransformEdition();
	}

	void Transform2D::move(const spk::Vector2 &p_delta)
	{
		setPosition(_position + p_delta);
	}

	void Transform2D::setRotation(float p_degrees)
	{
		if (_rotation == p_degrees)
		{
			return;
		}

		_rotation = p_degrees;
		_notifyModelTransformEdition();
	}

	void Transform2D::setScale(const spk::Vector2 &p_scale)
	{
		if (_scale == p_scale)
		{
			return;
		}

		_scale = p_scale;
		_notifyModelTransformEdition();
	}

	void Transform2D::setLayer(float p_layer)
	{
		if (_layer == p_layer)
		{
			return;
		}

		_layer = p_layer;
		_notifyModelTransformEdition();
	}

	spk::Matrix4x4 Transform2D::_generateModelTransform() const
	{
		const spk::Matrix4x4 localTransform =
			spk::Matrix4x4::translation({_position.x, _position.y, _layer}) *
			spk::Matrix4x4::rotation(0.0f, 0.0f, _rotation) *
			spk::Matrix4x4::scale({_scale.x, _scale.y, 1.0f});

		const spk::Entity *owner = entity();
		const Transform2D *parentTransform = nullptr;
		for (const spk::Entity *ancestor = (owner != nullptr ? owner->parent() : nullptr);
			 ancestor != nullptr;
			 ancestor = ancestor->parent())
		{
			parentTransform = ancestor->component<Transform2D>();
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

	spk::Matrix4x4 Transform2D::_generateInverseModelTransform() const
	{
		return modelTransform().inverse();
	}

	const spk::Matrix4x4 &Transform2D::modelTransform() const
	{
		return _modelTransform.get();
	}

	const spk::Matrix4x4 &Transform2D::inverseModelTransform() const
	{
		return _inverseModelTransform.get();
	}

	Transform2D::EditionContract Transform2D::subscribeToModelTransformEdition(EditionCallback p_callback)
	{
		return _modelTransformEditionProvider.subscribe(std::move(p_callback));
	}
}
