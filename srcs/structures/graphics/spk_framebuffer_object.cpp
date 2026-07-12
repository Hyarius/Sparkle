#include "structures/graphics/spk_framebuffer_object.hpp"

#include <limits>
#include <memory>
#include <stdexcept>

#include "structures/graphics/rendering/context/spk_render_context.hpp"

namespace spk
{
	FrameBufferObject::Description FrameBufferObject::colorTarget(const spk::Vector2UInt &p_size)
	{
		return Description{
			.size = p_size,
			.color = ColorAttachmentDescription{.format = spk::Texture::Format::RGBA},
			.depth = DepthAttachmentDescription{
				.storage = AttachmentStorage::RenderBuffer,
				.format = spk::Texture::Format::Depth24Stencil8}};
	}

	FrameBufferObject::Description FrameBufferObject::depthTextureTarget(
		const spk::Vector2UInt &p_size,
		spk::Texture::Format p_format)
	{
		return Description{
			.size = p_size,
			.color = std::nullopt,
			.depth = DepthAttachmentDescription{
				.storage = AttachmentStorage::Texture,
				.format = p_format}};
	}

	void FrameBufferObject::_validateDescription(const Description &p_description)
	{
		if (p_description.color.has_value() == false && p_description.depth.has_value() == false)
		{
			throw std::invalid_argument("A framebuffer description requires at least one attachment");
		}

		constexpr auto maxGLSize = static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max());
		if (static_cast<std::uint64_t>(p_description.size.x) > maxGLSize ||
			static_cast<std::uint64_t>(p_description.size.y) > maxGLSize)
		{
			throw std::invalid_argument("Framebuffer dimensions exceed the OpenGL GLsizei range");
		}

		if (p_description.color.has_value() &&
			spk::Texture::isColorFormat(p_description.color->format) == false)
		{
			throw std::invalid_argument("A framebuffer color attachment requires a color texture format");
		}

		if (p_description.depth.has_value() == false)
		{
			return;
		}

		const DepthAttachmentDescription &depth = *p_description.depth;
		if (depth.storage == AttachmentStorage::Texture)
		{
			if (spk::Texture::isDepthFormat(depth.format) == false)
			{
				throw std::invalid_argument(
					"Texture-backed depth attachments support Depth24 and Depth32F only");
			}
		}
		else if (depth.storage == AttachmentStorage::RenderBuffer)
		{
			if (depth.format != spk::Texture::Format::Depth24Stencil8)
			{
				throw std::invalid_argument(
					"Renderbuffer-backed depth attachments support Depth24Stencil8 only");
			}
		}
		else
		{
			throw std::invalid_argument("Unknown framebuffer depth attachment storage kind");
		}
	}

	FrameBufferObject::FrameBufferObject(const Description &p_description) :
		_description(p_description)
	{
		_validateDescription(_description);

		if (_description.color.has_value())
		{
			_colorAttachment.emplace();
		}
		if (_description.depth.has_value() && _description.depth->storage == AttachmentStorage::Texture)
		{
			_depthAttachment.emplace();
		}

		_applySize(_description.size);
		requestSynchronization();
	}

	FrameBufferObject::FrameBufferObject(const spk::Vector2UInt &p_size, bool p_withDepth) :
		FrameBufferObject([&]() {
			Description result = colorTarget(p_size);
			if (p_withDepth == false)
			{
				result.depth = std::nullopt;
			}
			return result;
		}())
	{
	}

	void FrameBufferObject::_applySize(const spk::Vector2UInt &p_size)
	{
		const spk::Rect2D rect({0, 0}, p_size);

		if (_colorAttachment.has_value())
		{
			_colorAttachment->allocateRenderTarget(
				p_size,
				_description.color->format,
				spk::Texture::Filtering::Linear,
				spk::Texture::Wrap::ClampToEdge);
		}
		if (_depthAttachment.has_value())
		{
			_depthAttachment->allocateRenderTarget(
				p_size,
				_description.depth->format,
				spk::Texture::Filtering::Nearest,
				spk::Texture::Wrap::ClampToBorder);
		}

		_description.size = p_size;
		_surfaceState.setRect(rect);
		_viewport.setGeometry(rect);
		_viewport.setScissor(rect);
	}

	void FrameBufferObject::_synchronize() const
	{
		spk::RenderContext *context = spk::RenderContext::current();
		if (context != nullptr && context->supportsOpenGLCommands() == true)
		{
			(void)gpu(*context);
		}
	}

	void FrameBufferObject::resize(const spk::Vector2UInt &p_size)
	{
		if (p_size == _description.size)
		{
			return;
		}

		Description resizedDescription = _description;
		resizedDescription.size = p_size;
		_validateDescription(resizedDescription);

		_applySize(p_size);
		_version.fetch_add(1);
		requestSynchronization();
	}

	const FrameBufferObject::Description &FrameBufferObject::description() const noexcept
	{
		return _description;
	}

	const spk::Vector2UInt &FrameBufferObject::size() const noexcept
	{
		return _description.size;
	}

	bool FrameBufferObject::hasColorAttachment() const noexcept
	{
		return _description.color.has_value();
	}

	bool FrameBufferObject::hasDepthAttachment() const noexcept
	{
		return _description.depth.has_value();
	}

	bool FrameBufferObject::hasSampleableDepth() const noexcept
	{
		return _depthAttachment.has_value();
	}

	const spk::Texture *FrameBufferObject::tryColorAttachment() const noexcept
	{
		return _colorAttachment.has_value() ? &*_colorAttachment : nullptr;
	}

	const spk::Texture *FrameBufferObject::tryDepthAttachment() const noexcept
	{
		return _depthAttachment.has_value() ? &*_depthAttachment : nullptr;
	}

	bool FrameBufferObject::hasDepth() const noexcept
	{
		return hasDepthAttachment();
	}

	const spk::SurfaceState &FrameBufferObject::surfaceState() const noexcept
	{
		return _surfaceState;
	}

	const spk::Viewport &FrameBufferObject::viewport() const noexcept
	{
		return _viewport;
	}

	const spk::Texture &FrameBufferObject::colorAttachment() const
	{
		if (_colorAttachment.has_value() == false)
		{
			throw std::runtime_error("Framebuffer has no texture-backed color attachment");
		}
		return *_colorAttachment;
	}

	const spk::Texture &FrameBufferObject::depthAttachment() const
	{
		if (_depthAttachment.has_value() == false)
		{
			throw std::runtime_error("Framebuffer has no texture-backed depth attachment");
		}
		return *_depthAttachment;
	}

	std::uint64_t FrameBufferObject::version() const noexcept
	{
		return _version.load();
	}

	spk::OpenGL::FrameBufferObject &FrameBufferObject::gpu(const spk::RenderContext &p_context) const
	{
		return _gpu.resolve(
			p_context,
			version(),
			[this, &p_context]() {
				return std::make_unique<spk::OpenGL::FrameBufferObject>(*this, p_context);
			});
	}

	bool FrameBufferObject::hasGpu(const spk::RenderContext &p_context) const noexcept
	{
		const spk::OpenGL::FrameBufferObject *object = _gpu.find(p_context);
		return object != nullptr && object->version() == version();
	}
}
