#pragma once

#include <cstddef>

#include "structures/graphics/geometry/spk_generic_mesh.hpp"
#include "structures/math/spk_vector2.hpp"
#include "structures/math/spk_vector3.hpp"

namespace spk
{
	// Vertex baked by the voxel mesher: a textured 3D vertex extended with the voxel's
	// opacity (1 = opaque, 0 = invisible) so one transparent mesh can mix voxel types
	// with different transparency levels.
	struct VoxelVertex3D
	{
		spk::Vector3 position{};
		spk::Vector3 normal{};
		spk::Vector2 uv{};
		float alpha = 1.0f;

		[[nodiscard]] bool operator==(const VoxelVertex3D &p_other) const noexcept
		{
			return position == p_other.position && normal == p_other.normal && uv == p_other.uv && alpha == p_other.alpha;
		}
	};
	static_assert(sizeof(VoxelVertex3D) == 36);

	using VoxelMesh3DLayout = spk::MeshLayout<
		spk::LayoutBufferObject::Attribute{0, spk::LayoutBufferObject::Attribute::Type::Vector3},
		spk::LayoutBufferObject::Attribute{1, spk::LayoutBufferObject::Attribute::Type::Vector3},
		spk::LayoutBufferObject::Attribute{2, spk::LayoutBufferObject::Attribute::Type::Vector2},
		spk::LayoutBufferObject::Attribute{3, spk::LayoutBufferObject::Attribute::Type::Float}>;
	using VoxelMesh3D = spk::GenericMesh<spk::VoxelVertex3D, spk::VoxelMesh3DLayout>;

	// A baked voxel grid, split by blending needs: the opaque mesh renders first with
	// depth writes, the transparent one renders afterwards with alpha blending.
	struct VoxelRenderMeshes
	{
		spk::VoxelMesh3D opaque;
		spk::VoxelMesh3D transparent;
	};
}
