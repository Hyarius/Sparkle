#pragma once

#include <cstdint>
#include <memory>

namespace spk
{
	class RenderContext;

	template <typename TGpuObject>
	class CachedOpenGLObjectCollection;

	namespace OpenGL
	{
		class Object
		{
			template <typename TGpuObject>
			friend class spk::CachedOpenGLObjectCollection;

		private:
			std::uint64_t _contextId = 0;
			std::uint64_t _version = 0;
			std::uint64_t _contentVersion = 0;

		protected:
			Object();

			[[nodiscard]] bool _ownsCurrentContext() const noexcept;

		public:
			virtual ~Object() = default;

			Object(const Object&) = delete;
			Object& operator=(const Object&) = delete;
			Object(Object&&) = delete;
			Object& operator=(Object&&) = delete;

			[[nodiscard]] std::uint64_t contextId() const noexcept;
			[[nodiscard]] std::uint64_t version() const noexcept;
			[[nodiscard]] std::uint64_t contentVersion() const noexcept;
		};

		// RenderContext accessors usable from headers that must not include
		// spk_opengl_render_context.hpp (it drags <Windows.h> in).
		[[nodiscard]] std::uint64_t contextIdOf(const spk::RenderContext& p_context) noexcept;
		[[nodiscard]] bool isContextCurrent(const spk::RenderContext& p_context) noexcept;
		[[nodiscard]] bool isContextAlive(std::uint64_t p_contextId) noexcept;
		[[nodiscard]] std::uint64_t contextDeathGeneration() noexcept;

		void releaseObject(std::unique_ptr<Object> p_object);
	}
}
