#pragma once

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <span>
#include <vector>

#include <GL/glew.h>

#include "structures/container/spk_binary_field.hpp"
#include "structures/design_pattern/spk_synchronizable_trait.hpp"

namespace spk
{
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
		mutable GLuint _id = 0;
		Target _target = Target::Array;
		Usage _usage = Usage::DynamicDraw;
		std::vector<std::uint8_t> _cpuBuffer;
		spk::BinaryField _field;
		mutable std::size_t _allocatedSize = 0;

	protected:
		void _allocate() const;
		void _release() const;
		void _resetField();
		void _synchronize() const override;

	public:
		explicit BufferObject(
			Target p_target = Target::Array,
			Usage p_usage = Usage::DynamicDraw,
			std::size_t p_size = 0);
		~BufferObject();

		BufferObject(const BufferObject&) = delete;
		BufferObject& operator=(const BufferObject&) = delete;
		BufferObject(BufferObject&&) noexcept = delete;
		BufferObject& operator=(BufferObject&&) noexcept = delete;

		[[nodiscard]] GLuint id() const noexcept;
		[[nodiscard]] Target target() const noexcept;
		[[nodiscard]] Usage usage() const noexcept;
		[[nodiscard]] std::size_t size() const noexcept;
		[[nodiscard]] bool isAllocated() const noexcept;

		void setTarget(Target p_target);
		void setUsage(Usage p_usage);

		void resize(std::size_t p_size);
		void clear();
		void edit(const void* p_data, std::size_t p_size, std::size_t p_offset = 0);
		void append(const void* p_data, std::size_t p_size);

		[[nodiscard]] std::uint8_t* data();
		[[nodiscard]] const std::uint8_t* data() const;
		[[nodiscard]] std::span<std::uint8_t> bytes();
		[[nodiscard]] std::span<const std::uint8_t> bytes() const;
		[[nodiscard]] spk::BinaryField& field();
		[[nodiscard]] const spk::BinaryField& field() const;

		virtual void activate();
		virtual void deactivate() const;
		void activateBase(GLuint p_bindingPoint);
		void activateRange(GLuint p_bindingPoint, GLintptr p_offset, GLsizeiptr p_size);
	};
}
