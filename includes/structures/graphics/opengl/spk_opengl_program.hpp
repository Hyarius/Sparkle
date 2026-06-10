#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string>
#include <mutex>

#include <GL/glew.h>

#include "type/spk_opengl_primitive.hpp"
#include "structures/design_pattern/spk_synchronizable_trait.hpp"

namespace spk
{
	namespace OpenGL
	{
		// GPU-side compiled program. Owned by the RenderContext that compiled it and
		// only usable while that context is current.
		class Program
		{
		private:
			GLuint _id = 0;

			static GLuint _compileShader(GLenum p_type, const std::string& p_source);

		public:
			Program(const std::string& p_vertexShaderSource, const std::string& p_fragmentShaderSource);
			~Program();

			Program(const Program&) = delete;
			Program& operator=(const Program&) = delete;
			Program(Program&&) noexcept = delete;
			Program& operator=(Program&&) noexcept = delete;

			[[nodiscard]] GLuint id() const noexcept;

			void activate() const;
			void deactivate() const;

			void renderRaw(spk::Primitive p_primitive, std::size_t p_firstVertex, std::size_t p_vertexCount) const;
			void render(spk::Primitive p_primitive, std::size_t p_firstIndex, std::size_t p_indexCount) const;
		};
	}

	// Context-free description of a program (shader sources only). Each RenderContext
	// compiles and caches its own spk::OpenGL::Program for a given handle, so a handle
	// can be shared across render command instances and outlive any context.
	class Program : public spk::SynchronizableTrait
	{
	private:
		static inline std::atomic<std::uint64_t> s_nextKey = 1;

		std::uint64_t _key = s_nextKey.fetch_add(1);
		mutable std::mutex _sourceMutex;
		std::string _vertexShaderSource;
		std::string _fragmentShaderSource;
		std::uint64_t _version = 0;

	protected:
		void _synchronize() const override;

	public:
		Program() = default;
		Program(std::string p_vertexShaderSource, std::string p_fragmentShaderSource);

		Program(const Program&) = delete;
		Program& operator=(const Program&) = delete;
		Program(Program&&) noexcept = delete;
		Program& operator=(Program&&) noexcept = delete;

		[[nodiscard]] std::uint64_t key() const noexcept;
		[[nodiscard]] std::uint64_t version() const;

		void setSources(std::string p_vertexShaderSource, std::string p_fragmentShaderSource);
		[[nodiscard]] std::string vertexShaderSource() const;
		[[nodiscard]] std::string fragmentShaderSource() const;

		[[nodiscard]] GLuint id() const;
		[[nodiscard]] bool isLinked() const noexcept;

		void activate();
		void deactivate() const;

		void renderRaw(spk::Primitive p_primitive, std::size_t p_firstVertex, std::size_t p_vertexCount);
		void render(spk::Primitive p_primitive, std::size_t p_firstIndex, std::size_t p_indexCount);
	};
}
