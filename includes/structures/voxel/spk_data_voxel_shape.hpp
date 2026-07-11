#pragma once

#include "structures/voxel/spk_voxel_shape.hpp"

#include <string>
#include <vector>

namespace spk
{
	// Geometry-only payload for DataVoxelShape: vertex positions in normalized cell space
	// with per-vertex local UVs, referencing texture slots by name. Outer/inner face
	// classification is not declared here - DataVoxelShape computes it from the geometry.
	struct VoxelShapeDescription
	{
		struct Polygon
		{
			std::string slot;
			std::vector<spk::Vector3> positions;
			std::vector<spk::Vector2> uvs;
		};

		std::vector<Polygon> polygons;
	};

	// A VoxelShape built from data instead of a dedicated subclass, so applications can
	// define custom shapes (parsed from files or synthesized at runtime) without adding
	// classes to the library.
	//
	// Faces are classified automatically: a polygon whose vertices all lie on one plane of
	// the unit-cell boundary and whose normal points out of the cell joins that plane's
	// outer-shell face, participating in neighbor occlusion. Every other polygon - inset
	// geometry, diagonal planes, inward-facing boundary polygons - becomes an inner face.
	// Vertex coordinates within BoundaryEpsilon of 0 or 1 are snapped exactly onto the
	// boundary so classification and the mesher's boundary-coverage math stay exact.
	class DataVoxelShape : public spk::VoxelShape
	{
	private:
		spk::VoxelShapeDescription _description;

	protected:
		void _constructRenderFaces() override;

	public:
		DataVoxelShape(
			TextureSlots p_textures,
			spk::VoxelShapeDescription p_description,
			const spk::Vector2Int &p_atlasSize = DefaultAtlasSize);

		[[nodiscard]] const spk::VoxelShapeDescription &description() const noexcept;
	};
}
