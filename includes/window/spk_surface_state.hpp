#pragma once

#include <atomic>

#include "math/spk_rect_2d.hpp"

namespace spk
{
	class ISurfaceState
	{
	private:
		std::atomic<bool> _isValid = true;
		spk::Rect2D _rect;

	public:
		virtual ~ISurfaceState();

		void invalidate();
		[[nodiscard]] bool isValid() const;

		void setRect(const spk::Rect2D& p_rect);
		[[nodiscard]] const spk::Rect2D& rect() const;
	};
}
