#pragma once

#include <memory>

#include "spk_rect_2d.hpp"

namespace spk
{
	class IFrame;

	class IRenderContext
	{
	public:
		class Backend
		{
		public:
			virtual ~Backend();

			virtual std::unique_ptr<IRenderContext> createRenderContext(IFrame& p_frame) = 0;
		};

	public:
		virtual ~IRenderContext();

		virtual void makeCurrent() = 0;
		virtual void present() = 0;
		virtual void setVSync(bool p_enabled) = 0;
		virtual void notifyResize(const spk::Rect2D& p_rect) = 0;
	};
}
