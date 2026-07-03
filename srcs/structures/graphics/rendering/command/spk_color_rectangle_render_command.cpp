#include "structures/graphics/rendering/command/spk_color_rectangle_render_command.hpp"

#include <memory>

namespace spk
{
	ColorRectangleRenderCommand::ColorRectangleRenderCommand(
		spk::Rect2D p_rect,
		spk::Color p_color,
		float p_depth)
	{
		const spk::Vector2 topLeft = static_cast<spk::Vector2>(p_rect.anchor);
		const spk::Vector2 bottomLeft = spk::Vector2(static_cast<float>(p_rect.left()), static_cast<float>(p_rect.bottom()));
		const spk::Vector2 bottomRight = spk::Vector2(static_cast<float>(p_rect.right()), static_cast<float>(p_rect.bottom()));
		const spk::Vector2 topRight = spk::Vector2(static_cast<float>(p_rect.right()), static_cast<float>(p_rect.top()));

		spk::ColorMesh2D::Builder builder;
		builder.addShape(
			spk::ColorMesh2D::Vertex{{topLeft, p_depth}, p_color},
			spk::ColorMesh2D::Vertex{{bottomLeft, p_depth}, p_color},
			spk::ColorMesh2D::Vertex{{bottomRight, p_depth}, p_color},
			spk::ColorMesh2D::Vertex{{topRight, p_depth}, p_color});
		_mesh = std::make_shared<spk::ColorMesh2D>(builder.bake());

		_colorCommand = std::make_unique<spk::DrawColorMeshRenderCommand>(_mesh);
	}

	void ColorRectangleRenderCommand::execute(spk::RenderContext &p_renderContext)
	{
		_colorCommand->execute(p_renderContext);
	}
}
