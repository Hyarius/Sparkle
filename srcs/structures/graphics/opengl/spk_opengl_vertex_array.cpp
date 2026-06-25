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
			// _id is always non-zero after construction (glGenVertexArrays on a live
			// context, no move/reset path), so only the owning-context guard remains.
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
