#pragma once

#include <cstdint>

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
	//
	// Cache coherency across the hierarchy is pull-based: every local mutation bumps a
	// per-transform generation counter, and modelTransform()/inverseModelTransform()
	// rebuild whenever the summed generation of this transform plus all of its ancestors
	// differs from the one the cached matrices were last built against. That way moving a
	// parent transparently invalidates every descendant's cached matrix without the parent
	// having to know who its children are. Both counters only ever increase, so the sum is
	// monotonic and never aliases a previously observed value.
	class Transform3D : public spk::Component
	{
	private:
		spk::Vector3 _position{0.0f, 0.0f, 0.0f};
		spk::Quaternion _rotation = spk::Quaternion::identity();
		spk::Vector3 _scale{1.0f, 1.0f, 1.0f};

		std::uint64_t _localGeneration = 1;
		mutable std::uint64_t _cachedAncestryGeneration = 0;

		mutable spk::CachedData<spk::Matrix4x4> _modelTransform;
		mutable spk::CachedData<spk::Matrix4x4> _inverseModelTransform;

		[[nodiscard]] const Transform3D *_parentTransform() const
		{
			const spk::Entity *owner = entity();
			for (const spk::Entity *ancestor = (owner != nullptr ? owner->parent() : nullptr);
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

		[[nodiscard]] spk::Matrix4x4 _generateModelTransform() const
		{
			const spk::Matrix4x4 localTransform =
				spk::Matrix4x4::translation(_position) *
				spk::Matrix4x4::rotation(_rotation) *
				spk::Matrix4x4::scale(_scale);

			const Transform3D *parentTransform = _parentTransform();
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

		// Sum of this transform's local generation and every ancestor's. Changes whenever
		// this transform or any ancestor is mutated, which is what lets a descendant notice
		// a parent moved and drop its stale cached matrices.
		[[nodiscard]] std::uint64_t _ancestryGeneration() const
		{
			const Transform3D *parentTransform = _parentTransform();
			return parentTransform != nullptr
					   ? _localGeneration + parentTransform->_ancestryGeneration()
					   : _localGeneration;
		}

		void _refreshIfAncestryChanged() const
		{
			const std::uint64_t generation = _ancestryGeneration();
			if (generation != _cachedAncestryGeneration)
			{
				_cachedAncestryGeneration = generation;
				_modelTransform.release();
				_inverseModelTransform.release();
			}
		}

		void _invalidate()
		{
			++_localGeneration;
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

		[[nodiscard]] const spk::Matrix4x4 &modelTransform() const
		{
			_refreshIfAncestryChanged();
			return _modelTransform.get();
		}

		[[nodiscard]] const spk::Matrix4x4 &inverseModelTransform() const
		{
			_refreshIfAncestryChanged();
			return _inverseModelTransform.get();
		}
	};
}
