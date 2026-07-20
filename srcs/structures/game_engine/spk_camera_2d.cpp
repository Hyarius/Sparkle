#include "structures/game_engine/spk_camera_2d.hpp"

#include "structures/game_engine/spk_entity.hpp"
#include "structures/game_engine/spk_entity_2d.hpp"
#include "structures/game_engine/spk_transform_2d.hpp"

namespace spk
{
	Camera2D *Camera2D::_mainCamera = nullptr;

	Camera2D::Camera2D() :
		_projectionMatrix([this]() {
			return _generateProjectionMatrix();
		})
	{
	}

	Camera2D::~Camera2D()
	{
		if (_mainCamera == this)
		{
			_mainCamera = nullptr;
		}
	}

	Camera2D *Camera2D::mainCamera()
	{
		return _mainCamera;
	}

	void Camera2D::makeMain()
	{
		_mainCamera = this;
	}

	void Camera2D::setViewport(const spk::Rect2D &p_viewport)
	{
		_viewport = p_viewport;
		_projectionMatrix.release();
	}

	const spk::Rect2D &Camera2D::viewport() const
	{
		return _viewport;
	}

	void Camera2D::setPixelsPerUnit(const spk::Vector2 &p_pixelsPerUnit)
	{
		_pixelsPerUnit = p_pixelsPerUnit;
		_projectionMatrix.release();
	}

	const spk::Vector2 &Camera2D::pixelsPerUnit() const
	{
		return _pixelsPerUnit;
	}

	spk::Matrix4x4 Camera2D::_generateProjectionMatrix() const
	{
		const float width = static_cast<float>(_viewport.width());
		const float height = static_cast<float>(_viewport.height());

		if (width <= 0.0f || height <= 0.0f || _pixelsPerUnit.x <= 0.0f || _pixelsPerUnit.y <= 0.0f)
		{
			return spk::Matrix4x4::identity();
		}

		const float halfWidth = (width / _pixelsPerUnit.x) * 0.5f;
		const float halfHeight = (height / _pixelsPerUnit.y) * 0.5f;

		return spk::Matrix4x4::ortho(
			-halfWidth,
			halfWidth,
			-halfHeight,
			halfHeight,
			-1000.0f,
			1000.0f);
	}

	const spk::Matrix4x4 &Camera2D::projectionMatrix() const
	{
		return _projectionMatrix.get();
	}

	spk::Matrix4x4 Camera2D::viewMatrix() const
	{
		if (entity() != nullptr)
		{
			return entity()->transform().inverseModelTransform();
		}

		return spk::Matrix4x4::identity();
	}
}
