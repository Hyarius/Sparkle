#include "structures/graphics/spk_framebuffer_object.hpp"

#include <memory>

#include "structures/graphics/rendering/context/spk_render_context.hpp"

namespace spk
{
	FrameBufferObject::FrameBufferObject(const spk::Vector2UInt &p_size, bool p_withDepth) :
		_hasDepth(p_withDepth)
	{
		_applySize(p_size);
		requestSynchronization();
	}

	void FrameBufferObject::_applySize(const spk::Vector2UInt &p_size)
	{
		const spk::Rect2D rect({0, 0}, p_size);

		_surfaceState.setRect(rect);
		_viewport.setGeometry(rect);
		_viewport.setScissor(rect);

		if (p_size.x > 0 && p_size.y > 0)
		{
			_colorAttachment.allocateRenderTarget(p_size);
		}
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
		if (p_size == _surfaceState.rect().size)
		{
			return;
		}

		_applySize(p_size);
		_version.fetch_add(1);
		requestSynchronization();
	}

	const spk::Vector2UInt &FrameBufferObject::size() const noexcept
	{
		return _surfaceState.rect().size;
	}

	bool FrameBufferObject::hasDepth() const noexcept
	{
		return _hasDepth;
	}

	const spk::SurfaceState &FrameBufferObject::surfaceState() const noexcept
	{
		return _surfaceState;
	}

	const spk::Viewport &FrameBufferObject::viewport() const noexcept
	{
		return _viewport;
	}

	const spk::Texture &FrameBufferObject::colorAttachment() const noexcept
	{
		return _colorAttachment;
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
