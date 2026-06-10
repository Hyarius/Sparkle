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
			// Compiled programs belong to this context and must be released while it
			// is current; a program id would name an unrelated object in another context.
			HDC previousDeviceContext = wglGetCurrentDC();
			HGLRC previousRenderContext = wglGetCurrentContext();

			if (wglMakeCurrent(_deviceContext, _renderContext) == FALSE)
			{
				// Could not bind this context: unbind entirely so the program
				// destructors do not delete ids belonging to another context.
				wglMakeCurrent(nullptr, nullptr);
			}
			_compiledPrograms.clear();

			if (previousRenderContext != nullptr && previousRenderContext != _renderContext)
			{
				wglMakeCurrent(previousDeviceContext, previousRenderContext);
			}
			else
			{
				wglMakeCurrent(nullptr, nullptr);
			}

			wglDeleteContext(_renderContext);
			_renderContext = nullptr;
		}

		if (s_current == this)
		{
			s_current = nullptr;
		}

		if (_windowHandle != nullptr && _deviceContext != nullptr)
		{
			ReleaseDC(_windowHandle, _deviceContext);
			_deviceContext = nullptr;
		}
	}

	RenderContext* RenderContext::current() noexcept
	{
		return s_current;
	}

	spk::OpenGL::Program& RenderContext::compiledProgram(const spk::Program& p_program)
	{
		if (supportsOpenGLCommands() == false)
		{
			throw std::runtime_error("spk::RenderContext::compiledProgram called on a context without OpenGL support");
		}

		CompiledProgramEntry& entry = _compiledPrograms[p_program.key()];
		if (entry.program == nullptr || entry.version != p_program.version())
		{
			auto compiled = std::make_unique<spk::OpenGL::Program>(
				p_program.vertexShaderSource(),
				p_program.fragmentShaderSource());
			entry.program = std::move(compiled);
			entry.version = p_program.version();
		}

		return *entry.program;
	}

	bool RenderContext::hasCompiledProgram(const spk::Program& p_program) const noexcept
	{
		const auto it = _compiledPrograms.find(p_program.key());
		return it != _compiledPrograms.end() &&
			   it->second.program != nullptr &&
			   it->second.version == p_program.version();
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

		s_current = this;
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
