#include "structures/voxel/spk_voxel_mesher.hpp"
#include "structures/voxel/spk_voxel_orientation.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iterator>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace
{
	constexpr std::array WorldPlanes = {
		spk::VoxelAxisPlane::PositiveX,
		spk::VoxelAxisPlane::NegativeX,
		spk::VoxelAxisPlane::PositiveY,
		spk::VoxelAxisPlane::NegativeY,
		spk::VoxelAxisPlane::PositiveZ,
		spk::VoxelAxisPlane::NegativeZ};
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
	class NeighborSnapshot
	{
	private:
		const spk::VoxelGrid &_grid;
		spk::Vector3Int _worldOrigin;
		std::unordered_map<spk::Vector3Int, std::optional<spk::VoxelCell>> _externalCells;

		[[nodiscard]] const spk::VoxelCell *_gridCell(const spk::Vector3Int &p_position) const
		{
			const spk::Vector3Int &size = _grid.size();
			const std::size_t index = static_cast<std::size_t>(p_position.x) +
									  static_cast<std::size_t>(size.x) *
										  (static_cast<std::size_t>(p_position.z) +
										   static_cast<std::size_t>(size.z) * static_cast<std::size_t>(p_position.y));
			return &_grid.cells()[index];
		}

	public:
		NeighborSnapshot(
			const spk::VoxelGrid &p_grid,
			const spk::IVoxelCellLookup *p_worldLookup,
			const spk::Vector3Int &p_worldOrigin) :
			_grid(p_grid),
			_worldOrigin(p_worldOrigin)
		{
			if (p_worldLookup == nullptr)
			{
				return;
			}

			std::size_t cellIndex = 0;
			for (int y = 0; y < p_grid.size().y; ++y)
			{
				for (int z = 0; z < p_grid.size().z; ++z)
				{
					for (int x = 0; x < p_grid.size().x; ++x)
					{
						const spk::VoxelCell &cell = p_grid.cells()[cellIndex++];
						if (cell.isEmpty())
						{
							continue;
						}

						const spk::Vector3Int position{x, y, z};
						for (const spk::Vector3Int &offset : PlaneOffsets)
						{
							const spk::Vector3Int neighborPosition = position + offset;
							if (p_grid.isWithinBounds(neighborPosition))
							{
								continue;
							}

							const spk::Vector3Int worldPosition = p_worldOrigin + neighborPosition;
							if (_externalCells.contains(worldPosition))
							{
								continue;
							}

							const spk::VoxelCell *observed = p_worldLookup->tryRenderableCell(worldPosition);
							_externalCells.emplace(
								worldPosition,
								observed == nullptr ? std::optional<spk::VoxelCell>{} : std::optional<spk::VoxelCell>{*observed});
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

			const auto iterator = _externalCells.find(_worldOrigin + p_gridPosition);
			if (iterator == _externalCells.end() || !iterator->second.has_value())
			{
				return nullptr;
			}
			return &*iterator->second;
		}
	};

	struct FaceOcclusion
	{
		bool fullyOccluded = false;
		const spk::VoxelShapeFace *partialOccluder = nullptr;
	};

	struct PlannedPolygon
	{
		spk::VoxelShapePolygon polygon;
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
		spk::VoxelAxisPlane p_worldPlane,
		const spk::VoxelShape &p_shape,
		spk::VoxelAxisPlane p_localPlane)
	{
		if (!p_shape.outerFaceLiesOnCellBoundary(p_localPlane))
		{
			return {};
		}

		const std::size_t worldPlaneIndex = static_cast<std::size_t>(p_worldPlane);
		const spk::VoxelCell *neighbor = p_snapshot.tryCell(p_position + PlaneOffsets[worldPlaneIndex]);
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
			spk::mapWorldPlaneToLocal(OppositePlanes[worldPlaneIndex], neighbor->orientation, neighbor->flip);
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

	[[nodiscard]] spk::Vector2 interpolateUV(
		const spk::Vector2 &p_from,
		const spk::Vector2 &p_to,
		float p_interpolation) noexcept
	{
		return p_from + (p_to - p_from) * p_interpolation;
	}

	[[nodiscard]] std::vector<spk::VoxelShapePolygon> visibleDifference(
		const spk::VoxelShapePolygon &p_polygon,
		const spk::VoxelShapeFace &p_occluder)
	{
		// The admitted built-in boundary polygons are convex. For a custom non-convex
		// polygon, retain the original face rather than risk deleting visible geometry.
		if (!p_polygon.isConvex(spk::VoxelShape::BoundaryEpsilon) ||
			std::ranges::any_of(p_occluder.polygons(), [](const spk::VoxelShapePolygon &p_candidate) {
				return !p_candidate.isConvex(spk::VoxelShape::BoundaryEpsilon);
			}))
		{
			return {p_polygon};
		}

		std::vector<spk::VoxelShapePolygon> visible = {p_polygon};
		for (const spk::VoxelShapePolygon &occludingPolygon : p_occluder.polygons())
		{
			std::vector<spk::VoxelShapePolygon> next;
			for (const spk::VoxelShapePolygon &piece : visible)
			{
				std::vector<spk::VoxelShapePolygon> difference =
					piece.subtractConvex(occludingPolygon, interpolateUV, spk::VoxelShape::BoundaryEpsilon);
				next.insert(next.end(), std::make_move_iterator(difference.begin()), std::make_move_iterator(difference.end()));
			}
			visible = std::move(next);
			if (visible.empty())
			{
				break;
			}
		}
		return visible;
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
		for (const spk::VoxelShapePolygon &polygon : p_face.polygons())
		{
			for (const spk::VoxelShapePolygon &visible :
				 visibleDifference(polygon, *p_occlusion.partialOccluder))
			{
				p_function(visible, p_offset, p_alpha);
				emitted = true;
			}
		}
		return emitted;
	}

	[[nodiscard]] bool cellIsFullyEnclosed(
		const NeighborSnapshot &p_snapshot,
		const spk::VoxelRegistry &p_registry,
		const spk::Vector3Int &p_position,
		const spk::VoxelShape &p_shape)
	{
		for (std::size_t planeIndex = 0; planeIndex < WorldPlanes.size(); ++planeIndex)
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
				spk::mapWorldPlaneToLocal(OppositePlanes[planeIndex], neighbor->orientation, neighbor->flip);
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
					for (std::size_t planeIndex = 0; planeIndex < WorldPlanes.size(); ++planeIndex)
					{
						const spk::VoxelAxisPlane localPlane =
							spk::mapWorldPlaneToLocal(WorldPlanes[planeIndex], cell.orientation, cell.flip);
						if (!faces.outer(localPlane).has_value())
						{
							continue;
						}

						const FaceOcclusion occlusion = faceOcclusionByNeighbor(
							p_snapshot,
							p_registry,
							position,
							WorldPlanes[planeIndex],
							shape,
							localPlane);
						forEachVisiblePolygon(
							shape.transformedOuterFace(localPlane, cell.orientation, cell.flip),
							spk::Vector3(position),
							alpha,
							occlusion,
							p_function);
					}

					// Inner-only shapes retain their unconditional behavior. Mixed shapes suppress
					// inner geometry only when all six boundary planes are actually sealed by
					// complete, occlusion-compatible neighbor faces.
					if (!shape.hasOuterFaces() || !cellIsFullyEnclosed(p_snapshot, p_registry, position, shape))
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
		std::array<std::vector<PlannedPolygon>, 2> plans; // [0] opaque, [1] transparent
		planVisiblePolygons(
			p_grid,
			p_registry,
			snapshot,
			[&plans](const spk::VoxelShapePolygon &p_polygon, const spk::Vector3 &p_offset, float p_alpha) {
				plans[p_alpha < 1.0f ? 1 : 0].push_back({p_polygon, p_offset, p_alpha});
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
				checkedAdd(counts[meshIndex].vertices, planned.polygon.size(), "vertex");
				const std::size_t triangleCount = planned.polygon.size() - 2;
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
				const std::size_t triangleCount = planned.polygon.size() - 2;
				const std::size_t indexCount = triangleCount * 3;
				if (planned.polygon.size() > target.vertices.size() - target.vertexCursor ||
					indexCount > target.indexes.size() - target.indexCursor)
				{
					throw std::logic_error("Voxel mesh plan exceeds its allocated buffers");
				}

				const std::uint32_t first = static_cast<std::uint32_t>(target.vertexCursor);
				for (const spk::VoxelShapeVertex &vertex : planned.polygon)
				{
					target.vertices[target.vertexCursor++] = {
						vertex.position + planned.offset,
						planned.polygon.normal(),
						vertex.data,
						planned.alpha};
				}
				for (std::size_t index = 1; index + 1 < planned.polygon.size(); ++index)
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
