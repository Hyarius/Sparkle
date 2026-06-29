#pragma once

#include "structures/container/spk_cached_data.hpp"
#include "structures/game_engine/spk_component_2d.hpp"
#include "structures/math/spk_matrix.hpp"
#include "structures/math/spk_rect_2d.hpp"
#include "structures/math/spk_vector2.hpp"

namespace spk
{
	class Camera2D : public spk::Component2D
	{
	private:
		static Camera2D *_mainCamera;

		spk::Rect2D _viewport;
		spk::Vector2 _pixelsPerUnit{64.0f, 64.0f};
		mutable spk::CachedData<spk::Matrix4x4> _projectionMatrix;

		[[nodiscard]] spk::Matrix4x4 _generateProjectionMatrix() const;

	public:
		Camera2D();
		~Camera2D() override;

		[[nodiscard]] static Camera2D *mainCamera();
		void makeMain();

		void setViewport(const spk::Rect2D &p_viewport);
		[[nodiscard]] const spk::Rect2D &viewport() const;

		void setPixelsPerUnit(const spk::Vector2 &p_pixelsPerUnit);
		[[nodiscard]] const spk::Vector2 &pixelsPerUnit() const;

		[[nodiscard]] const spk::Matrix4x4 &projectionMatrix() const;
		[[nodiscard]] spk::Matrix4x4 viewMatrix() const;
	};
}
