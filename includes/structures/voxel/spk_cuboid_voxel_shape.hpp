#pragma once

#include "structures/voxel/spk_voxel_shape.hpp"

namespace spk
{
	// Axis-aligned box constrained to one voxel cell. Unlike CubeVoxelShape, its
	// bounds may be inset on any axis, which makes it suitable for trunks, posts,
	// rocks and other scenery that should not occupy the full visual cell.
	class CuboidVoxelShape : public spk::VoxelShape
	{
	private:
		spk::Vector3 _minimum;
		spk::Vector3 _maximum;

	protected:
		void _constructRenderFaces() override;

	public:
		CuboidVoxelShape(
			TextureSlots p_textures,
			const spk::Vector3 &p_minimum,
			const spk::Vector3 &p_maximum,
			const spk::Vector2Int &p_atlasSize = DefaultAtlasSize);
		CuboidVoxelShape(
			const spk::AtlasCell &p_uniformTexture,
			const spk::Vector3 &p_minimum,
			const spk::Vector3 &p_maximum,
			const spk::Vector2Int &p_atlasSize = DefaultAtlasSize);

		[[nodiscard]] const spk::Vector3 &minimum() const noexcept;
		[[nodiscard]] const spk::Vector3 &maximum() const noexcept;
	};
}
