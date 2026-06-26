#include "structures/graphics/opengl/spk_opengl_vertex_array.hpp"

namespace spk
{
	namespace OpenGL
	{
		VertexArray::VertexArray()
		{
			glGenVertexArrays(1, &_id);
		}

		VertexArray::~VertexArray()
		{
			if (_ownsCurrentContext() == true)
			{
				glDeleteVertexArrays(1, &_id);
				notifyVertexArrayDeleted(*this);
			}
		}

		GLuint VertexArray::id() const noexcept
		{
			return _id;
		}
	}
}
