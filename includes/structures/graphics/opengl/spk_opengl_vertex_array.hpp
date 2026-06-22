#pragma once

#include <GL/glew.h>

#include "structures/graphics/opengl/spk_opengl_object.hpp"

namespace spk
{
	namespace OpenGL
	{
		class VertexArray : public Object
		{
		private:
			GLuint _id = 0;

		public:
			VertexArray();
			~VertexArray();

			VertexArray(const VertexArray &) = delete;
			VertexArray &operator=(const VertexArray &) = delete;
			VertexArray(VertexArray &&) = delete;
			VertexArray &operator=(VertexArray &&) = delete;

			[[nodiscard]] GLuint id() const noexcept;
		};
	}
}
