#pragma once

#include <GL/glew.h>

#include "structures/graphics/opengl/spk_opengl_object.hpp"

namespace spk
{
	class FrameBufferObject;
	class RenderContext;

	namespace OpenGL
	{
		class FrameBufferObject : public Object
		{
		private:
			GLuint _framebufferId = 0;
			GLuint _depthStencilRenderbuffer = 0;
			bool _complete = false;

		public:
			FrameBufferObject(const spk::FrameBufferObject &p_owner, const spk::RenderContext &p_context);
			~FrameBufferObject() override;

			FrameBufferObject(const FrameBufferObject &) = delete;
			FrameBufferObject &operator=(const FrameBufferObject &) = delete;
			FrameBufferObject(FrameBufferObject &&) = delete;
			FrameBufferObject &operator=(FrameBufferObject &&) = delete;

			[[nodiscard]] GLuint id() const noexcept;
			[[nodiscard]] bool isComplete() const noexcept;

			void bind() const;
			static void bindDefault();
		};
	}
}
