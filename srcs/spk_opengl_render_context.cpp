#include "spk_opengl_render_context.hpp"

#if defined(_WIN32) && defined(SPARKLE_GPU_BACKEND_OPENGL)

#include <stdexcept>

#include <GL/glew.h>

#include "spk_winapi_helpers.hpp"

namespace
{
	using WGLSwapIntervalEXTPtr = BOOL(WINAPI*)(int);

	void configurePixelFormat(HDC p_deviceContext)
	{
		PIXELFORMATDESCRIPTOR descriptor{};
		descriptor.nSize = sizeof(descriptor);
		descriptor.nVersion = 1;
		descriptor.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
		descriptor.iPixelType = PFD_TYPE_RGBA;
		descriptor.cColorBits = 32;
		descriptor.cDepthBits = 24;
		descriptor.cStencilBits = 8;
		descriptor.iLayerType = PFD_MAIN_PLANE;

		if (GetPixelFormat(p_deviceContext) != 0)
		{
			return;
		}

		const int pixelFormat = ChoosePixelFormat(p_deviceContext, &descriptor);
		if (pixelFormat == 0)
		{
			spk::WinAPI::throwLastError("ChoosePixelFormat");
		}

		if (SetPixelFormat(p_deviceContext, pixelFormat, &descriptor) == FALSE)
		{
			spk::WinAPI::throwLastError("SetPixelFormat");
		}
	}
}

namespace spk::OpenGL
{
	RenderContext::RenderContext(spk::WinAPI::Frame& p_frame) :
		spk::IRenderContext(p_frame.surfaceState()),
		_windowHandle(p_frame.handle()),
		_deviceContext(GetDC(_windowHandle))
	{
		if (_windowHandle == nullptr)
		{
			throw std::runtime_error("spk::OpenGL::RenderContext requires a valid WinAPI frame window");
		}
		if (_deviceContext == nullptr)
		{
			spk::WinAPI::throwLastError("GetDC");
		}

		configurePixelFormat(_deviceContext);

		_renderContext = wglCreateContext(_deviceContext);
		if (_renderContext == nullptr)
		{
			spk::WinAPI::throwLastError("wglCreateContext");
		}

		makeCurrent();

		glewExperimental = GL_TRUE;
		const GLenum glewResult = glewInit();
		if (glewResult != GLEW_OK)
		{
			throw std::runtime_error(
				"spk::OpenGL::RenderContext failed to initialize GLEW: " +
				std::string(reinterpret_cast<const char*>(glewGetErrorString(glewResult))));
		}
	}

	RenderContext::~RenderContext()
	{
		if (_renderContext != nullptr)
		{
			if (wglGetCurrentContext() == _renderContext)
			{
				wglMakeCurrent(nullptr, nullptr);
			}
			wglDeleteContext(_renderContext);
			_renderContext = nullptr;
		}

		if (_windowHandle != nullptr && _deviceContext != nullptr)
		{
			ReleaseDC(_windowHandle, _deviceContext);
			_deviceContext = nullptr;
		}
	}

	void RenderContext::invalidate()
	{
		IRenderContext::invalidate();
		_windowHandle = nullptr;
	}

	bool RenderContext::isValid() const
	{
		return IRenderContext::isValid() == true &&
			   _windowHandle != nullptr &&
			   _deviceContext != nullptr &&
			   _renderContext != nullptr;
	}

	void RenderContext::makeCurrent()
	{
		if (isValid() == false)
		{
			throw std::runtime_error("spk::OpenGL::RenderContext::makeCurrent called on an invalid context");
		}

		if (wglMakeCurrent(_deviceContext, _renderContext) == FALSE)
		{
			spk::WinAPI::throwLastError("wglMakeCurrent");
		}
	}

	void RenderContext::present()
	{
		if (isValid() == false)
		{
			return;
		}

		SwapBuffers(_deviceContext);
	}

	void RenderContext::setVSync(bool p_enabled)
	{
		if (isValid() == false)
		{
			return;
		}

		auto* swapInterval = reinterpret_cast<WGLSwapIntervalEXTPtr>(wglGetProcAddress("wglSwapIntervalEXT"));
		if (swapInterval != nullptr)
		{
			swapInterval(p_enabled == true ? 1 : 0);
		}
	}

	void RenderContext::notifyResize(const spk::Rect2D& p_rect)
	{
		if (isValid() == false)
		{
			return;
		}

		makeCurrent();
		glViewport(0, 0, static_cast<GLsizei>(p_rect.width()), static_cast<GLsizei>(p_rect.height()));
	}
}

#endif
