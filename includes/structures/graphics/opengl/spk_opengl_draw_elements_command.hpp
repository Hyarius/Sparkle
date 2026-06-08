#pragma once

#include <memory>
#include <optional>

#include <GL/glew.h>

#include "structures/graphics/opengl/spk_opengl_index_buffer_object.hpp"
#include "type/spk_opengl_primitive.hpp"
#include "structures/graphics/opengl/spk_opengl_vertex_array_object.hpp"
#include "structures/graphics/opengl/spk_opengl_program.hpp"
#include "structures/graphics/rendering/command/spk_render_command.hpp"

namespace spk
{
	class DrawElementsCommand : public spk::RenderCommand
	{
	private:
		Primitive _primitive = Primitive::Triangles;
		std::shared_ptr<spk::Program> _program = nullptr;
		std::shared_ptr<spk::VertexArrayObject> _vertexArray = nullptr;
		std::shared_ptr<spk::IndexBufferObject> _indexBuffer = nullptr;
		std::optional<GLsizei> _count;
		std::size_t _offset = 0;

	public:
		DrawElementsCommand(
			Primitive p_primitive,
			std::shared_ptr<spk::IndexBufferObject> p_indexBuffer,
			std::optional<GLsizei> p_count = std::nullopt,
			std::size_t p_offset = 0);
		DrawElementsCommand(
			Primitive p_primitive,
			std::shared_ptr<spk::Program> p_program,
			std::shared_ptr<spk::VertexArrayObject> p_vertexArray,
			std::shared_ptr<spk::IndexBufferObject> p_indexBuffer,
			std::optional<GLsizei> p_count = std::nullopt,
			std::size_t p_offset = 0);

		void execute(spk::RenderContext& p_renderContext) override;
	};
}
