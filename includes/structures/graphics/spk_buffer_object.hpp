#pragma once

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <span>
#include <vector>

#include <GL/glew.h>

#include "structures/container/spk_binary_field.hpp"
#include "structures/graphics/opengl/spk_cached_opengl_object_collection.hpp"
#include "structures/graphics/opengl/spk_opengl_buffer.hpp"
#include "structures/design_pattern/spk_synchronizable_trait.hpp"

namespace spk
{
	class RenderContext;

	class BufferObject : public spk::SynchronizableTrait
	{
	public:
		enum class Target : GLenum
		{
			Array = GL_ARRAY_BUFFER,
			ElementArray = GL_ELEMENT_ARRAY_BUFFER,
			Uniform = GL_UNIFORM_BUFFER,
			ShaderStorage = GL_SHADER_STORAGE_BUFFER,
			Texture = GL_TEXTURE_BUFFER,
			TransformFeedback = GL_TRANSFORM_FEEDBACK_BUFFER,
			PixelPack = GL_PIXEL_PACK_BUFFER,
			PixelUnpack = GL_PIXEL_UNPACK_BUFFER,
			DrawIndirect = GL_DRAW_INDIRECT_BUFFER,
			AtomicCounter = GL_ATOMIC_COUNTER_BUFFER
		};

		enum class Usage : GLenum
		{
			StaticDraw = GL_STATIC_DRAW,
			DynamicDraw = GL_DYNAMIC_DRAW,
			StreamDraw = GL_STREAM_DRAW,
			StaticRead = GL_STATIC_READ,
			DynamicRead = GL_DYNAMIC_READ,
			StreamRead = GL_STREAM_READ,
			StaticCopy = GL_STATIC_COPY,
			DynamicCopy = GL_DYNAMIC_COPY,
			StreamCopy = GL_STREAM_COPY
		};

	private:
		mutable std::mutex _mutex;
		Target _target = Target::Array;
		Usage _usage = Usage::DynamicDraw;
		std::vector<std::uint8_t> _cpuBuffer;
		spk::BinaryField _field;

		std::uint64_t _structureVersion = 1; // target / usage
		std::uint64_t _contentVersion = 1;   // bytes / size

		mutable spk::CachedOpenGLObjectCollection<spk::OpenGL::Buffer> _gpu;

	protected:
		void _resetField();
		void _synchronize() const override;

	public:
		explicit BufferObject(
			Target p_target = Target::Array,
			Usage p_usage = Usage::DynamicDraw,
			std::size_t p_size = 0);
		~BufferObject() = default;

		BufferObject(const BufferObject&) = delete;
		BufferObject& operator=(const BufferObject&) = delete;
		BufferObject(BufferObject&&) noexcept = delete;
		BufferObject& operator=(BufferObject&&) noexcept = delete;

		[[nodiscard]] Target target() const noexcept;
		[[nodiscard]] Usage usage() const noexcept;
		[[nodiscard]] std::size_t size() const noexcept;

		[[nodiscard]] std::uint64_t structureVersion() const noexcept;
		[[nodiscard]] std::uint64_t contentVersion() const noexcept;

		// Resolves (uploading if needed) this buffer's GPU copy for p_context.
		// p_context must be the current context.
		[[nodiscard]] spk::OpenGL::Buffer& gpu(const spk::RenderContext& p_context) const;
		[[nodiscard]] bool hasGpu(const spk::RenderContext& p_context) const noexcept;

		void resize(std::size_t p_size);
		void reserve(std::size_t p_size);
		void clear();
		void edit(const void* p_data, std::size_t p_size, std::size_t p_offset = 0);
		void append(const void* p_data, std::size_t p_size);

		[[nodiscard]] std::uint8_t* data();
		[[nodiscard]] const std::uint8_t* data() const;
		[[nodiscard]] std::span<std::uint8_t> bytes();
		[[nodiscard]] std::span<const std::uint8_t> bytes() const;
		[[nodiscard]] spk::BinaryField& field();
		[[nodiscard]] const spk::BinaryField& field() const;
		[[nodiscard]] bool empty() const;

		virtual void activate(const spk::RenderContext& p_context) const;
		virtual void deactivate() const;
		void activateBase(const spk::RenderContext& p_context, GLuint p_bindingPoint) const;
		void activateRange(const spk::RenderContext& p_context, GLuint p_bindingPoint, GLintptr p_offset, GLsizeiptr p_size) const;
	};
}
