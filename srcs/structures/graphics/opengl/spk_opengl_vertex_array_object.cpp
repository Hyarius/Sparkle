#include "structures/graphics/spk_vertex_array_object.hpp"

#include <memory>
#include <stdexcept>

#include "structures/graphics/rendering/context/spk_render_context.hpp"

namespace spk
{
	VertexArrayObject::VertexArrayObject()
	{
		requestSynchronization();
	}

	std::uint64_t VertexArrayObject::_effectiveVersion() const
	{
		std::uint64_t version = _layoutVersion;
		for (const VertexBufferBinding &binding : _vertexBufferBindings)
		{
			if (binding.buffer != nullptr)
			{
				version += binding.buffer->structureVersion();
			}
		}
		if (_indexBuffer != nullptr)
		{
			version += _indexBuffer->structureVersion();
		}
		return version;
	}

	void VertexArrayObject::_synchronize() const
	{
		spk::RenderContext *ctx = spk::RenderContext::current();
		if (ctx != nullptr && ctx->supportsOpenGLCommands() == true)
		{
			(void)gpu(*ctx);
		}
	}

	spk::OpenGL::VertexArray &VertexArrayObject::gpu(const spk::RenderContext &p_context) const
	{
		return _gpu.resolve(
			p_context,
			_effectiveVersion(),
			[this, &p_context]() {
				auto vertexArray = std::make_unique<spk::OpenGL::VertexArray>();
				if (p_context.isVertexArrayActive(vertexArray.get()) == false)
				{
					glBindVertexArray(vertexArray->id());
					p_context.setActiveVertexArray(vertexArray.get());
				}

				for (const VertexBufferBinding &binding : _vertexBufferBindings)
				{
					if (binding.buffer == nullptr)
					{
						throw std::runtime_error("spk::VertexArrayObject contains a null vertex buffer binding");
					}

					binding.buffer->activate(p_context);
					const Attribute &attribute = binding.attribute;

					glEnableVertexAttribArray(attribute.index);
					glVertexAttribPointer(
						attribute.index,
						attribute.componentCount,
						attribute.componentType,
						attribute.normalized == true ? GL_TRUE : GL_FALSE,
						attribute.stride,
						reinterpret_cast<const void *>(attribute.offset));
				}

				if (_indexBuffer != nullptr)
				{
					_indexBuffer->activate(p_context);
				}

				return vertexArray;
			});
	}

	bool VertexArrayObject::hasGpu(const spk::RenderContext &p_context) const noexcept
	{
		const spk::OpenGL::VertexArray *object = _gpu.find(p_context);
		return object != nullptr && object->version() == _effectiveVersion();
	}

	void VertexArrayObject::addVertexBuffer(std::shared_ptr<VertexBufferObject> p_buffer, Attribute p_attribute)
	{
		if (p_buffer == nullptr)
		{
			throw std::runtime_error("spk::VertexArrayObject::addVertexBuffer requires a valid buffer");
		}

		_vertexBufferBindings.push_back(VertexBufferBinding{.buffer = std::move(p_buffer), .attribute = p_attribute});
		++_layoutVersion;
		requestSynchronization();
	}

	void VertexArrayObject::clearVertexBuffers()
	{
		_vertexBufferBindings.clear();
		++_layoutVersion;
		requestSynchronization();
	}

	void VertexArrayObject::setIndexBuffer(std::shared_ptr<IndexBufferObject> p_buffer)
	{
		_indexBuffer = std::move(p_buffer);
		++_layoutVersion;
		requestSynchronization();
	}

	void VertexArrayObject::clearIndexBuffer()
	{
		_indexBuffer = nullptr;
		++_layoutVersion;
		requestSynchronization();
	}

	const std::shared_ptr<IndexBufferObject> &VertexArrayObject::indexBuffer() const noexcept
	{
		return _indexBuffer;
	}

	void VertexArrayObject::activate(const spk::RenderContext &p_context) const
	{
		if (needsSynchronization() == true)
		{
			synchronize();
		}

		bool childrenClean = true;
		for (const VertexBufferBinding &binding : _vertexBufferBindings)
		{
			if (binding.buffer != nullptr && binding.buffer->hasGpu(p_context) == false)
			{
				childrenClean = false;
				break;
			}
		}
		if (childrenClean == true && _indexBuffer != nullptr && _indexBuffer->hasGpu(p_context) == false)
		{
			childrenClean = false;
		}

		if (childrenClean == false)
		{
			if (p_context.isVertexArrayActive(nullptr) == false)
			{
				glBindVertexArray(0);
				p_context.setActiveVertexArray(nullptr);
			}
			for (const VertexBufferBinding &binding : _vertexBufferBindings)
			{
				if (binding.buffer != nullptr)
				{
					(void)binding.buffer->gpu(p_context);
				}
			}
			if (_indexBuffer != nullptr)
			{
				(void)_indexBuffer->gpu(p_context);
			}
		}

		spk::OpenGL::VertexArray &vertexArray = gpu(p_context);
		if (p_context.isVertexArrayActive(&vertexArray) == false)
		{
			glBindVertexArray(vertexArray.id());
			p_context.setActiveVertexArray(&vertexArray);
		}
	}

	void VertexArrayObject::deactivate() const
	{
		spk::RenderContext *context = spk::RenderContext::current();
		if (context != nullptr && context->isVertexArrayActive(nullptr) == true)
		{
			return;
		}

		glBindVertexArray(0);

		if (context != nullptr)
		{
			context->setActiveVertexArray(nullptr);
		}
	}
}
