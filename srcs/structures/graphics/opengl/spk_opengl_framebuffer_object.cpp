#include "structures/graphics/opengl/spk_opengl_framebuffer_object.hpp"

#include <sstream>
#include <stdexcept>

#include "structures/graphics/opengl/spk_opengl_texture.hpp"
#include "structures/graphics/rendering/context/spk_render_context.hpp"
#include "structures/graphics/spk_framebuffer_object.hpp"

namespace spk
{
	namespace OpenGL
	{
		namespace
		{
			void restoreBindings(GLint p_drawFramebuffer, GLint p_readFramebuffer, GLint p_renderbuffer)
			{
				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(p_drawFramebuffer));
				glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(p_readFramebuffer));
				glBindRenderbuffer(GL_RENDERBUFFER, static_cast<GLuint>(p_renderbuffer));
			}

			std::string failureDescription(
				const spk::FrameBufferObject &p_owner,
				GLenum p_status)
			{
				const auto &description = p_owner.description();
				std::ostringstream stream;
				stream << "OpenGL framebuffer is incomplete (status=0x" << std::hex << p_status << std::dec
					   << ", size=" << description.size.x << 'x' << description.size.y
					   << ", color=" << (description.color.has_value() ? "texture" : "none")
					   << ", depth=";
				if (description.depth.has_value() == false)
				{
					stream << "none";
				}
				else
				{
					stream << (description.depth->storage == spk::FrameBufferObject::AttachmentStorage::Texture ? "texture" : "renderbuffer");
				}
				stream << ')';
				return stream.str();
			}
		}

		FrameBufferObject::FrameBufferObject(const spk::FrameBufferObject &p_owner, const spk::RenderContext &p_context)
		{
			const spk::Vector2UInt &size = p_owner.size();
			if (size.x == 0 || size.y == 0)
			{
				return;
			}

			GLint previousDrawFramebuffer = 0;
			GLint previousReadFramebuffer = 0;
			GLint previousRenderbuffer = 0;
			glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previousDrawFramebuffer);
			glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previousReadFramebuffer);
			glGetIntegerv(GL_RENDERBUFFER_BINDING, &previousRenderbuffer);

			try
			{
				glGenFramebuffers(1, &_framebufferId);
				glBindFramebuffer(GL_FRAMEBUFFER, _framebufferId);

				if (const spk::Texture *color = p_owner.tryColorAttachment(); color != nullptr)
				{
					const GLuint textureId = color->gpu(p_context).id();
					if (textureId == 0)
					{
						throw std::runtime_error("Framebuffer color attachment failed to allocate an OpenGL texture");
					}
					glFramebufferTexture2D(
						GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureId, 0);
				}

				if (const spk::Texture *depth = p_owner.tryDepthAttachment(); depth != nullptr)
				{
					const GLuint textureId = depth->gpu(p_context).id();
					if (textureId == 0)
					{
						throw std::runtime_error("Framebuffer depth attachment failed to allocate an OpenGL texture");
					}
					glFramebufferTexture2D(
						GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, textureId, 0);
				}
				else if (p_owner.hasDepthAttachment())
				{
					glGenRenderbuffers(1, &_depthStencilRenderbuffer);
					glBindRenderbuffer(GL_RENDERBUFFER, _depthStencilRenderbuffer);
					glRenderbufferStorage(
						GL_RENDERBUFFER,
						GL_DEPTH24_STENCIL8,
						static_cast<GLsizei>(size.x),
						static_cast<GLsizei>(size.y));
					glFramebufferRenderbuffer(
						GL_FRAMEBUFFER,
						GL_DEPTH_STENCIL_ATTACHMENT,
						GL_RENDERBUFFER,
						_depthStencilRenderbuffer);
				}

				if (p_owner.hasColorAttachment())
				{
					glDrawBuffer(GL_COLOR_ATTACHMENT0);
					glReadBuffer(GL_COLOR_ATTACHMENT0);
				}
				else
				{
					glDrawBuffer(GL_NONE);
					glReadBuffer(GL_NONE);
				}

				_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
				_complete = _status == GL_FRAMEBUFFER_COMPLETE;
				if (_complete == false)
				{
					throw std::runtime_error(failureDescription(p_owner, _status));
				}

				restoreBindings(previousDrawFramebuffer, previousReadFramebuffer, previousRenderbuffer);
			} catch (...)
			{
				restoreBindings(previousDrawFramebuffer, previousReadFramebuffer, previousRenderbuffer);
				if (_depthStencilRenderbuffer != 0)
				{
					glDeleteRenderbuffers(1, &_depthStencilRenderbuffer);
					_depthStencilRenderbuffer = 0;
				}
				if (_framebufferId != 0)
				{
					glDeleteFramebuffers(1, &_framebufferId);
					_framebufferId = 0;
				}
				throw;
			}
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

		GLenum FrameBufferObject::status() const noexcept
		{
			return _status;
		}

		void FrameBufferObject::bind() const
		{
			if (_framebufferId == 0 || _complete == false)
			{
				throw std::runtime_error("Cannot bind an empty or incomplete OpenGL framebuffer");
			}
			glBindFramebuffer(GL_FRAMEBUFFER, _framebufferId);
		}

		void FrameBufferObject::bindDefault()
		{
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
	}
}
