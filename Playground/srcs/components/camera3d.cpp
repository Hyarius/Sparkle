#include "components/camera3d.hpp"

namespace pg
{
	Camera3D *Camera3D::_mainCamera = nullptr;

	Camera3D::Camera3D() :
		_projectionMatrix([this]() { return _generateProjectionMatrix(); })
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
		if (_aspectRatio <= 0.0f)
		{
			return spk::Matrix4x4::identity();
		}

		return spk::Matrix4x4::perspective(_fieldOfView, _aspectRatio, _nearPlane, _farPlane);
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
		if (p_height <= 0.0f || p_width <= 0.0f)
		{
			return;
		}

		setAspectRatio(p_width / p_height);
	}

	const spk::Vector3 &Camera3D::position() const
	{
		return _position;
	}

	const spk::Vector3 &Camera3D::target() const
	{
		return _target;
	}

	float Camera3D::aspectRatio() const
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
}
