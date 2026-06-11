#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include <GL/glew.h>

#include "structures/graphics/opengl/spk_cached_opengl_object_collection.hpp"
#include "structures/graphics/spk_index_buffer_object.hpp"
#include "structures/graphics/opengl/spk_opengl_vertex_array.hpp"
#include "structures/graphics/spk_vertex_buffer_object.hpp"
#include "structures/design_pattern/spk_synchronizable_trait.hpp"

namespace spk
{
	class RenderContext;

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

		std::vector<VertexBufferBinding> _vertexBufferBindings;
		std::shared_ptr<IndexBufferObject> _indexBuffer = nullptr;

		std::uint64_t _layoutVersion = 1;

		mutable spk::CachedOpenGLObjectCollection<spk::OpenGL::VertexArray> _gpu;

		[[nodiscard]] std::uint64_t _effectiveVersion() const;

	protected:
		void _synchronize() const override;

	public:
		VertexArrayObject();
		~VertexArrayObject() = default;

		VertexArrayObject(const VertexArrayObject&) = delete;
		VertexArrayObject& operator=(const VertexArrayObject&) = delete;
		VertexArrayObject(VertexArrayObject&&) noexcept = delete;
		VertexArrayObject& operator=(VertexArrayObject&&) noexcept = delete;

		// Resolves (building if needed) this vertex array's GPU copy for p_context.
		// p_context must be the current context.
		[[nodiscard]] spk::OpenGL::VertexArray& gpu(const spk::RenderContext& p_context) const;
		[[nodiscard]] bool hasGpu(const spk::RenderContext& p_context) const noexcept;

		void addVertexBuffer(std::shared_ptr<VertexBufferObject> p_buffer, Attribute p_attribute);
		void clearVertexBuffers();

		void setIndexBuffer(std::shared_ptr<IndexBufferObject> p_buffer);
		void clearIndexBuffer();
		[[nodiscard]] const std::shared_ptr<IndexBufferObject>& indexBuffer() const noexcept;

		void activate(const spk::RenderContext& p_context);
		void deactivate() const;
	};
}
