#pragma once

#include "structures/voxel/spk_voxel_ids.hpp"
#include "structures/voxel/spk_voxel_shape.hpp"

#include <cstddef>
#include <memory>
#include <vector>

namespace spk
{
	// Textures of one fluid family. Every generated state renders as a full-width slab
	// using these three slots; the side texture is cropped vertically to the fill height.
	struct VoxelFluidAppearance
	{
		spk::AtlasCell topTexture{};
		spk::AtlasCell bottomTexture{};
		spk::AtlasCell sideTexture{};

		float transparency = 0.0f;
		spk::Vector2Int atlasSize = spk::VoxelShape::DefaultAtlasSize;
	};

	// Everything VoxelRegistry::registerFluid needs to generate one fluid family: the
	// number of flow levels (level k renders at height k / levelCount), whether the fluid
	// forms falling columns, and how it looks. Fluid geometry is always engine-generated -
	// a caller can never supply an arbitrary shape, so slope- or stair-shaped "water"
	// cannot exist.
	struct VoxelFluidDescription
	{
		std::size_t levelCount = 8;
		bool falls = true;

		VoxelFluidAppearance appearance;
	};

	// One generated flow level of a fluid family. Levels are one-based at the public fluid
	// API (level 1 = weakest flow, level N = strongest); level k is state k of the family's
	// semantic type and renders at the normalized height k / N.
	struct VoxelFluidState
	{
		spk::VoxelStateId state{};
		spk::VoxelRuntimeId runtime{};

		std::size_t level = 0;
		float height = 0.0f;
	};

	// One registered fluid resolved to ONE semantic voxel type: state 0 is the persistent
	// source, states 1..N the generated flow levels. The source and the strongest flow
	// level stay distinct states even though both render at full height.
	struct VoxelFluidFamily
	{
		spk::VoxelTypeId type{};

		spk::VoxelStateId sourceState{};
		spk::VoxelRuntimeId sourceRuntime{};

		std::vector<VoxelFluidState> levels;

		bool falls = true;

		[[nodiscard]] std::size_t levelCount() const noexcept;

		// One-based level access: level(1) is the weakest flow, level(levelCount()) the
		// strongest.
		[[nodiscard]] const VoxelFluidState &level(std::size_t p_level) const;
	};

	// Where a given runtime id sits inside its fluid family, for O(1) classification during
	// simulation. A source reports the maximum level (it pours at full strength).
	struct VoxelFluidRef
	{
		std::size_t family = 0;

		spk::VoxelTypeId type{};
		spk::VoxelStateId state{};
		spk::VoxelRuntimeId runtime{};

		std::size_t level = 0;
		bool source = false;
	};

	namespace detail
	{
		// Engine-generated fluid geometry: a full-width axis-aligned slab from y == 0 up to
		// the normalized fill height (0 < height <= 1), textured through the "top", "bottom"
		// and "side" slots. Side UVs are cropped to the fill (bottomV = 1, topV = 1 - height).
		[[nodiscard]] std::unique_ptr<spk::VoxelShape> makeVoxelFluidStateShape(
			const spk::VoxelFluidAppearance &p_appearance,
			float p_height);
	}
}
