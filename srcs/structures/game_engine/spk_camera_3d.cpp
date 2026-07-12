#include "structures/game_engine/spk_camera_3d.hpp"

#include "structures/math/spk_vector4.hpp"

#include <cmath>
#include <stdexcept>

namespace spk
{
	Camera3D *Camera3D::_mainCamera = nullptr;

	Camera3D::Camera3D() :
		_projectionMatrix([this]() {
			return _generateProjectionMatrix();
		})
	{
	}

	Camera3D::~Camera3D()
	{
		if (_mainCamera == this)
		{
			_mainCamera = nullptr;
		}
	}

	Camera3D *Camera3D::mainCamera()
	{
		return _mainCamera;
	}

	void Camera3D::makeMain()
	{
		_mainCamera = this;
	}

	spk::Matrix4x4 Camera3D::_generateProjectionMatrix() const
	{
		return _aspectRatio <= 0.0f
				   ? spk::Matrix4x4::identity()
				   : spk::Matrix4x4::perspective(_fieldOfView, _aspectRatio, _nearPlane, _farPlane);
	}

	void Camera3D::setPosition(const spk::Vector3 &p_position)
	{
		_position = p_position;
	}

	void Camera3D::setTarget(const spk::Vector3 &p_target)
	{
		_target = p_target;
	}

	void Camera3D::setUp(const spk::Vector3 &p_up)
	{
		_up = p_up;
	}

	void Camera3D::setPerspective(float p_fieldOfView, float p_nearPlane, float p_farPlane)
	{
		_fieldOfView = p_fieldOfView;
		_nearPlane = p_nearPlane;
		_farPlane = p_farPlane;
		_projectionMatrix.release();
	}

	void Camera3D::setAspectRatio(float p_aspectRatio)
	{
		if (_aspectRatio == p_aspectRatio)
		{
			return;
		}
		_aspectRatio = p_aspectRatio;
		_projectionMatrix.release();
	}

	void Camera3D::setViewportSize(float p_width, float p_height)
	{
		if (p_width > 0.0f && p_height > 0.0f)
		{
			setAspectRatio(p_width / p_height);
		}
	}

	const spk::Vector3 &Camera3D::position() const
	{
		return _position;
	}

	const spk::Vector3 &Camera3D::target() const
	{
		return _target;
	}

	const spk::Vector3 &Camera3D::up() const noexcept
	{
		return _up;
	}

	float Camera3D::fieldOfView() const noexcept
	{
		return _fieldOfView;
	}

	float Camera3D::nearPlane() const noexcept
	{
		return _nearPlane;
	}

	float Camera3D::farPlane() const noexcept
	{
		return _farPlane;
	}

	float Camera3D::aspectRatio() const noexcept
	{
		return _aspectRatio;
	}

	const spk::Matrix4x4 &Camera3D::projectionMatrix() const
	{
		return _projectionMatrix.get();
	}

	spk::Matrix4x4 Camera3D::viewMatrix() const
	{
		return spk::Matrix4x4::lookAt(_position, _target, _up);
	}

	spk::Matrix4x4 Camera3D::viewProjectionMatrix() const
	{
		return projectionMatrix() * viewMatrix();
	}

	spk::Ray3D Camera3D::rayFromViewport(
		const spk::Vector2 &p_viewportSize,
		const spk::Vector2 &p_pixelPosition) const
	{
		if (!std::isfinite(p_viewportSize.x) || !std::isfinite(p_viewportSize.y) ||
			p_viewportSize.x <= 0.0f || p_viewportSize.y <= 0.0f)
		{
			throw std::invalid_argument("Camera3D ray viewport must be finite and positive");
		}
		if (!std::isfinite(p_pixelPosition.x) || !std::isfinite(p_pixelPosition.y))
		{
			throw std::invalid_argument("Camera3D ray pixel position must be finite");
		}

		const float ndcX = 2.0f * p_pixelPosition.x / p_viewportSize.x - 1.0f;
		const float ndcY = 1.0f - 2.0f * p_pixelPosition.y / p_viewportSize.y;
		const spk::Matrix4x4 inverse = viewProjectionMatrix().inverse();
		spk::Vector4 nearPoint = inverse * spk::Vector4{ndcX, ndcY, -1.0f, 1.0f};
		spk::Vector4 farPoint = inverse * spk::Vector4{ndcX, ndcY, 1.0f, 1.0f};
		nearPoint /= nearPoint.w;
		farPoint /= farPoint.w;

		return {
			.origin = position(),
			.direction = (farPoint.xyz() - nearPoint.xyz()).normalized()};
	}
}
