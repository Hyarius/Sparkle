#pragma once

#include "structures/voxel/spk_voxel_shape.hpp"

#include <cstddef>
#include <unordered_map>
#include <utility>
#include <vector>

namespace spk
{
	// Exact boundary-plane occlusion support for the voxel mesher.
	//
	// A neighbor face that only partially covers the shared cell boundary removes just
	// the area it overlaps: both faces are projected on the boundary plane and the
	// occluder footprint is subtracted from every source polygon (convex clipping).

	// Visible remnants of p_face once p_occluder's footprint is subtracted, concatenated
	// over the source polygons in polygon-index order — the exact polygons, in the exact
	// order, the mesher emits for a partially occluded face. Non-projectable or
	// non-convex configurations conservatively keep the affected source polygon whole.
	[[nodiscard]] std::vector<spk::VoxelShapePolygon> visibleFaceRemnants(
		const spk::VoxelShapeFace &p_face,
		const spk::VoxelShapeFace &p_occluder);

	// Memoizes visibleFaceRemnants per distinct (source face, occluder face) identity.
	//
	// Both operands are precomputed transform variants living at a fixed address inside
	// their VoxelShape once initialize() ran, so the pointer pair is a stable key and a
	// recurring adjacency (say slope-against-cube) is clipped once instead of once per
	// cell. The cache is meant to be owned by a single bake — create it in the mesh
	// build, drop it with it: it is not synchronized, and the remnant vectors it owns
	// must outlive every reference emitted during that bake, which the per-bake
	// lifetime guarantees under concurrent WorkerPool bakes.
	class VoxelFaceRemnantCache
	{
	private:
		using FacePair = std::pair<const spk::VoxelShapeFace *, const spk::VoxelShapeFace *>;

		struct FacePairHash
		{
			[[nodiscard]] std::size_t operator()(const FacePair &p_pair) const noexcept;
		};

		std::unordered_map<FacePair, std::vector<spk::VoxelShapePolygon>, FacePairHash> _remnants;

	public:
		// Cached remnant list for the pair, computed on first use. The returned
		// reference and the polygons behind it stay valid for the cache's lifetime:
		// rehashing moves the vector objects, never their heap storage.
		[[nodiscard]] const std::vector<spk::VoxelShapePolygon> &remnants(
			const spk::VoxelShapeFace &p_face,
			const spk::VoxelShapeFace &p_occluder);
	};
}
