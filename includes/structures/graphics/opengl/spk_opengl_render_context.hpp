#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <vector>

#include "structures/graphics/opengl/spk_opengl_object.hpp"

#include <Windows.h>

#include "structures/math/spk_rect_2d.hpp"
#include "structures/system/device/window/spk_surface_state.hpp"
#include "structures/system/win32/spk_winapi_frame.hpp"

namespace spk
{
	namespace OpenGL
	{
		class Buffer;
		class Program;
		class VertexArray;
	}

	class RenderContext
	{
	private:
		static inline std::atomic<std::uint64_t> s_nextId = 1;
		static inline std::atomic<std::uint64_t> s_deathGeneration = 0;
		static inline thread_local RenderContext *s_current = nullptr;

		// Monotonic, never reused: a cached {contextId, pointer} pair whose context
		// died can never be revalidated, so a dangling pointer is unreachable.
		const std::uint64_t _id = s_nextId.fetch_add(1);

		std::shared_ptr<SurfaceState> _surfaceState;
		bool _valid = true;

		HWND _windowHandle = nullptr;
		HDC _deviceContext = nullptr;
		HGLRC _renderContext = nullptr;

		// GPU objects whose handle died while this context was alive but not
		// current: they can only be glDelete*'d while this context is current.
		std::mutex _releaseQueueMutex;
		std::vector<std::unique_ptr<spk::OpenGL::Object>> _releaseQueue;

		struct BindingCache
		{
			static constexpr std::size_t TrackedUniformBindingPoints = 8;

			std::optional<const spk::OpenGL::Program *> program;
			std::optional<const spk::OpenGL::VertexArray *> vertexArray;
			std::array<std::optional<const spk::OpenGL::Buffer *>, TrackedUniformBindingPoints> uniformBuffers{};
		};
		mutable BindingCache _bindingCache;

		void _registerSelf();
		void _unregisterSelf();

	protected:
		explicit RenderContext(std::shared_ptr<SurfaceState> p_surfaceState);

	public:
		explicit RenderContext(spk::Frame &p_frame);
		virtual ~RenderContext();

		[[nodiscard]] static RenderContext *current() noexcept;
		[[nodiscard]] static RenderContext *fromId(std::uint64_t p_id) noexcept;
		[[nodiscard]] static std::uint64_t deathGeneration() noexcept;

		[[nodiscard]] std::uint64_t id() const noexcept;

		void scheduleRelease(std::unique_ptr<spk::OpenGL::Object> p_object);
		void flushReleaseQueue();

		[[nodiscard]] bool isProgramActive(const spk::OpenGL::Program *p_program) const noexcept;
		void setActiveProgram(const spk::OpenGL::Program *p_program) const noexcept;

		[[nodiscard]] bool isVertexArrayActive(const spk::OpenGL::VertexArray *p_vertexArray) const noexcept;
		void setActiveVertexArray(const spk::OpenGL::VertexArray *p_vertexArray) const noexcept;

		[[nodiscard]] bool isUniformBufferBaseActive(
			std::uint32_t p_bindingPoint,
			const spk::OpenGL::Buffer *p_buffer) const noexcept;
		void setActiveUniformBufferBase(
			std::uint32_t p_bindingPoint,
			const spk::OpenGL::Buffer *p_buffer) const noexcept;

		void onProgramDeleted(const spk::OpenGL::Program &p_program) const noexcept;
		void onVertexArrayDeleted(const spk::OpenGL::VertexArray &p_vertexArray) const noexcept;
		void onBufferDeleted(const spk::OpenGL::Buffer &p_buffer) const noexcept;
		void resetBindingCache() const noexcept;

		[[nodiscard]] std::shared_ptr<SurfaceState> surfaceState() const;
		virtual void invalidate();
		[[nodiscard]] virtual bool isValid() const;
		[[nodiscard]] virtual bool supportsOpenGLCommands() const;

		virtual void makeCurrent();
		virtual void present();
		virtual void setVSync(bool p_enabled);
		virtual void notifyResize(const spk::Rect2D &p_rect);
	};
}
