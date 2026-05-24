#include "rendering/spk_viewport.hpp"

#include <stdexcept>

#include <GL/glew.h>

#include "window/spk_surface_state.hpp"

namespace spk
{
	void Viewport::_applyToGraphicsContext(const spk::SurfaceState& p_surfaceState) const
	{
		const spk::Vector2UInt& windowSize = p_surfaceState.rect().size;
		if (windowSize.x == 0 || windowSize.y == 0)
		{
			throw std::runtime_error("spk::Viewport::_applyToGraphicsContext() - window size must be positive");
		}

		const GLint viewportGLY = static_cast<GLint>(windowSize.y)
			- _geometry.y()
			- static_cast<GLint>(_geometry.height());

		glViewport(
			static_cast<GLint>(_geometry.x()),
			viewportGLY,
			static_cast<GLsizei>(_geometry.width()),
			static_cast<GLsizei>(_geometry.height()));

		const GLint scissorGLY = static_cast<GLint>(windowSize.y)
			- _scissor.y()
			- static_cast<GLint>(_scissor.height());

		glEnable(GL_SCISSOR_TEST);
		glScissor(
			static_cast<GLint>(_scissor.x()),
			scissorGLY,
			static_cast<GLsizei>(_scissor.width()),
			static_cast<GLsizei>(_scissor.height()));
	}

	void Viewport::_configureMatrix()
	{
		_matrix.configure([this]() -> spk::Matrix4x4
		{
			const float left   = 0.0f;
			const float right  = static_cast<float>(_geometry.width());
			const float top    = 0.0f;
			const float bottom = static_cast<float>(_geometry.height());

			return spk::Matrix4x4::ortho(left, right, bottom, top, -_maxLayer, _maxLayer);
		});
	}

	Viewport::Viewport() :
		_geometry({0, 0}, {1, 1}),
		_scissor({0, 0}, {1, 1})
	{
		_configureMatrix();
	}

	Viewport::Viewport(const spk::Rect2D& p_geometry) :
		_geometry(p_geometry),
		_scissor(p_geometry)
	{
		_configureMatrix();
	}

	Viewport::Viewport(const spk::Rect2D& p_geometry, const spk::Rect2D& p_scissor) :
		_geometry(p_geometry),
		_scissor(p_scissor)
	{
		_configureMatrix();
	}

	void Viewport::setGeometry(const spk::Rect2D& p_geometry)
	{
		_geometry = p_geometry;
		_matrix.release();
	}

	const spk::Rect2D& Viewport::geometry() const
	{
		return _geometry;
	}

	void Viewport::setScissor(const spk::Rect2D& p_scissor)
	{
		_scissor = p_scissor;
	}

	const spk::Rect2D& Viewport::scissor() const
	{
		return _scissor;
	}

	void Viewport::setMaxLayer(float p_maxLayer)
	{
		_maxLayer = p_maxLayer;
	}

	float Viewport::maxLayer()
	{
		return _maxLayer;
	}

	void Viewport::activate(const spk::SurfaceState& p_surfaceState) const
	{
		if (_geometry.width() == 0 || _geometry.height() == 0)
		{
			throw std::runtime_error("spk::Viewport::activate() - geometry size must be positive");
		}

		if (_scissor.width() == 0 || _scissor.height() == 0)
		{
			throw std::runtime_error("spk::Viewport::activate() - scissor size must be positive");
		}

		_applyToGraphicsContext(p_surfaceState);

		_activeViewport = this;
	}

	const spk::Matrix4x4& Viewport::matrix() const
	{
		return _matrix.get();
	}

	const Viewport* Viewport::activeViewport()
	{
		return _activeViewport;
	}

	spk::Vector2 Viewport::convertScreenToOpenGL(const spk::Vector2Int& p_screenPosition)
	{
		const spk::Matrix4x4& mat = _activeViewport->matrix();
		const spk::Vector4 result = mat * spk::Vector4(
			static_cast<float>(p_screenPosition.x),
			static_cast<float>(p_screenPosition.y),
			0.0f,
			1.0f);
		return {result.x, result.y};
	}

	spk::Vector2 Viewport::convertScreenToOpenGL(int p_x, int p_y)
	{
		return convertScreenToOpenGL(spk::Vector2Int{p_x, p_y});
	}

	spk::Vector3 Viewport::convertScreenToOpenGL(const spk::Vector2Int& p_screenPosition, float p_layer)
	{
		const spk::Matrix4x4& mat = _activeViewport->matrix();
		const spk::Vector4 result = mat * spk::Vector4(
			static_cast<float>(p_screenPosition.x),
			static_cast<float>(p_screenPosition.y),
			p_layer,
			1.0f);
		return {result.x, result.y, result.z};
	}

	spk::Vector3 Viewport::convertScreenToOpenGL(int p_x, int p_y, float p_layer)
	{
		return convertScreenToOpenGL(spk::Vector2Int{p_x, p_y}, p_layer);
	}

	float Viewport::convertLayerToOpenGL(float p_layer)
	{
		return p_layer / _maxLayer;
	}
}
