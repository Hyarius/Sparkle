#include "structures/voxel/spk_voxel_mesher.hpp"

#include <algorithm>
#include <array>
#include <cmath>
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
	constexpr float FaceEpsilon = spk::VoxelShape::BoundaryEpsilon;

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
	constexpr std::array PlaneMappings = {
		// PositiveX orientation.
		std::array{
			std::array{spk::VoxelAxisPlane::PositiveZ, spk::VoxelAxisPlane::NegativeZ, spk::VoxelAxisPlane::PositiveY, spk::VoxelAxisPlane::NegativeY, spk::VoxelAxisPlane::NegativeX, spk::VoxelAxisPlane::PositiveX},
			std::array{spk::VoxelAxisPlane::PositiveZ, spk::VoxelAxisPlane::NegativeZ, spk::VoxelAxisPlane::NegativeY, spk::VoxelAxisPlane::PositiveY, spk::VoxelAxisPlane::NegativeX, spk::VoxelAxisPlane::PositiveX}},
		// PositiveZ orientation.
		std::array{
			std::array{spk::VoxelAxisPlane::PositiveX, spk::VoxelAxisPlane::NegativeX, spk::VoxelAxisPlane::PositiveY, spk::VoxelAxisPlane::NegativeY, spk::VoxelAxisPlane::PositiveZ, spk::VoxelAxisPlane::NegativeZ},
			std::array{spk::VoxelAxisPlane::PositiveX, spk::VoxelAxisPlane::NegativeX, spk::VoxelAxisPlane::NegativeY, spk::VoxelAxisPlane::PositiveY, spk::VoxelAxisPlane::PositiveZ, spk::VoxelAxisPlane::NegativeZ}},
		// NegativeX orientation.
		std::array{
			std::array{spk::VoxelAxisPlane::NegativeZ, spk::VoxelAxisPlane::PositiveZ, spk::VoxelAxisPlane::PositiveY, spk::VoxelAxisPlane::NegativeY, spk::VoxelAxisPlane::PositiveX, spk::VoxelAxisPlane::NegativeX},
			std::array{spk::VoxelAxisPlane::NegativeZ, spk::VoxelAxisPlane::PositiveZ, spk::VoxelAxisPlane::NegativeY, spk::VoxelAxisPlane::PositiveY, spk::VoxelAxisPlane::PositiveX, spk::VoxelAxisPlane::NegativeX}},
		// NegativeZ orientation.
		std::array{
			std::array{spk::VoxelAxisPlane::NegativeX, spk::VoxelAxisPlane::PositiveX, spk::VoxelAxisPlane::PositiveY, spk::VoxelAxisPlane::NegativeY, spk::VoxelAxisPlane::NegativeZ, spk::VoxelAxisPlane::PositiveZ},
			std::array{spk::VoxelAxisPlane::NegativeX, spk::VoxelAxisPlane::PositiveX, spk::VoxelAxisPlane::NegativeY, spk::VoxelAxisPlane::PositiveY, spk::VoxelAxisPlane::NegativeZ, spk::VoxelAxisPlane::PositiveZ}}};

	[[nodiscard]] const auto &planeMapping(spk::VoxelOrientation p_orientation, spk::VoxelFlip p_flip)
	{
		const std::size_t orientation = static_cast<std::size_t>(p_orientation);
		const std::size_t flip = static_cast<std::size_t>(p_flip);
		if (orientation >= PlaneMappings.size() || flip >= PlaneMappings.front().size())
		{
			throw std::invalid_argument("Invalid voxel orientation or flip");
		}
		return PlaneMappings[orientation][flip];
	}

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

	struct BoundaryPoint
	{
		float first = 0.0f;
		float second = 0.0f;
	};

	struct FaceOcclusion
	{
		bool fullyOccluded = false;
		const spk::VoxelShapeFace *partialOccluder = nullptr;
		spk::VoxelAxisPlane boundaryPlane = spk::VoxelAxisPlane::Count;
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
			planeMapping(neighbor->orientation, neighbor->flip)[static_cast<std::size_t>(OppositePlanes[worldPlaneIndex])];
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
			.partialOccluder = &neighborShape.transformedOuterFace(neighborLocalPlane, neighbor->orientation, neighbor->flip),
			.boundaryPlane = p_worldPlane};
	}

	[[nodiscard]] BoundaryPoint project(const spk::Vector3 &p_position, spk::VoxelAxisPlane p_plane)
	{
		switch (p_plane)
		{
		case spk::VoxelAxisPlane::PositiveX:
		case spk::VoxelAxisPlane::NegativeX:
			return {p_position.y, p_position.z};
		case spk::VoxelAxisPlane::PositiveY:
		case spk::VoxelAxisPlane::NegativeY:
			return {p_position.x, p_position.z};
		case spk::VoxelAxisPlane::PositiveZ:
		case spk::VoxelAxisPlane::NegativeZ:
			return {p_position.x, p_position.y};
		case spk::VoxelAxisPlane::Count:
			break;
		}
		throw std::invalid_argument("VoxelAxisPlane::Count is not a geometric plane");
	}

	[[nodiscard]] float cross(const BoundaryPoint &p_a, const BoundaryPoint &p_b, const BoundaryPoint &p_c)
	{
		return (p_b.first - p_a.first) * (p_c.second - p_a.second) -
			   (p_b.second - p_a.second) * (p_c.first - p_a.first);
	}

	[[nodiscard]] float signedArea(
		std::span<const spk::VoxelShapeVertex> p_vertices,
		spk::VoxelAxisPlane p_plane)
	{
		float doubledArea = 0.0f;
		for (std::size_t index = 0; index < p_vertices.size(); ++index)
		{
			const BoundaryPoint a = project(p_vertices[index].position, p_plane);
			const BoundaryPoint b = project(p_vertices[(index + 1) % p_vertices.size()].position, p_plane);
			doubledArea += a.first * b.second - b.first * a.second;
		}
		return doubledArea * 0.5f;
	}

	[[nodiscard]] bool sameVertex(
		const spk::VoxelShapeVertex &p_left,
		const spk::VoxelShapeVertex &p_right)
	{
		return p_left.position == p_right.position && p_left.data == p_right.data;
	}

	void appendDistinct(
		std::vector<spk::VoxelShapeVertex> &p_vertices,
		spk::VoxelShapeVertex p_vertex)
	{
		if (p_vertices.empty() || !sameVertex(p_vertices.back(), p_vertex))
		{
			p_vertices.push_back(std::move(p_vertex));
		}
	}

	[[nodiscard]] std::vector<spk::VoxelShapeVertex> cleanPolygon(
		std::vector<spk::VoxelShapeVertex> p_vertices,
		spk::VoxelAxisPlane p_plane)
	{
		if (p_vertices.size() > 1 && sameVertex(p_vertices.front(), p_vertices.back()))
		{
			p_vertices.pop_back();
		}

		bool changed = true;
		while (changed && p_vertices.size() >= 3)
		{
			changed = false;
			for (std::size_t index = 0; index < p_vertices.size(); ++index)
			{
				const BoundaryPoint previous = project(p_vertices[(index + p_vertices.size() - 1) % p_vertices.size()].position, p_plane);
				const BoundaryPoint current = project(p_vertices[index].position, p_plane);
				const BoundaryPoint next = project(p_vertices[(index + 1) % p_vertices.size()].position, p_plane);
				if (sameVertex(p_vertices[(index + p_vertices.size() - 1) % p_vertices.size()], p_vertices[index]) ||
					std::abs(cross(previous, current, next)) <= FaceEpsilon)
				{
					p_vertices.erase(p_vertices.begin() + static_cast<std::ptrdiff_t>(index));
					changed = true;
					break;
				}
			}
		}
		return p_vertices;
	}

	[[nodiscard]] bool isConvex(
		const spk::VoxelShapePolygon &p_polygon,
		spk::VoxelAxisPlane p_plane)
	{
		if (p_polygon.size() < 3 || std::abs(signedArea(p_polygon.vertices(), p_plane)) <= FaceEpsilon)
		{
			return false;
		}

		float turn = 0.0f;
		for (std::size_t index = 0; index < p_polygon.size(); ++index)
		{
			const float current = cross(
				project(p_polygon[index].position, p_plane),
				project(p_polygon[(index + 1) % p_polygon.size()].position, p_plane),
				project(p_polygon[(index + 2) % p_polygon.size()].position, p_plane));
			if (std::abs(current) <= FaceEpsilon)
			{
				continue;
			}
			if (turn != 0.0f && current * turn < 0.0f)
			{
				return false;
			}
			turn = current;
		}
		return turn != 0.0f;
	}

	[[nodiscard]] spk::VoxelShapeVertex interpolate(
		const spk::VoxelShapeVertex &p_from,
		const spk::VoxelShapeVertex &p_to,
		float p_t)
	{
		return {
			.position = p_from.position + (p_to.position - p_from.position) * p_t,
			.data = p_from.data + (p_to.data - p_from.data) * p_t};
	}

	[[nodiscard]] std::vector<spk::VoxelShapeVertex> clipHalfPlane(
		std::span<const spk::VoxelShapeVertex> p_vertices,
		const BoundaryPoint &p_edgeStart,
		const BoundaryPoint &p_edgeEnd,
		float p_orientation,
		bool p_keepInside,
		spk::VoxelAxisPlane p_plane)
	{
		std::vector<spk::VoxelShapeVertex> result;
		if (p_vertices.empty())
		{
			return result;
		}

		const auto distance = [&](const spk::VoxelShapeVertex &p_vertex) {
			return p_orientation * cross(p_edgeStart, p_edgeEnd, project(p_vertex.position, p_plane));
		};
		const auto accepted = [p_keepInside](float p_distance) {
			return p_keepInside ? p_distance >= -FaceEpsilon : p_distance <= FaceEpsilon;
		};

		spk::VoxelShapeVertex previous = p_vertices.back();
		float previousDistance = distance(previous);
		bool previousAccepted = accepted(previousDistance);
		for (const spk::VoxelShapeVertex &current : p_vertices)
		{
			const float currentDistance = distance(current);
			const bool currentAccepted = accepted(currentDistance);
			if (currentAccepted != previousAccepted)
			{
				const float denominator = previousDistance - currentDistance;
				if (std::abs(denominator) > FaceEpsilon)
				{
					appendDistinct(result, interpolate(previous, current, previousDistance / denominator));
				}
			}
			if (currentAccepted)
			{
				appendDistinct(result, current);
			}
			previous = current;
			previousDistance = currentDistance;
			previousAccepted = currentAccepted;
		}
		return cleanPolygon(std::move(result), p_plane);
	}

	[[nodiscard]] std::optional<spk::VoxelShapePolygon> bakePolygon(
		std::vector<spk::VoxelShapeVertex> p_vertices,
		spk::VoxelAxisPlane p_plane)
	{
		p_vertices = cleanPolygon(std::move(p_vertices), p_plane);
		if (p_vertices.size() < 3 || std::abs(signedArea(p_vertices, p_plane)) <= FaceEpsilon)
		{
			return std::nullopt;
		}

		spk::VoxelShapePolygon::Builder builder;
		builder.reserve(p_vertices.size());
		for (spk::VoxelShapeVertex &vertex : p_vertices)
		{
			builder.addVertex(std::move(vertex));
		}
		return std::move(builder).bake();
	}

	[[nodiscard]] std::vector<spk::VoxelShapePolygon> subtractConvex(
		const spk::VoxelShapePolygon &p_subject,
		const spk::VoxelShapePolygon &p_clip,
		spk::VoxelAxisPlane p_plane)
	{
		std::vector<spk::VoxelShapePolygon> result;
		std::vector<spk::VoxelShapeVertex> remaining(p_subject.begin(), p_subject.end());
		const float orientation = signedArea(p_clip.vertices(), p_plane) > 0.0f ? 1.0f : -1.0f;
		for (std::size_t edge = 0; edge < p_clip.size(); ++edge)
		{
			const BoundaryPoint start = project(p_clip[edge].position, p_plane);
			const BoundaryPoint end = project(p_clip[(edge + 1) % p_clip.size()].position, p_plane);
			if (std::optional<spk::VoxelShapePolygon> outside = bakePolygon(
					clipHalfPlane(remaining, start, end, orientation, false, p_plane),
					p_plane))
			{
				result.push_back(std::move(*outside));
			}

			remaining = clipHalfPlane(remaining, start, end, orientation, true, p_plane);
			if (remaining.size() < 3 || std::abs(signedArea(remaining, p_plane)) <= FaceEpsilon)
			{
				break;
			}
		}
		return result;
	}

	[[nodiscard]] std::vector<spk::VoxelShapePolygon> visibleDifference(
		const spk::VoxelShapePolygon &p_polygon,
		const spk::VoxelShapeFace &p_occluder,
		spk::VoxelAxisPlane p_plane)
	{
		// The admitted built-in boundary polygons are convex. For a custom non-convex
		// polygon, retain the original face rather than risk deleting visible geometry.
		if (!isConvex(p_polygon, p_plane) ||
			std::ranges::any_of(p_occluder.polygons(), [p_plane](const spk::VoxelShapePolygon &p_candidate) {
				return !isConvex(p_candidate, p_plane);
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
				std::vector<spk::VoxelShapePolygon> difference = subtractConvex(piece, occludingPolygon, p_plane);
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
				 visibleDifference(polygon, *p_occlusion.partialOccluder, p_occlusion.boundaryPlane))
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
				planeMapping(neighbor->orientation, neighbor->flip)[static_cast<std::size_t>(OppositePlanes[planeIndex])];
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
					const auto &localPlanes = planeMapping(cell.orientation, cell.flip);
					for (std::size_t planeIndex = 0; planeIndex < WorldPlanes.size(); ++planeIndex)
					{
						const spk::VoxelAxisPlane localPlane = localPlanes[planeIndex];
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
		const std::size_t plane = static_cast<std::size_t>(p_worldPlane);
		if (plane >= WorldPlanes.size())
		{
			throw std::invalid_argument("VoxelAxisPlane::Count is not a geometric plane");
		}
		return planeMapping(p_orientation, p_flip)[plane];
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
