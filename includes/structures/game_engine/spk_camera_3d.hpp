#pragma once

#include <array>

#include "structures/container/spk_cached_data.hpp"
#include "structures/game_engine/spk_component.hpp"
#include "structures/math/spk_matrix.hpp"
#include "structures/math/spk_ray_3d.hpp"
#include "structures/math/spk_vector2.hpp"
#include "structures/math/spk_vector3.hpp"

namespace spk
{
	class Camera3D : public spk::Component
	{
	private:
		static Camera3D *_mainCamera;

		spk::Vector3 _position{0.0f, 0.0f, 5.0f};
		spk::Vector3 _target{0.0f, 0.0f, 0.0f};
		spk::Vector3 _up{0.0f, 1.0f, 0.0f};
		float _fieldOfView = 60.0f;
		float _nearPlane = 0.1f;
		float _farPlane = 1000.0f;
		float _aspectRatio = 1.0f;
		mutable spk::CachedData<spk::Matrix4x4> _projectionMatrix;

		[[nodiscard]] spk::Matrix4x4 _generateProjectionMatrix() const;

	public:
		Camera3D();
		~Camera3D() override;

		[[nodiscard]] static Camera3D *mainCamera();
		void makeMain();

		void setPosition(const spk::Vector3 &p_position);
		void setTarget(const spk::Vector3 &p_target);
		void setUp(const spk::Vector3 &p_up);
		void setPerspective(float p_fieldOfView, float p_nearPlane, float p_farPlane);
		void setAspectRatio(float p_aspectRatio);
		void setViewportSize(float p_width, float p_height);

		[[nodiscard]] const spk::Vector3 &position() const;
		[[nodiscard]] const spk::Vector3 &target() const;
		[[nodiscard]] const spk::Vector3 &up() const noexcept;
		[[nodiscard]] float fieldOfView() const noexcept;
		[[nodiscard]] float nearPlane() const noexcept;
		[[nodiscard]] float farPlane() const noexcept;
		[[nodiscard]] float aspectRatio() const noexcept;
		[[nodiscard]] const spk::Matrix4x4 &projectionMatrix() const;
		[[nodiscard]] spk::Matrix4x4 viewMatrix() const;
		[[nodiscard]] spk::Matrix4x4 viewProjectionMatrix() const;
		[[nodiscard]] std::array<spk::Vector3, 8> frustumSliceCorners(float p_nearDistance, float p_farDistance) const;
		// Builds a perspective ray through a viewport pixel. Pixel coordinates use
		// the usual UI convention: origin at the viewport's top-left.
		[[nodiscard]] spk::Ray3D rayFromViewport(
			const spk::Vector2 &p_viewportSize,
			const spk::Vector2 &p_pixelPosition) const;
	};
}
