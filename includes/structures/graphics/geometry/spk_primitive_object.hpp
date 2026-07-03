#pragma once

#include "structures/graphics/geometry/spk_texture_mesh_2d.hpp"
#include "structures/graphics/geometry/spk_texture_mesh_3d.hpp"
#include "structures/math/spk_vector2.hpp"

namespace spk
{
	class PrimitiveObject
	{
	public:
		PrimitiveObject() = delete;

		[[nodiscard]] static spk::TextureMesh2D CreateSquare(
			const spk::Vector2 &p_anchor,
			const spk::Vector2 &p_size,
			const spk::Vector2 &p_uvAnchor,
			const spk::Vector2 &p_uvSize)
		{
			spk::TextureMesh2D::Builder builder;

			const float left = p_anchor.x;
			const float right = p_anchor.x + p_size.x;
			const float bottom = p_anchor.y;
			const float top = p_anchor.y + p_size.y;

			const float uvLeft = p_uvAnchor.x;
			const float uvRight = p_uvAnchor.x + p_uvSize.x;
			const float uvTop = p_uvAnchor.y;
			const float uvBottom = p_uvAnchor.y + p_uvSize.y;

			builder.addShape(
				spk::TextureVertex2D{{left, top, 0.0f}, {uvLeft, uvTop}},
				spk::TextureVertex2D{{left, bottom, 0.0f}, {uvLeft, uvBottom}},
				spk::TextureVertex2D{{right, bottom, 0.0f}, {uvRight, uvBottom}},
				spk::TextureVertex2D{{right, top, 0.0f}, {uvRight, uvTop}});

			return builder.bake();
		}

		[[nodiscard]] static spk::TextureMesh3D CreateCube(float p_size = 1.0f)
		{
			using Vertex = spk::TextureVertex3D;
			const float halfSize = p_size * 0.5f;
			spk::TextureMesh3D::Builder builder;
			builder.reserve(24, 36);

			builder.addShape(
				Vertex{{halfSize, -halfSize, halfSize}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
				Vertex{{halfSize, -halfSize, -halfSize}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
				Vertex{{halfSize, halfSize, -halfSize}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
				Vertex{{halfSize, halfSize, halfSize}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}});
			builder.addShape(
				Vertex{{-halfSize, -halfSize, -halfSize}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
				Vertex{{-halfSize, -halfSize, halfSize}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
				Vertex{{-halfSize, halfSize, halfSize}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
				Vertex{{-halfSize, halfSize, -halfSize}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}});
			builder.addShape(
				Vertex{{-halfSize, halfSize, halfSize}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
				Vertex{{halfSize, halfSize, halfSize}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
				Vertex{{halfSize, halfSize, -halfSize}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
				Vertex{{-halfSize, halfSize, -halfSize}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}});
			builder.addShape(
				Vertex{{-halfSize, -halfSize, -halfSize}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
				Vertex{{halfSize, -halfSize, -halfSize}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
				Vertex{{halfSize, -halfSize, halfSize}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
				Vertex{{-halfSize, -halfSize, halfSize}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}});
			builder.addShape(
				Vertex{{-halfSize, -halfSize, halfSize}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
				Vertex{{halfSize, -halfSize, halfSize}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
				Vertex{{halfSize, halfSize, halfSize}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
				Vertex{{-halfSize, halfSize, halfSize}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}});
			builder.addShape(
				Vertex{{halfSize, -halfSize, -halfSize}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},
				Vertex{{-halfSize, -halfSize, -halfSize}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},
				Vertex{{-halfSize, halfSize, -halfSize}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},
				Vertex{{halfSize, halfSize, -halfSize}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}});

			return builder.bake();
		}
	};
}
