#pragma once

#include <atomic>
#include <cstdint>
#include <optional>

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

	// Logical off-screen render target. Texture attachments are sampleable;
	// renderbuffer attachments are GPU-only rasterization storage. Each render
	// context resolves independent OpenGL objects from this shared description.
	class FrameBufferObject : public spk::SynchronizableTrait
	{
	public:
		enum class AttachmentStorage
		{
			Texture,
			RenderBuffer
		};

		struct ColorAttachmentDescription
		{
			spk::Texture::Format format = spk::Texture::Format::RGBA;

			bool operator==(const ColorAttachmentDescription &) const = default;
		};

		struct DepthAttachmentDescription
		{
			AttachmentStorage storage = AttachmentStorage::RenderBuffer;
			spk::Texture::Format format = spk::Texture::Format::Depth24Stencil8;

			bool operator==(const DepthAttachmentDescription &) const = default;
		};

		struct Description
		{
			spk::Vector2UInt size{};
			std::optional<ColorAttachmentDescription> color;
			std::optional<DepthAttachmentDescription> depth;

			bool operator==(const Description &) const = default;
		};

	private:
		Description _description;
		spk::SurfaceState _surfaceState;
		spk::Viewport _viewport;
		std::optional<spk::Texture> _colorAttachment;
		std::optional<spk::Texture> _depthAttachment;
		std::atomic<std::uint64_t> _version = 1;

		mutable spk::CachedOpenGLObjectCollection<spk::OpenGL::FrameBufferObject> _gpu;

		static void _validateDescription(const Description &p_description);
		void _applySize(const spk::Vector2UInt &p_size);

	protected:
		void _synchronize() const override;

	public:
		[[nodiscard]] static Description colorTarget(const spk::Vector2UInt &p_size);
		[[nodiscard]] static Description depthTextureTarget(
			const spk::Vector2UInt &p_size,
			spk::Texture::Format p_format = spk::Texture::Format::Depth24);

		explicit FrameBufferObject(const Description &p_description);

		// Compatibility mapping: false creates RGBA color only; true creates RGBA
		// color plus a Depth24Stencil8 renderbuffer.
		explicit FrameBufferObject(const spk::Vector2UInt &p_size = {0, 0}, bool p_withDepth = true);

		FrameBufferObject(const FrameBufferObject &) = delete;
		FrameBufferObject &operator=(const FrameBufferObject &) = delete;
		FrameBufferObject(FrameBufferObject &&) noexcept = delete;
		FrameBufferObject &operator=(FrameBufferObject &&) noexcept = delete;

		void resize(const spk::Vector2UInt &p_size);

		[[nodiscard]] const Description &description() const noexcept;
		[[nodiscard]] const spk::Vector2UInt &size() const noexcept;
		[[nodiscard]] bool hasColorAttachment() const noexcept;
		[[nodiscard]] bool hasDepthAttachment() const noexcept;
		[[nodiscard]] bool hasSampleableDepth() const noexcept;
		[[nodiscard]] const spk::Texture *tryColorAttachment() const noexcept;
		[[nodiscard]] const spk::Texture *tryDepthAttachment() const noexcept;
		// Compatibility alias for hasDepthAttachment().
		[[nodiscard]] bool hasDepth() const noexcept;

		[[nodiscard]] const spk::SurfaceState &surfaceState() const noexcept;
		[[nodiscard]] const spk::Viewport &viewport() const noexcept;
		[[nodiscard]] const spk::Texture &colorAttachment() const;
		[[nodiscard]] const spk::Texture &depthAttachment() const;

		[[nodiscard]] std::uint64_t version() const noexcept;

		[[nodiscard]] spk::OpenGL::FrameBufferObject &gpu(const spk::RenderContext &p_context) const;
		[[nodiscard]] bool hasGpu(const spk::RenderContext &p_context) const noexcept;
	};

	using FrameBufferDescription = FrameBufferObject::Description;
	using FrameBufferAttachmentStorage = FrameBufferObject::AttachmentStorage;
	using FrameBufferColorAttachmentDescription = FrameBufferObject::ColorAttachmentDescription;
	using FrameBufferDepthAttachmentDescription = FrameBufferObject::DepthAttachmentDescription;
}
