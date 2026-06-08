#include "structures/graphics/opengl/spk_opengl_render_context.hpp"

#ifdef _WIN32

#include <stdexcept>

#include <GL/glew.h>

#include "utils/spk_winapi_helpers.hpp"

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
			spk::throwLastError("ChoosePixelFormat");
		}

		if (SetPixelFormat(p_deviceContext, pixelFormat, &descriptor) == FALSE)
		{
			spk::throwLastError("SetPixelFormat");
		}
	}
}

namespace spk
{
	RenderContext::RenderContext(std::shared_ptr<SurfaceState> p_surfaceState) :
		_surfaceState(std::move(p_surfaceState)),
		_valid(false)
	{
	}

	RenderContext::RenderContext(spk::Frame& p_frame) :
		_surfaceState(p_frame.surfaceState()),
		_valid(true),
		_windowHandle(p_frame.handle()),
		_deviceContext(GetDC(_windowHandle))
	{
		if (_surfaceState == nullptr)
		{
			throw std::invalid_argument("spk::RenderContext requires a valid surface state");
		}
		if (_windowHandle == nullptr)
		{
			throw std::runtime_error("spk::RenderContext requires a valid WinAPI frame window");
		}
		if (_deviceContext == nullptr)
		{
			spk::throwLastError("GetDC");
		}

		configurePixelFormat(_deviceContext);

		_renderContext = wglCreateContext(_deviceContext);
		if (_renderContext == nullptr)
		{
			spk::throwLastError("wglCreateContext");
		}

		makeCurrent();

		glewExperimental = GL_TRUE;
		const GLenum glewResult = glewInit();
		if (glewResult != GLEW_OK)
		{
			throw std::runtime_error(
				"spk::RenderContext failed to initialize GLEW: " +
				std::string(reinterpret_cast<const char*>(glewGetErrorString(glewResult))));
		}

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glBlendFuncSeparate(
			GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glFrontFace(GL_CCW);

		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_TRUE);
		glClearDepth(1.0f);
		glDepthFunc(GL_LEQUAL);

		glDisable(GL_STENCIL_TEST);
		glStencilFunc(GL_ALWAYS, 0, 0xFF);
		glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
		glStencilMask(0xFF);

		glEnable(GL_SCISSOR_TEST);
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

	std::shared_ptr<SurfaceState> RenderContext::surfaceState() const
	{
		return _surfaceState;
	}

	void RenderContext::invalidate()
	{
		_valid = false;
		_windowHandle = nullptr;
		if (_surfaceState != nullptr)
		{
			_surfaceState->invalidate();
		}
	}

	bool RenderContext::isValid() const
	{
		return _valid &&
			   _windowHandle != nullptr &&
			   _deviceContext != nullptr &&
			   _renderContext != nullptr;
	}

	bool RenderContext::supportsOpenGLCommands() const
	{
		return _renderContext != nullptr;
	}

	void RenderContext::makeCurrent()
	{
		if (isValid() == false)
		{
			throw std::runtime_error("spk::RenderContext::makeCurrent called on an invalid context");
		}

		if (wglMakeCurrent(_deviceContext, _renderContext) == FALSE)
		{
			spk::throwLastError("wglMakeCurrent");
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
		surfaceState()->setRect(p_rect);

		if (isValid() == false)
		{
			return;
		}

		makeCurrent();
		glViewport(0, 0, static_cast<GLsizei>(p_rect.width()), static_cast<GLsizei>(p_rect.height()));
	}
}

#endif
