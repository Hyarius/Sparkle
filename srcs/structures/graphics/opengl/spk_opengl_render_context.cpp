#include "structures/graphics/opengl/spk_opengl_render_context.hpp"

#include <stdexcept>
#include <unordered_map>
#include <utility>

#include <GL/glew.h>

#include "utils/spk_winapi_helpers.hpp"

namespace
{
	using WGLSwapIntervalEXTPtr = BOOL(WINAPI *)(int);

	// Live-context registry: lets handle destructors route GPU objects to their
	// owning context (or detect it died). The storage intentionally survives
	// process teardown because static GPU handles may release after normal static
	// objects have already been destroyed.
	std::mutex &contextRegistryMutex()
	{
		static std::mutex *mutex = new std::mutex();
		return *mutex;
	}

	std::unordered_map<std::uint64_t, spk::RenderContext *> &contextRegistry()
	{
		static std::unordered_map<std::uint64_t, spk::RenderContext *> *registry =
			new std::unordered_map<std::uint64_t, spk::RenderContext *>();
		return *registry;
	}

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
	void RenderContext::_registerSelf()
	{
		std::scoped_lock lock(contextRegistryMutex());
		contextRegistry()[_id] = this;
	}

	void RenderContext::_unregisterSelf()
	{
		std::scoped_lock lock(contextRegistryMutex());
		contextRegistry().erase(_id);
	}

	RenderContext::RenderContext(std::shared_ptr<SurfaceState> p_surfaceState) :
		_surfaceState(std::move(p_surfaceState)),
		_valid(false)
	{
		_registerSelf();
	}

	RenderContext::RenderContext(spk::Frame &p_frame) :
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
				std::string(reinterpret_cast<const char *>(glewGetErrorString(glewResult))));
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

		// Registered last: a half-constructed context must not be reachable through
		// fromId() (the destructor does not run when the constructor throws).
		_registerSelf();
	}

	RenderContext::~RenderContext()
	{
		// Unregister first: handles releasing GPU objects from now on see this
		// context as dead and drop their wrappers without GL calls.
		_unregisterSelf();
		s_deathGeneration.fetch_add(1, std::memory_order_relaxed);

		if (_renderContext != nullptr)
		{
			// Queued GPU objects belong to this context and must be released while it
			// is current; their ids would name unrelated objects in another context.
			HDC previousDeviceContext = wglGetCurrentDC();
			HGLRC previousRenderContext = wglGetCurrentContext();

			RenderContext *previousCurrent = s_current;

			if (wglMakeCurrent(_deviceContext, _renderContext) == TRUE)
			{
				s_current = this;
				flushReleaseQueue();
			}

			if (previousRenderContext != nullptr && previousRenderContext != _renderContext)
			{
				wglMakeCurrent(previousDeviceContext, previousRenderContext);
				s_current = previousCurrent != this ? previousCurrent : nullptr;
			}
			else
			{
				wglMakeCurrent(nullptr, nullptr);
				s_current = nullptr;
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

	RenderContext *RenderContext::current() noexcept
	{
		return s_current;
	}

	RenderContext *RenderContext::fromId(std::uint64_t p_id) noexcept
	{
		std::scoped_lock lock(contextRegistryMutex());
		const auto it = contextRegistry().find(p_id);
		return it != contextRegistry().end() ? it->second : nullptr;
	}

	std::uint64_t RenderContext::deathGeneration() noexcept
	{
		return s_deathGeneration.load(std::memory_order_relaxed);
	}

	std::uint64_t RenderContext::id() const noexcept
	{
		return _id;
	}

	void RenderContext::scheduleRelease(std::unique_ptr<spk::OpenGL::Object> p_object)
	{
		if (p_object == nullptr)
		{
			return;
		}

		std::scoped_lock lock(_releaseQueueMutex);
		_releaseQueue.push_back(std::move(p_object));
	}

	void RenderContext::flushReleaseQueue()
	{
		std::vector<std::unique_ptr<spk::OpenGL::Object>> pending;
		{
			std::scoped_lock lock(_releaseQueueMutex);
			pending.swap(_releaseQueue);
		}
		// Destructors run GL calls: outside the lock, with this context current.
		pending.clear();
	}

	bool RenderContext::isProgramActive(const spk::OpenGL::Program *p_program) const noexcept
	{
		return _bindingCache.program.has_value() == true && _bindingCache.program.value() == p_program;
	}

	void RenderContext::setActiveProgram(const spk::OpenGL::Program *p_program) const noexcept
	{
		_bindingCache.program = p_program;
	}

	bool RenderContext::isVertexArrayActive(const spk::OpenGL::VertexArray *p_vertexArray) const noexcept
	{
		return _bindingCache.vertexArray.has_value() == true && _bindingCache.vertexArray.value() == p_vertexArray;
	}

	void RenderContext::setActiveVertexArray(const spk::OpenGL::VertexArray *p_vertexArray) const noexcept
	{
		_bindingCache.vertexArray = p_vertexArray;
	}

	bool RenderContext::isUniformBufferBaseActive(
		std::uint32_t p_bindingPoint,
		const spk::OpenGL::Buffer *p_buffer) const noexcept
	{
		return p_bindingPoint < BindingCache::TrackedUniformBindingPoints &&
			   _bindingCache.uniformBuffers[p_bindingPoint].has_value() == true &&
			   _bindingCache.uniformBuffers[p_bindingPoint].value() == p_buffer;
	}

	void RenderContext::setActiveUniformBufferBase(
		std::uint32_t p_bindingPoint,
		const spk::OpenGL::Buffer *p_buffer) const noexcept
	{
		if (p_bindingPoint >= BindingCache::TrackedUniformBindingPoints)
		{
			return;
		}

		_bindingCache.uniformBuffers[p_bindingPoint] = p_buffer;
	}

	void RenderContext::onProgramDeleted(const spk::OpenGL::Program &p_program) const noexcept
	{
		if (_bindingCache.program.has_value() == true && _bindingCache.program.value() == &p_program)
		{
			_bindingCache.program = nullptr;
		}
	}

	void RenderContext::onVertexArrayDeleted(const spk::OpenGL::VertexArray &p_vertexArray) const noexcept
	{
		if (_bindingCache.vertexArray.has_value() == true && _bindingCache.vertexArray.value() == &p_vertexArray)
		{
			// Deleting the bound vertex array reverts the binding to zero.
			_bindingCache.vertexArray = nullptr;
		}
	}

	void RenderContext::onBufferDeleted(const spk::OpenGL::Buffer &p_buffer) const noexcept
	{
		for (std::size_t i = 0; i < _bindingCache.uniformBuffers.size(); ++i)
		{
			if (_bindingCache.uniformBuffers[i].has_value() == true &&
				_bindingCache.uniformBuffers[i].value() == &p_buffer)
			{
				_bindingCache.uniformBuffers[i] = nullptr;
			}
		}
	}

	void RenderContext::resetBindingCache() const noexcept
	{
		_bindingCache = BindingCache{};
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

		// Another context (or raw GL between frames) may have changed bindings.
		resetBindingCache();

		flushReleaseQueue();
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

		auto *swapInterval = reinterpret_cast<WGLSwapIntervalEXTPtr>(wglGetProcAddress("wglSwapIntervalEXT"));
		if (swapInterval != nullptr)
		{
			swapInterval(p_enabled == true ? 1 : 0);
		}
	}

	void RenderContext::notifyResize(const spk::Rect2D &p_rect)
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
