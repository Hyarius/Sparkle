#pragma once

#include "math/spk_matrix.hpp"
#include "math/spk_rect_2d.hpp"
#include "math/spk_vector2.hpp"
#include "math/spk_vector3.hpp"
#include "utils/spk_cached_data.hpp"

namespace spk
{
	class ISurfaceState;

	class Viewport
	{
	public:
		static constexpr float DefaultMaxLayer = 1000.0f;

	private:
		static inline const Viewport* _activeViewport = nullptr;
		static inline float _maxLayer = DefaultMaxLayer;

		mutable spk::CachedData<spk::Matrix4x4> _matrix;

		void _configureMatrix();
		virtual void _applyToGraphicsContext(const spk::ISurfaceState& p_surfaceState) const = 0;

	protected:
		spk::Rect2D _geometry;
		spk::Rect2D _scissor;
	public:
		Viewport();
		explicit Viewport(const spk::Rect2D& p_geometry);
		Viewport(const spk::Rect2D& p_geometry, const spk::Rect2D& p_scissor);
		virtual ~Viewport() = default;

		void setGeometry(const spk::Rect2D& p_geometry);
		const spk::Rect2D& geometry() const;

		void setScissor(const spk::Rect2D& p_scissor);
		const spk::Rect2D& scissor() const;

		static void setMaxLayer(float p_maxLayer);
		static float maxLayer();

		void activate(const spk::ISurfaceState& p_surfaceState) const;

		const spk::Matrix4x4& matrix() const;

		static const Viewport* activeViewport();

		static spk::Vector2 convertScreenToOpenGL(const spk::Vector2Int& p_screenPosition);
		static spk::Vector2 convertScreenToOpenGL(int p_x, int p_y);

		static spk::Vector3 convertScreenToOpenGL(const spk::Vector2Int& p_screenPosition, float p_layer);
		static spk::Vector3 convertScreenToOpenGL(int p_x, int p_y, float p_layer);

		static float convertLayerToOpenGL(float p_layer);
	};
}
