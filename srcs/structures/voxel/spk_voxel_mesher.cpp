#include "structures/voxel/spk_voxel_mesher.hpp"
#include "structures/voxel/spk_voxel_mesher_occlusion.hpp"
#include "structures/voxel/spk_voxel_orientation.hpp"

#include <array>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

// Throughput notes. Chunk bakes run concurrently on spk::WorkerPool (one task per
// dirty chunk), so every acceleration structure here is either immutable
// (VoxelWorldToLocalPlaneTable, built at compile time) or owned by one bake and
// destroyed with it (VoxelFaceRemnantCache, the neighbor snapshot, the plan vectors).
// None of it changes the emitted meshes: the golden-reference tests in
// voxel_mesher_invariance_test.cpp pin the output byte-for-byte.

namespace
{
	constexpr std::size_t PlaneCount = static_cast<std::size_t>(spk::VoxelAxisPlane::Count);
	// Indexed by the VoxelAxisPlane value of the world plane.
	constexpr std::array OppositePlanes = {
		spk::VoxelAxisPlane::NegativeX,
		spk::VoxelAxisPlane::PositiveX,
		spk::VoxelAxisPlane::NegativeY,
		spk::VoxelAxisPlane::PositiveY,
		spk::VoxelAxisPlane::NegativeZ,
		spk::VoxelAxisPlane::PositiveZ};
	constexpr std::array PlaneOffsets = {
		spk::Vector3Int{1, 0, 0},
		spk::Vector3Int{-1, 0, 0},
		spk::Vector3Int{0, 1, 0},
		spk::Vector3Int{0, -1, 0},
		spk::Vector3Int{0, 0, 1},
		spk::Vector3Int{0, 0, -1}};

	// Immutable per-bake view of every cell an occlusion query can reach. In-bounds
	// reads are direct flat-index reads. External cells are only ever queried one step
	// outside the grid along one axis (every query starts from an in-bounds, non-empty
	// cell and moves by one plane offset), so the six boundary slabs bound all reachable
	// external cells: each face stores its observed world cells in a flat buffer sized
	// to the face area instead of a hashed world-position map. An absent external cell
	// reads as empty (no occlusion), and without a world lookup every external cell is
	// absent.
	class NeighborSnapshot
	{
	private:
		const spk::VoxelGrid &_grid;
		std::array<std::vector<std::optional<spk::VoxelCell>>, PlaneCount> _externalSlabs;

		[[nodiscard]] const spk::VoxelCell *_gridCell(const spk::Vector3Int &p_position) const
		{
			const spk::Vector3Int &size = _grid.size();
			const std::size_t index = static_cast<std::size_t>(p_position.x) +
									  static_cast<std::size_t>(size.x) *
										  (static_cast<std::size_t>(p_position.z) +
										   static_cast<std::size_t>(size.z) * static_cast<std::size_t>(p_position.y));
			return &_grid.cells()[index];
		}

		// Flat index of a position's in-plane coordinates inside face p_faceIndex's slab.
		[[nodiscard]] static std::size_t _slabIndex(
			std::size_t p_faceIndex,
			const spk::Vector3Int &p_position,
			const spk::Vector3Int &p_size)
		{
			switch (p_faceIndex >> 1)
			{
			case 0: // +X / -X faces span (y, z)
				return static_cast<std::size_t>(p_position.y) * static_cast<std::size_t>(p_size.z) +
					   static_cast<std::size_t>(p_position.z);
			case 1: // +Y / -Y faces span (x, z)
				return static_cast<std::size_t>(p_position.x) * static_cast<std::size_t>(p_size.z) +
					   static_cast<std::size_t>(p_position.z);
			default: // +Z / -Z faces span (x, y)
				return static_cast<std::size_t>(p_position.x) * static_cast<std::size_t>(p_size.y) +
					   static_cast<std::size_t>(p_position.y);
			}
		}

	public:
		NeighborSnapshot(
			const spk::VoxelGrid &p_grid,
			const spk::IVoxelCellLookup *p_worldLookup,
			const spk::Vector3Int &p_worldOrigin) :
			_grid(p_grid)
		{
			const spk::Vector3Int &size = p_grid.size();
			if (p_worldLookup == nullptr || size.x <= 0 || size.y <= 0 || size.z <= 0)
			{
				return;
			}

			for (std::size_t faceIndex = 0; faceIndex < PlaneCount; ++faceIndex)
			{
				const std::size_t axis = faceIndex >> 1;
				const int uCount = axis == 0 ? size.y : size.x;
				const int vCount = axis == 2 ? size.y : size.z;
				const int boundary = (faceIndex & 1) == 0
										 ? (axis == 0 ? size.x : axis == 1 ? size.y
																		   : size.z) -
											   1
										 : 0;

				std::vector<std::optional<spk::VoxelCell>> &slab = _externalSlabs[faceIndex];
				slab.resize(static_cast<std::size_t>(uCount) * static_cast<std::size_t>(vCount));

				for (int u = 0; u < uCount; ++u)
				{
					for (int v = 0; v < vCount; ++v)
					{
						const spk::Vector3Int position =
							axis == 0	? spk::Vector3Int{boundary, u, v}
							: axis == 1 ? spk::Vector3Int{u, boundary, v}
										: spk::Vector3Int{u, v, boundary};
						if (_gridCell(position)->isEmpty())
						{
							continue;
						}

						const spk::Vector3Int worldPosition = p_worldOrigin + position + PlaneOffsets[faceIndex];
						const spk::VoxelCell *observed = p_worldLookup->tryRenderableCell(worldPosition);
						if (observed != nullptr)
						{
							slab[_slabIndex(faceIndex, position, size)] = *observed;
						}
					}
				}
			}
		}

		[[nodiscard]] const spk::VoxelCell *tryCell(const spk::Vector3Int &p_gridPosition) const
		{
			if (_grid.isWithinBounds(p_gridPosition))
			{
				return _gridCell(p_gridPosition);
			}

			const spk::Vector3Int &size = _grid.size();
			const bool xIn = p_gridPosition.x >= 0 && p_gridPosition.x < size.x;
			const bool yIn = p_gridPosition.y >= 0 && p_gridPosition.y < size.y;
			const bool zIn = p_gridPosition.z >= 0 && p_gridPosition.z < size.z;

			std::size_t faceIndex = PlaneCount;
			if (yIn && zIn && p_gridPosition.x == size.x)
			{
				faceIndex = 0;
			}
			else if (yIn && zIn && p_gridPosition.x == -1)
			{
				faceIndex = 1;
			}
			else if (xIn && zIn && p_gridPosition.y == size.y)
			{
				faceIndex = 2;
			}
			else if (xIn && zIn && p_gridPosition.y == -1)
			{
				faceIndex = 3;
			}
			else if (xIn && yIn && p_gridPosition.z == size.z)
			{
				faceIndex = 4;
			}
			else if (xIn && yIn && p_gridPosition.z == -1)
			{
				faceIndex = 5;
			}
			if (faceIndex == PlaneCount)
			{
				return nullptr;
			}

			const std::vector<std::optional<spk::VoxelCell>> &slab = _externalSlabs[faceIndex];
			if (slab.empty())
			{
				return nullptr;
			}
			const std::optional<spk::VoxelCell> &cell = slab[_slabIndex(faceIndex, p_gridPosition, size)];
			return cell.has_value() ? &*cell : nullptr;
		}
	};

	struct FaceOcclusion
	{
		bool fullyOccluded = false;
		const spk::VoxelShapeFace *partialOccluder = nullptr;
	};

	// Lightweight plan entry: the polygon is referenced, never copied. It lives either
	// in the shape's precomputed transform variants or in the per-bake remnant cache,
	// both of which outlive emission.
	struct PlannedPolygon
	{
		const spk::VoxelShapePolygon *polygon = nullptr;
		spk::Vector3 offset;
		float alpha = 1.0f;
	};

	[[nodiscard]] bool transparentOcclusionCompatible(
		const spk::VoxelShape &p_shape,
		const spk::VoxelShape &p_neighborShape)
	{
		if (!p_shape.isTransparent() || !p_neighborShape.isTransparent())
		{
			return false;
		}
		if (&p_shape == &p_neighborShape)
		{
			return true;
		}

		const std::string &group = p_shape.transparentOcclusionGroup();
		return !group.empty() && group == p_neighborShape.transparentOcclusionGroup();
	}

	[[nodiscard]] FaceOcclusion faceOcclusionByNeighbor(
		const NeighborSnapshot &p_snapshot,
		const spk::VoxelRegistry &p_registry,
		const spk::Vector3Int &p_position,
		std::size_t p_worldPlaneIndex,
		const spk::VoxelShape &p_shape,
		spk::VoxelAxisPlane p_localPlane)
	{
		if (!p_shape.outerFaceLiesOnCellBoundary(p_localPlane))
		{
			return {};
		}

		const spk::VoxelCell *neighbor = p_snapshot.tryCell(p_position + PlaneOffsets[p_worldPlaneIndex]);
		if (neighbor == nullptr || neighbor->isEmpty())
		{
			return {};
		}

		const spk::VoxelShape &neighborShape = p_registry.shape(neighbor->id);
		if (neighborShape.isTransparent() && !transparentOcclusionCompatible(p_shape, neighborShape))
		{
			return {};
		}

		const spk::VoxelAxisPlane neighborLocalPlane =
			spk::VoxelWorldToLocalPlaneTable[spk::voxelTransformVariantIndex(neighbor->orientation, neighbor->flip)]
											[static_cast<std::size_t>(OppositePlanes[p_worldPlaneIndex])];
		const auto &neighborFace = neighborShape.renderFaces().outer(neighborLocalPlane);
		if (!neighborFace.has_value() || !neighborShape.outerFaceLiesOnCellBoundary(neighborLocalPlane))
		{
			return {};
		}
		if (neighborShape.outerFaceCoversCellBoundary(neighborLocalPlane))
		{
			return {.fullyOccluded = true};
		}

		return {
			.partialOccluder = &neighborShape.transformedOuterFace(neighborLocalPlane, neighbor->orientation, neighbor->flip)};
	}

	template <typename TFunction>
	bool forEachPolygon(
		const spk::VoxelShapeFace &p_face,
		const spk::Vector3 &p_offset,
		float p_alpha,
		TFunction &&p_function)
	{
		bool emitted = false;
		for (const spk::VoxelShapePolygon &polygon : p_face.polygons())
		{
			if (polygon.size() < 3)
			{
				continue;
			}
			p_function(polygon, p_offset, p_alpha);
			emitted = true;
		}
		return emitted;
	}

	template <typename TFunction>
	bool forEachVisiblePolygon(
		const spk::VoxelShapeFace &p_face,
		const spk::Vector3 &p_offset,
		float p_alpha,
		const FaceOcclusion &p_occlusion,
		spk::VoxelFaceRemnantCache &p_remnantCache,
		TFunction &&p_function)
	{
		if (p_occlusion.fullyOccluded)
		{
			return false;
		}
		if (p_occlusion.partialOccluder == nullptr)
		{
			return forEachPolygon(p_face, p_offset, p_alpha, std::forward<TFunction>(p_function));
		}

		bool emitted = false;
		for (const spk::VoxelShapePolygon &visible : p_remnantCache.remnants(p_face, *p_occlusion.partialOccluder))
		{
			p_function(visible, p_offset, p_alpha);
			emitted = true;
		}
		return emitted;
	}

	[[nodiscard]] bool cellIsFullyEnclosed(
		const NeighborSnapshot &p_snapshot,
		const spk::VoxelRegistry &p_registry,
		const spk::Vector3Int &p_position,
		const spk::VoxelShape &p_shape)
	{
		for (std::size_t planeIndex = 0; planeIndex < PlaneCount; ++planeIndex)
		{
			const spk::VoxelCell *neighbor = p_snapshot.tryCell(p_position + PlaneOffsets[planeIndex]);
			if (neighbor == nullptr || neighbor->isEmpty())
			{
				return false;
			}

			const spk::VoxelShape &neighborShape = p_registry.shape(neighbor->id);
			if (neighborShape.isTransparent() && !transparentOcclusionCompatible(p_shape, neighborShape))
			{
				return false;
			}
			const spk::VoxelAxisPlane neighborLocalPlane =
				spk::VoxelWorldToLocalPlaneTable[spk::voxelTransformVariantIndex(neighbor->orientation, neighbor->flip)]
												[static_cast<std::size_t>(OppositePlanes[planeIndex])];
			const auto &neighborFace = neighborShape.renderFaces().outer(neighborLocalPlane);
			if (!neighborFace.has_value() ||
				!neighborShape.outerFaceLiesOnCellBoundary(neighborLocalPlane) ||
				!neighborShape.outerFaceCoversCellBoundary(neighborLocalPlane))
			{
				return false;
			}
		}
		return true;
	}

	[[nodiscard]] float shapeAlpha(const spk::VoxelShape &p_shape)
	{
		return 1.0f - p_shape.transparency();
	}

	template <typename TFunction>
	void planVisiblePolygons(
		const spk::VoxelGrid &p_grid,
		const spk::VoxelRegistry &p_registry,
		const NeighborSnapshot &p_snapshot,
		spk::VoxelFaceRemnantCache &p_remnantCache,
		TFunction &&p_function)
	{
		std::size_t cellIndex = 0;
		for (int y = 0; y < p_grid.size().y; ++y)
		{
			for (int z = 0; z < p_grid.size().z; ++z)
			{
				for (int x = 0; x < p_grid.size().x; ++x)
				{
					const spk::Vector3Int position{x, y, z};
					const spk::VoxelCell &cell = p_grid.cells()[cellIndex++];
					if (cell.isEmpty())
					{
						continue;
					}

					const spk::VoxelShape &shape = p_registry.shape(cell.id);
					if (shape.transparency() >= 1.0f)
					{
						continue;
					}

					const float alpha = shapeAlpha(shape);
					const spk::VoxelShapeFaceSet &faces = shape.renderFaces();
					const auto &worldToLocal =
						spk::VoxelWorldToLocalPlaneTable[spk::voxelTransformVariantIndex(cell.orientation, cell.flip)];
					for (std::size_t planeIndex = 0; planeIndex < PlaneCount; ++planeIndex)
					{
						const spk::VoxelAxisPlane localPlane = worldToLocal[planeIndex];
						if (!faces.outer(localPlane).has_value())
						{
							continue;
						}

						const FaceOcclusion occlusion = faceOcclusionByNeighbor(
							p_snapshot,
							p_registry,
							position,
							planeIndex,
							shape,
							localPlane);
						forEachVisiblePolygon(
							shape.transformedOuterFace(localPlane, cell.orientation, cell.flip),
							spk::Vector3(position),
							alpha,
							occlusion,
							p_remnantCache,
							p_function);
					}

					// Enclosure only gates inner-face emission, so shapes without inner
					// faces (every full cube) never pay the six-neighbor probe.
					// Inner-only shapes retain their unconditional behavior. Mixed shapes
					// suppress inner geometry only when all six boundary planes are
					// actually sealed by complete, occlusion-compatible neighbor faces.
					if (!faces.innerFaces.empty() &&
						(!shape.hasOuterFaces() || !cellIsFullyEnclosed(p_snapshot, p_registry, position, shape)))
					{
						for (std::size_t faceIndex = 0; faceIndex < faces.innerFaces.size(); ++faceIndex)
						{
							forEachPolygon(
								shape.transformedInnerFace(faceIndex, cell.orientation, cell.flip),
								spk::Vector3(position),
								alpha,
								p_function);
						}
					}
				}
			}
		}
	}

	void checkedAdd(std::size_t &p_target, std::size_t p_value, const char *p_label)
	{
		if (p_value > std::numeric_limits<std::size_t>::max() - p_target)
		{
			throw std::overflow_error(std::string("Voxel mesh ") + p_label + " count overflow");
		}
		p_target += p_value;
	}
}

namespace spk
{
	spk::VoxelAxisPlane VoxelMesher::mapWorldPlaneToLocal(
		spk::VoxelAxisPlane p_worldPlane,
		spk::VoxelOrientation p_orientation,
		spk::VoxelFlip p_flip)
	{
		return spk::mapWorldPlaneToLocal(p_worldPlane, p_orientation, p_flip);
	}

	spk::VoxelRenderMeshes VoxelMesher::buildRenderMesh(const spk::VoxelGrid &p_grid, const spk::VoxelRegistry &p_registry)
	{
		return _buildRenderMesh(p_grid, p_registry, nullptr, {});
	}

	spk::VoxelRenderMeshes VoxelMesher::buildRenderMesh(
		const spk::VoxelGrid &p_grid,
		const spk::VoxelRegistry &p_registry,
		const spk::IVoxelCellLookup &p_worldLookup,
		const spk::Vector3Int &p_worldOrigin)
	{
		return _buildRenderMesh(p_grid, p_registry, &p_worldLookup, p_worldOrigin);
	}

	spk::VoxelRenderMeshes VoxelMesher::_buildRenderMesh(
		const spk::VoxelGrid &p_grid,
		const spk::VoxelRegistry &p_registry,
		const spk::IVoxelCellLookup *p_worldLookup,
		const spk::Vector3Int &p_worldOrigin)
	{
		const NeighborSnapshot snapshot(p_grid, p_worldLookup, p_worldOrigin);
		spk::VoxelFaceRemnantCache remnantCache;

		std::array<std::vector<PlannedPolygon>, 2> plans; // [0] opaque, [1] transparent
		{
			// Size the plan vectors once from the shapes' polygon counts. Occlusion culls
			// entries and partial subtraction can split polygons, so this is an estimate,
			// not a bound — but a close one, and only reallocation cost depends on it.
			std::array<std::size_t, 2> estimates{};
			for (const spk::VoxelCell &cell : p_grid.cells())
			{
				if (cell.isEmpty())
				{
					continue;
				}
				const spk::VoxelShape &shape = p_registry.shape(cell.id);
				if (shape.transparency() >= 1.0f)
				{
					continue;
				}
				const spk::VoxelShapeFaceSet &faces = shape.renderFaces();
				std::size_t polygons = 0;
				for (const auto &outer : faces.outerShell)
				{
					if (outer.has_value())
					{
						polygons += outer->size();
					}
				}
				for (const spk::VoxelShapeFace &inner : faces.innerFaces)
				{
					polygons += inner.size();
				}
				estimates[shape.isTransparent() ? 1 : 0] += polygons;
			}
			plans[0].reserve(estimates[0]);
			plans[1].reserve(estimates[1]);
		}

		planVisiblePolygons(
			p_grid,
			p_registry,
			snapshot,
			remnantCache,
			[&plans](const spk::VoxelShapePolygon &p_polygon, const spk::Vector3 &p_offset, float p_alpha) {
				plans[p_alpha < 1.0f ? 1 : 0].push_back({&p_polygon, p_offset, p_alpha});
			});

		struct MeshCounts
		{
			std::size_t vertices = 0;
			std::size_t indexes = 0;
			std::size_t shapes = 0;
		};
		std::array<MeshCounts, 2> counts;
		for (std::size_t meshIndex = 0; meshIndex < plans.size(); ++meshIndex)
		{
			for (const PlannedPolygon &planned : plans[meshIndex])
			{
				checkedAdd(counts[meshIndex].vertices, planned.polygon->size(), "vertex");
				const std::size_t triangleCount = planned.polygon->size() - 2;
				if (triangleCount > std::numeric_limits<std::size_t>::max() / 3)
				{
					throw std::overflow_error("Voxel mesh index count overflow");
				}
				checkedAdd(counts[meshIndex].indexes, triangleCount * 3, "index");
				checkedAdd(counts[meshIndex].shapes, 1, "shape");
			}
		}

		struct MeshEmitter
		{
			spk::VoxelMesh3D::Builder builder;
			std::span<spk::VoxelVertex3D> vertices;
			std::span<std::uint32_t> indexes;
			std::size_t vertexCursor = 0;
			std::size_t indexCursor = 0;

			explicit MeshEmitter(const MeshCounts &p_counts)
			{
				if (p_counts.vertices > std::numeric_limits<std::uint32_t>::max())
				{
					throw std::overflow_error("Voxel mesh exceeds the 32-bit index range");
				}
				builder.resize(p_counts.vertices, p_counts.indexes, p_counts.shapes);
				vertices = builder.vertices().cast<spk::VoxelVertex3D>();
				indexes = builder.indexes().cast<std::uint32_t>();
			}
		};
		std::array<MeshEmitter, 2> emitters{MeshEmitter(counts[0]), MeshEmitter(counts[1])};

		for (std::size_t meshIndex = 0; meshIndex < plans.size(); ++meshIndex)
		{
			MeshEmitter &target = emitters[meshIndex];
			for (const PlannedPolygon &planned : plans[meshIndex])
			{
				const spk::VoxelShapePolygon &polygon = *planned.polygon;
				const std::size_t triangleCount = polygon.size() - 2;
				const std::size_t indexCount = triangleCount * 3;
				if (polygon.size() > target.vertices.size() - target.vertexCursor ||
					indexCount > target.indexes.size() - target.indexCursor)
				{
					throw std::logic_error("Voxel mesh plan exceeds its allocated buffers");
				}

				const std::uint32_t first = static_cast<std::uint32_t>(target.vertexCursor);
				for (const spk::VoxelShapeVertex &vertex : polygon)
				{
					target.vertices[target.vertexCursor++] = {
						vertex.position + planned.offset,
						polygon.normal(),
						vertex.data,
						planned.alpha};
				}
				for (std::size_t index = 1; index + 1 < polygon.size(); ++index)
				{
					target.indexes[target.indexCursor++] = first;
					target.indexes[target.indexCursor++] = first + static_cast<std::uint32_t>(index);
					target.indexes[target.indexCursor++] = first + static_cast<std::uint32_t>(index + 1);
				}
			}
		}

		return {
			.opaque = std::move(emitters[0].builder).bake(),
			.transparent = std::move(emitters[1].builder).bake()};
	}
}
