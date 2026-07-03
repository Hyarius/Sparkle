#pragma once

#include "structures/container/spk_cached_data.hpp"
#include "structures/game_engine/spk_component.hpp"
#include "structures/math/spk_matrix.hpp"
#include "structures/math/spk_quaternion.hpp"
#include "structures/math/spk_vector3.hpp"

namespace spk
{
	class Entity;
	class Entity3D;

	class Transform3D : public spk::Component
	{
		friend class Entity3D;

	private:
		spk::Vector3 _position{0.0f, 0.0f, 0.0f};
		spk::Quaternion _rotation = spk::Quaternion::identity();
		spk::Vector3 _scale{1.0f, 1.0f, 1.0f};

		mutable spk::CachedData<spk::Matrix4x4> _modelTransform;
		mutable spk::CachedData<spk::Matrix4x4> _inverseModelTransform;

		[[nodiscard]] const Transform3D *_parentTransform() const;
		[[nodiscard]] spk::Matrix4x4 _generateModelTransform() const;
		[[nodiscard]] spk::Matrix4x4 _generateInverseModelTransform() const;
		static void _invalidateDescendants(spk::Entity &p_entity);
		void _invalidate();

	public:
		Transform3D();

		[[nodiscard]] const spk::Vector3 &position() const;
		[[nodiscard]] const spk::Quaternion &rotation() const;
		[[nodiscard]] const spk::Vector3 &scale() const;

		void setPosition(const spk::Vector3 &p_position);
		void move(const spk::Vector3 &p_delta);
		void setRotation(const spk::Quaternion &p_rotation);
		void setScale(const spk::Vector3 &p_scale);

		[[nodiscard]] const spk::Matrix4x4 &modelTransform() const;
		[[nodiscard]] const spk::Matrix4x4 &inverseModelTransform() const;
	};
}
