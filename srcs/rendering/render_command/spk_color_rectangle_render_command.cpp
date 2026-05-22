#include "rendering/render_command/spk_color_rectangle_render_command.hpp"

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

#include "rendering/spk_mesh_2d.hpp"

namespace spk
{
	ColorRectangleRenderCommand::ColorRectangleRenderCommand(
		spk::Rect2D p_rect,
		spk::Color p_color,
		float p_depth)
	{
		(void)p_depth;

		const spk::Vector2 topLeft     = static_cast<spk::Vector2>(p_rect.anchor);
		const spk::Vector2 bottomLeft  = spk::Vector2(static_cast<float>(p_rect.left()),  static_cast<float>(p_rect.bottom()));
		const spk::Vector2 bottomRight = spk::Vector2(static_cast<float>(p_rect.right()), static_cast<float>(p_rect.bottom()));
		const spk::Vector2 topRight    = spk::Vector2(static_cast<float>(p_rect.right()), static_cast<float>(p_rect.top()));

		spk::Mesh2D mesh;
		mesh.addShape(
			spk::Vertex2D{topLeft,     {}},
			spk::Vertex2D{bottomLeft,  {}},
			spk::Vertex2D{bottomRight, {}},
			spk::Vertex2D{topRight,    {}});

		_colorCommand = std::make_unique<spk::DrawColorMeshRenderCommand>(std::move(mesh), p_color);
	}

	void ColorRectangleRenderCommand::execute(spk::IRenderContext& p_renderContext)
	{
		_colorCommand->execute(p_renderContext);
	}
}

#endif
