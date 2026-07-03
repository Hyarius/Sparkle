#pragma once

#include "geometry/mesh3d.hpp"

namespace pg
{
	// Builds a cube centered on the origin. Each face owns four vertices so its normal stays
	// discontinuous across hard edges, and is wound CCW as seen from outside.
	[[nodiscard]] inline Mesh3D makeCube(float p_size = 1.0f)
	{
		using V = MeshVertex3D;

		const float h = p_size * 0.5f;
		Mesh3D::Builder builder;
		builder.reserve(24, 36);

		// +X (right)
		builder.addShape(
			V{{h, -h, h}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
			V{{h, -h, -h}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
			V{{h, h, -h}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
			V{{h, h, h}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}});

		// -X (left)
		builder.addShape(
			V{{-h, -h, -h}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
			V{{-h, -h, h}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
			V{{-h, h, h}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
			V{{-h, h, -h}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}});

		// +Y (top)
		builder.addShape(
			V{{-h, h, h}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
			V{{h, h, h}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
			V{{h, h, -h}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
			V{{-h, h, -h}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}});

		// -Y (bottom)
		builder.addShape(
			V{{-h, -h, -h}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
			V{{h, -h, -h}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
			V{{h, -h, h}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
			V{{-h, -h, h}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}});

		// +Z (front)
		builder.addShape(
			V{{-h, -h, h}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
			V{{h, -h, h}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
			V{{h, h, h}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
			V{{-h, h, h}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}});

		// -Z (back)
		builder.addShape(
			V{{h, -h, -h}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},
			V{{-h, -h, -h}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},
			V{{-h, h, -h}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},
			V{{h, h, -h}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}});

		return builder.bake();
	}
}
