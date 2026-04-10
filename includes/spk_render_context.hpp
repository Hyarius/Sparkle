#pragma once

#include "spk_rect_2d.hpp"

namespace spk
{
	class IRenderContext
	{
	public:
		virtual ~IRenderContext();

		virtual void makeCurrent() = 0;
		virtual void present() = 0;
		virtual void setVSync(bool p_enabled) = 0;
		virtual void notifyResize(const spk::Rect2D& p_rect) = 0;
	};
}
