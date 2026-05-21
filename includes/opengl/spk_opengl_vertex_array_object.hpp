#pragma once

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

#include <GL/glew.h>

#include "opengl/spk_opengl_index_buffer_object.hpp"
#include "opengl/spk_opengl_vertex_buffer_object.hpp"
#include "traits/spk_synchronizable_trait.hpp"

namespace spk::OpenGL
{
	class VertexArrayObject : public spk::SynchronizableTrait
	{
	public:
		struct Attribute
		{
			GLuint index = 0;
			GLint componentCount = 0;
			GLenum componentType = GL_FLOAT;
			bool normalized = false;
			GLsizei stride = 0;
			std::size_t offset = 0;
		};

	private:
		struct VertexBufferBinding
		{
			std::shared_ptr<VertexBufferObject> buffer;
			Attribute attribute;
		};

		mutable GLuint _id = 0;
		std::vector<VertexBufferBinding> _vertexBufferBindings;
		std::shared_ptr<IndexBufferObject> _indexBuffer = nullptr;

	private:
		void _allocate() const;
		void _release() const;

	protected:
		void _synchronize() const override;

	public:
		VertexArrayObject();
		~VertexArrayObject();

		VertexArrayObject(const VertexArrayObject&) = delete;
		VertexArrayObject& operator=(const VertexArrayObject&) = delete;
		VertexArrayObject(VertexArrayObject&&) noexcept = delete;
		VertexArrayObject& operator=(VertexArrayObject&&) noexcept = delete;

		[[nodiscard]] GLuint id() const noexcept;
		[[nodiscard]] bool isAllocated() const noexcept;

		void addVertexBuffer(std::shared_ptr<VertexBufferObject> p_buffer, Attribute p_attribute);
		void clearVertexBuffers();

		void setIndexBuffer(std::shared_ptr<IndexBufferObject> p_buffer);
		void clearIndexBuffer();
		[[nodiscard]] const std::shared_ptr<IndexBufferObject>& indexBuffer() const noexcept;

		void activate();
		void deactivate() const;
	};
}

#endif
