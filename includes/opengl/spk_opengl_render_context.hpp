#pragma once

#ifdef _WIN32

#include <memory>

#include <Windows.h>

#include "math/spk_rect_2d.hpp"
#include "window/spk_surface_state.hpp"
#include "winapi/spk_winapi_frame.hpp"

namespace spk
{
	class RenderContext
	{
	private:
		std::shared_ptr<SurfaceState> _surfaceState;
		bool _valid = true;

		HWND _windowHandle = nullptr;
		HDC _deviceContext = nullptr;
		HGLRC _renderContext = nullptr;

	protected:
		explicit RenderContext(std::shared_ptr<SurfaceState> p_surfaceState);

	public:
		explicit RenderContext(spk::Frame& p_frame);
		virtual ~RenderContext();

		[[nodiscard]] std::shared_ptr<SurfaceState> surfaceState() const;
		virtual void invalidate();
		[[nodiscard]] virtual bool isValid() const;
		[[nodiscard]] virtual bool supportsOpenGLCommands() const;

		virtual void makeCurrent();
		virtual void present();
		virtual void setVSync(bool p_enabled);
		virtual void notifyResize(const spk::Rect2D& p_rect);
	};
}

#endif
