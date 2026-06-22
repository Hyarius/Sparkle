#pragma once

#include "structures/container/spk_cached_data.hpp"
#include "structures/math/spk_matrix.hpp"
#include "structures/math/spk_rect_2d.hpp"
#include "structures/math/spk_vector2.hpp"
#include "structures/math/spk_vector3.hpp"

namespace spk
{
	class UniformBufferObject;

	class Viewport
	{
	public:
		static constexpr float DefaultMaxLayer = 1000.0f;

	private:
		static inline const Viewport *_activeViewport = nullptr;
		static inline float _maxLayer = DefaultMaxLayer;

		mutable spk::CachedData<spk::Matrix4x4> _matrix;

		void _configureMatrix();

	protected:
		spk::Rect2D _geometry;
		spk::Rect2D _scissor;

	public:
		Viewport();
		explicit Viewport(const spk::Rect2D &p_geometry);
		Viewport(const spk::Rect2D &p_geometry, const spk::Rect2D &p_scissor);
		Viewport(const Viewport &p_other);
		~Viewport() = default;

		void setGeometry(const spk::Rect2D &p_geometry);
		const spk::Rect2D &geometry() const;

		void setScissor(const spk::Rect2D &p_scissor);
		const spk::Rect2D &scissor() const;

		static void setMaxLayer(float p_maxLayer);
		static float maxLayer();

		void activate() const;

		const spk::Matrix4x4 &matrix() const;

		static const Viewport *activeViewport();
		static spk::UniformBufferObject &viewportUniformBuffer();

		static spk::Vector2 convertScreenToOpenGL(const spk::Vector2Int &p_screenPosition);
		static spk::Vector2 convertScreenToOpenGL(int p_x, int p_y);

		static spk::Vector3 convertScreenToOpenGL(const spk::Vector2Int &p_screenPosition, float p_layer);
		static spk::Vector3 convertScreenToOpenGL(int p_x, int p_y, float p_layer);

		static float convertLayerToOpenGL(float p_layer);
	};
}
