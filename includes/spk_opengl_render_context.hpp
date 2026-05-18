#pragma once

#ifdef _WIN32

#include <Windows.h>

#include "spk_render_context.hpp"
#include "spk_winapi_frame.hpp"

namespace spk::OpenGL
{
	class RenderContext : public spk::IRenderContext
	{
	private:
		HWND _windowHandle = nullptr;
		HDC _deviceContext = nullptr;
		HGLRC _renderContext = nullptr;

	public:
		explicit RenderContext(spk::WinAPI::Frame& p_frame);
		~RenderContext() override;

		void invalidate() override;
		[[nodiscard]] bool isValid() const override;

		void makeCurrent() override;
		void present() override;
		void setVSync(bool p_enabled) override;
		void notifyResize(const spk::Rect2D& p_rect) override;
	};
}

#endif
