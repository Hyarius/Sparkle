#pragma once

#include "structures/graphics/geometry/spk_texture_mesh_2d.hpp"

namespace pg
{
	// Builds a unit-ish cube centered on the origin as a spk::TextureMesh2D (whose vertex
	// already carries a Vector3 position). Each of the 6 faces is wound CCW as seen from
	// outside, so back-face culling alone renders it correctly (no depth buffer needed —
	// see steps/step1.md). Each face maps the full [0,1] UV rect.
	[[nodiscard]] inline spk::TextureMesh2D makeCube(float p_size = 1.0f)
	{
		using V = spk::TextureVertex2D;

		const float h = p_size * 0.5f;
		spk::TextureMesh2D mesh;

		// +X (right)
		mesh.addShape(
			V{{h, -h, h}, {0.0f, 0.0f}},
			V{{h, -h, -h}, {1.0f, 0.0f}},
			V{{h, h, -h}, {1.0f, 1.0f}},
			V{{h, h, h}, {0.0f, 1.0f}});

		// -X (left)
		mesh.addShape(
			V{{-h, -h, -h}, {0.0f, 0.0f}},
			V{{-h, -h, h}, {1.0f, 0.0f}},
			V{{-h, h, h}, {1.0f, 1.0f}},
			V{{-h, h, -h}, {0.0f, 1.0f}});

		// +Y (top)
		mesh.addShape(
			V{{-h, h, h}, {0.0f, 0.0f}},
			V{{h, h, h}, {1.0f, 0.0f}},
			V{{h, h, -h}, {1.0f, 1.0f}},
			V{{-h, h, -h}, {0.0f, 1.0f}});

		// -Y (bottom)
		mesh.addShape(
			V{{-h, -h, -h}, {0.0f, 0.0f}},
			V{{h, -h, -h}, {1.0f, 0.0f}},
			V{{h, -h, h}, {1.0f, 1.0f}},
			V{{-h, -h, h}, {0.0f, 1.0f}});

		// +Z (front)
		mesh.addShape(
			V{{-h, -h, h}, {0.0f, 0.0f}},
			V{{h, -h, h}, {1.0f, 0.0f}},
			V{{h, h, h}, {1.0f, 1.0f}},
			V{{-h, h, h}, {0.0f, 1.0f}});

		// -Z (back)
		mesh.addShape(
			V{{h, -h, -h}, {0.0f, 0.0f}},
			V{{-h, -h, -h}, {1.0f, 0.0f}},
			V{{-h, h, -h}, {1.0f, 1.0f}},
			V{{h, h, -h}, {0.0f, 1.0f}});

		return mesh;
	}
}
