#pragma once

#include "component2d.hpp"
#include "structures/container/spk_cached_data.hpp"
#include "structures/design_pattern/spk_contract_provider.hpp"
#include "structures/math/spk_matrix.hpp"
#include "structures/math/spk_vector2.hpp"

namespace pg
{
	class Entity2D;

	class Transform2D : public Component2D
	{
		friend class Entity2D;

	public:
		using EditionProvider = spk::ContractProvider<const Transform2D *>;
		using EditionContract = EditionProvider::Contract;
		using EditionCallback = EditionProvider::Callback;

	private:
		spk::Vector2 _position{0.0f, 0.0f};
		float _rotation = 0.0f;
		spk::Vector2 _scale{1.0f, 1.0f};
		float _layer = 0.0f;
		mutable spk::CachedData<spk::Matrix4x4> _modelTransform;
		mutable spk::CachedData<spk::Matrix4x4> _inverseModelTransform;

		EditionProvider _modelTransformEditionProvider;

		void _notifyModelTransformEdition();
		static void _notifyDescendantModelTransformEditions(spk::Entity &p_entity);
		[[nodiscard]] spk::Matrix4x4 _generateModelTransform() const;
		[[nodiscard]] spk::Matrix4x4 _generateInverseModelTransform() const;

	public:
		Transform2D();

		[[nodiscard]] const spk::Vector2 &position() const
		{
			return _position;
		}

		[[nodiscard]] float rotation() const
		{
			return _rotation;
		}

		[[nodiscard]] const spk::Vector2 &scale() const
		{
			return _scale;
		}

		[[nodiscard]] float layer() const
		{
			return _layer;
		}

		void setPosition(const spk::Vector2 &p_position);
		void move(const spk::Vector2 &p_delta);
		void setRotation(float p_degrees);
		void setScale(const spk::Vector2 &p_scale);
		void setLayer(float p_layer);

		[[nodiscard]] const spk::Matrix4x4 &modelTransform() const;
		[[nodiscard]] const spk::Matrix4x4 &inverseModelTransform() const;

		[[nodiscard]] EditionContract subscribeToModelTransformEdition(EditionCallback p_callback);
	};
}
