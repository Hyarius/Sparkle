#include "structures/graphics/opengl/spk_opengl_object.hpp"

#include "structures/graphics/opengl/spk_opengl_render_context.hpp"

namespace spk::OpenGL
{
	Object::Object()
	{
		spk::RenderContext *context = spk::RenderContext::current();
		if (context != nullptr)
		{
			_contextId = context->id();
		}
	}

	bool Object::_ownsCurrentContext() const noexcept
	{
		spk::RenderContext *context = spk::RenderContext::current();
		return context != nullptr && context->id() == _contextId;
	}

	std::uint64_t Object::contextId() const noexcept
	{
		return _contextId;
	}

	std::uint64_t Object::version() const noexcept
	{
		return _version;
	}

	std::uint64_t Object::contentVersion() const noexcept
	{
		return _contentVersion;
	}

	std::uint64_t contextIdOf(const spk::RenderContext &p_context) noexcept
	{
		return p_context.id();
	}

	bool isContextCurrent(const spk::RenderContext &p_context) noexcept
	{
		return spk::RenderContext::current() == &p_context;
	}

	bool isContextAlive(std::uint64_t p_contextId) noexcept
	{
		return spk::RenderContext::fromId(p_contextId) != nullptr;
	}

	std::uint64_t contextDeathGeneration() noexcept
	{
		return spk::RenderContext::deathGeneration();
	}

	void notifyProgramDeleted(const Program &p_program) noexcept
	{
		spk::RenderContext *current = spk::RenderContext::current();
		if (current != nullptr)
		{
			current->onProgramDeleted(p_program);
		}
	}

	void notifyVertexArrayDeleted(const VertexArray &p_vertexArray) noexcept
	{
		spk::RenderContext *current = spk::RenderContext::current();
		if (current != nullptr)
		{
			current->onVertexArrayDeleted(p_vertexArray);
		}
	}

	void notifyBufferDeleted(const Buffer &p_buffer) noexcept
	{
		spk::RenderContext *current = spk::RenderContext::current();
		if (current != nullptr)
		{
			current->onBufferDeleted(p_buffer);
		}
	}

	void releaseObject(std::unique_ptr<Object> p_object)
	{
		if (p_object == nullptr)
		{
			return;
		}

		spk::RenderContext *current = spk::RenderContext::current();
		if (current != nullptr && current->id() == p_object->contextId())
		{
			return;
		}

		spk::RenderContext *owner = spk::RenderContext::fromId(p_object->contextId());
		if (owner != nullptr)
		{
			owner->scheduleRelease(std::move(p_object));
		}
	}
}
