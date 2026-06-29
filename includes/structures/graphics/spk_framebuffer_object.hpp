#pragma once

#include <atomic>
#include <cstdint>

#include "structures/design_pattern/spk_synchronizable_trait.hpp"
#include "structures/graphics/opengl/spk_cached_opengl_object_collection.hpp"
#include "structures/graphics/opengl/spk_opengl_framebuffer_object.hpp"
#include "structures/graphics/rendering/state/spk_viewport.hpp"
#include "structures/graphics/spk_texture.hpp"
#include "structures/math/spk_vector2.hpp"
#include "structures/system/device/window/spk_surface_state.hpp"

namespace spk
{
	class RenderContext;

	// Handle for an off-screen render target. Mirrors the window surface model:
	// it owns a SurfaceState (size + validity), a default Viewport sized to the
	// whole target, and a samplable color Texture. Its per-context GPU twin is a
	// spk::OpenGL::FrameBufferObject that attaches the color texture + a depth
	// renderbuffer.
	class FrameBufferObject : public spk::SynchronizableTrait
	{
	private:
		spk::SurfaceState _surfaceState;
		spk::Viewport _viewport;
		spk::Texture _colorAttachment;
		bool _hasDepth = true;
		std::atomic<std::uint64_t> _version = 1;

		mutable spk::CachedOpenGLObjectCollection<spk::OpenGL::FrameBufferObject> _gpu;

		void _applySize(const spk::Vector2UInt &p_size);

	protected:
		void _synchronize() const override;

	public:
		explicit FrameBufferObject(const spk::Vector2UInt &p_size = {0, 0}, bool p_withDepth = true);

		FrameBufferObject(const FrameBufferObject &) = delete;
		FrameBufferObject &operator=(const FrameBufferObject &) = delete;
		FrameBufferObject(FrameBufferObject &&) noexcept = delete;
		FrameBufferObject &operator=(FrameBufferObject &&) noexcept = delete;

		void resize(const spk::Vector2UInt &p_size);

		[[nodiscard]] const spk::Vector2UInt &size() const noexcept;
		[[nodiscard]] bool hasDepth() const noexcept;

		[[nodiscard]] const spk::SurfaceState &surfaceState() const noexcept;
		[[nodiscard]] const spk::Viewport &viewport() const noexcept;
		[[nodiscard]] const spk::Texture &colorAttachment() const noexcept;

		[[nodiscard]] std::uint64_t version() const noexcept;

		[[nodiscard]] spk::OpenGL::FrameBufferObject &gpu(const spk::RenderContext &p_context) const;
		[[nodiscard]] bool hasGpu(const spk::RenderContext &p_context) const noexcept;
	};
}
