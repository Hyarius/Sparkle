#pragma once

#include <cstddef>
#include <string>

#include <GL/glew.h>

#include "structures/graphics/opengl/spk_opengl_object.hpp"
#include "structures/graphics/spk_primitive.hpp"

namespace spk
{
	namespace OpenGL
	{
		class Program : public Object
		{
		private:
			GLuint _id = 0;

			static GLuint _compileShader(GLenum p_type, const std::string &p_source);

		public:
			Program(const std::string &p_vertexShaderSource, const std::string &p_fragmentShaderSource);
			~Program();

			Program(const Program &) = delete;
			Program &operator=(const Program &) = delete;
			Program(Program &&) noexcept = delete;
			Program &operator=(Program &&) noexcept = delete;

			[[nodiscard]] GLuint id() const noexcept;

			void activate() const;
			void deactivate() const;

			void renderRaw(spk::Primitive p_primitive, std::size_t p_firstVertex, std::size_t p_vertexCount) const;
			void render(spk::Primitive p_primitive, std::size_t p_firstIndex, std::size_t p_indexCount) const;
		};
	}
}
