#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>

#include <GL/glew.h>

#include "structures/design_pattern/spk_synchronizable_trait.hpp"
#include "structures/graphics/opengl/spk_cached_opengl_object_collection.hpp"
#include "structures/graphics/opengl/spk_opengl_program.hpp"
#include "structures/graphics/spk_primitive.hpp"

namespace spk
{
	class RenderContext;

	class Program : public spk::SynchronizableTrait
	{
	private:
		mutable std::mutex _sourceMutex;
		std::string _vertexShaderSource;
		std::string _fragmentShaderSource;
		std::atomic<std::uint64_t> _version = 0;

		mutable spk::CachedOpenGLObjectCollection<spk::OpenGL::Program> _gpu;

	protected:
		void _synchronize() const override;

	public:
		Program() = default;
		Program(std::string p_vertexShaderSource, std::string p_fragmentShaderSource);

		Program(const Program &) = delete;
		Program &operator=(const Program &) = delete;
		Program(Program &&) noexcept = delete;
		Program &operator=(Program &&) noexcept = delete;

		[[nodiscard]] std::uint64_t version() const noexcept;

		void setSources(std::string p_vertexShaderSource, std::string p_fragmentShaderSource);
		[[nodiscard]] std::string vertexShaderSource() const;
		[[nodiscard]] std::string fragmentShaderSource() const;

		// Resolves (compiling if needed) this program's GPU copy for p_context.
		// p_context must be the current context.
		[[nodiscard]] spk::OpenGL::Program &gpu(const spk::RenderContext &p_context) const;
		[[nodiscard]] bool hasGpu(const spk::RenderContext &p_context) const noexcept;

		// Resolved against the current context (SynchronizableTrait / uniform glue).
		[[nodiscard]] GLuint id() const;
		[[nodiscard]] bool isLinked() const noexcept;

		void activate(const spk::RenderContext &p_context) const;
		void deactivate() const;

		void renderRaw(const spk::RenderContext &p_context, spk::Primitive p_primitive, std::size_t p_firstVertex, std::size_t p_vertexCount) const;
		void render(const spk::RenderContext &p_context, spk::Primitive p_primitive, std::size_t p_firstIndex, std::size_t p_indexCount) const;
	};
}
