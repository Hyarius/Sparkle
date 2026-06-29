#include "structures/graphics/opengl/spk_opengl_framebuffer_object.hpp"

#include "structures/graphics/opengl/spk_opengl_texture.hpp"
#include "structures/graphics/rendering/context/spk_render_context.hpp"
#include "structures/graphics/spk_framebuffer_object.hpp"

namespace spk
{
	namespace OpenGL
	{
		FrameBufferObject::FrameBufferObject(const spk::FrameBufferObject &p_owner, const spk::RenderContext &p_context)
		{
			const spk::Vector2UInt &size = p_owner.size();
			if (size.x == 0 || size.y == 0)
			{
				return;
			}

			const GLuint colorTextureId = p_owner.colorAttachment().gpu(p_context).id();
			if (colorTextureId == 0)
			{
				return;
			}

			glGenFramebuffers(1, &_framebufferId);
			glBindFramebuffer(GL_FRAMEBUFFER, _framebufferId);

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTextureId, 0);

			if (p_owner.hasDepth() == true)
			{
				glGenRenderbuffers(1, &_depthStencilRenderbuffer);
				glBindRenderbuffer(GL_RENDERBUFFER, _depthStencilRenderbuffer);
				glRenderbufferStorage(
					GL_RENDERBUFFER,
					GL_DEPTH24_STENCIL8,
					static_cast<GLsizei>(size.x),
					static_cast<GLsizei>(size.y));
				glBindRenderbuffer(GL_RENDERBUFFER, 0);

				glFramebufferRenderbuffer(
					GL_FRAMEBUFFER,
					GL_DEPTH_STENCIL_ATTACHMENT,
					GL_RENDERBUFFER,
					_depthStencilRenderbuffer);
			}

			_complete = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		FrameBufferObject::~FrameBufferObject()
		{
			if (_ownsCurrentContext() == false)
			{
				return;
			}

			if (_framebufferId != 0)
			{
				glDeleteFramebuffers(1, &_framebufferId);
			}

			if (_depthStencilRenderbuffer != 0)
			{
				glDeleteRenderbuffers(1, &_depthStencilRenderbuffer);
			}
		}

		GLuint FrameBufferObject::id() const noexcept
		{
			return _framebufferId;
		}

		bool FrameBufferObject::isComplete() const noexcept
		{
			return _complete;
		}

		void FrameBufferObject::bind() const
		{
			glBindFramebuffer(GL_FRAMEBUFFER, _framebufferId);
		}

		void FrameBufferObject::bindDefault()
		{
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
	}
}
