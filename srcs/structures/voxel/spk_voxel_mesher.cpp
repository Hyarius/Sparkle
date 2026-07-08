#include "structures/voxel/spk_voxel_mesher.hpp"

#include <array>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace
{
	using VisibilityMask = std::uint8_t;

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
	constexpr VisibilityMask InnerFacesMask = VisibilityMask{1U << 6U};
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

	[[nodiscard]] constexpr VisibilityMask outerFaceMask(std::size_t p_planeIndex)
	{
		return static_cast<VisibilityMask>(1U << p_planeIndex);
	}

	[[nodiscard]] bool isFaceOccludedByNeighbor(
		const spk::VoxelGrid &p_grid,
		const spk::VoxelRegistry &p_registry,
		const spk::IVoxelCellLookup *p_worldLookup,
		const spk::Vector3Int &p_worldOrigin,
		const spk::Vector3Int &p_position,
		spk::VoxelAxisPlane p_worldPlane,
		const spk::VoxelShape &p_shape,
		spk::VoxelAxisPlane p_localPlane)
	{
		if (!p_shape.outerFaceLiesOnCellBoundary(p_localPlane))
		{
			return false;
		}

		const std::size_t worldPlaneIndex = static_cast<std::size_t>(p_worldPlane);
		const spk::Vector3Int neighborPosition = p_position + PlaneOffsets[worldPlaneIndex];
		const spk::VoxelCell *neighbor = nullptr;
		if (p_grid.isWithinBounds(neighborPosition))
		{
			const spk::Vector3Int &size = p_grid.size();
			const std::size_t neighborIndex = static_cast<std::size_t>(neighborPosition.x) +
											  static_cast<std::size_t>(size.x) *
												  (static_cast<std::size_t>(neighborPosition.z) +
												   static_cast<std::size_t>(size.z) * static_cast<std::size_t>(neighborPosition.y));
			neighbor = &p_grid.cells()[neighborIndex];
		}
		else if (p_worldLookup != nullptr)
		{
			neighbor = p_worldLookup->tryRenderableCell(p_worldOrigin + neighborPosition);
		}
		if (neighbor == nullptr || neighbor->isEmpty())
		{
			return false;
		}

		const spk::VoxelShape &neighborShape = p_registry.shape(neighbor->id);
		const spk::VoxelAxisPlane neighborLocalPlane =
			planeMapping(neighbor->orientation, neighbor->flip)[static_cast<std::size_t>(OppositePlanes[worldPlaneIndex])];
		const auto &neighborFace = neighborShape.renderFaces().outer(neighborLocalPlane);
		return neighborFace.has_value() && neighborShape.outerFaceCoversCellBoundary(neighborLocalPlane);
	}

	template <typename TFunction>
	void forEachPolygon(
		const spk::VoxelShapeFace &p_face,
		const spk::Vector3 &p_offset,
		TFunction &&p_function)
	{
		for (const spk::VoxelShapePolygon &polygon : p_face.polygons())
		{
			if (polygon.size() < 3)
			{
				continue;
			}

			p_function(polygon, polygon.normal(), p_offset);
		}
	}

	template <typename TFunction>
	[[nodiscard]] std::vector<VisibilityMask> computeVisibilityMasks(
		const spk::VoxelGrid &p_grid,
		const spk::VoxelRegistry &p_registry,
		const spk::IVoxelCellLookup *p_worldLookup,
		const spk::Vector3Int &p_worldOrigin,
		TFunction &&p_function)
	{
		std::vector<VisibilityMask> result(p_grid.cells().size(), VisibilityMask{0});
		std::size_t cellIndex = 0;

		for (int y = 0; y < p_grid.size().y; ++y)
		{
			for (int z = 0; z < p_grid.size().z; ++z)
			{
				for (int x = 0; x < p_grid.size().x; ++x)
				{
					const std::size_t currentCellIndex = cellIndex++;
					const spk::Vector3Int position{x, y, z};
					const spk::VoxelCell &cell = p_grid.cells()[currentCellIndex];
					if (cell.isEmpty())
					{
						continue;
					}

					const spk::VoxelShape &shape = p_registry.shape(cell.id);
					const spk::VoxelShapeFaceSet &faces = shape.renderFaces();
					const auto &localPlanes = planeMapping(cell.orientation, cell.flip);
					VisibilityMask visibility = 0;
					bool hasVisibleShell = false;
					for (std::size_t planeIndex = 0; planeIndex < WorldPlanes.size(); ++planeIndex)
					{
						const spk::VoxelAxisPlane worldPlane = WorldPlanes[planeIndex];
						const spk::VoxelAxisPlane localPlane = localPlanes[planeIndex];
						const auto &face = faces.outer(localPlane);
						if (!face.has_value() || isFaceOccludedByNeighbor(
													 p_grid, p_registry, p_worldLookup, p_worldOrigin, position, worldPlane, shape, localPlane))
						{
							continue;
						}
						visibility |= outerFaceMask(planeIndex);
						hasVisibleShell = true;
						forEachPolygon(*face, spk::Vector3(position), p_function);
					}

					if (hasVisibleShell || !shape.hasOuterFaces())
					{
						visibility |= InnerFacesMask;
						for (const spk::VoxelShapeFace &face : faces.innerFaces)
						{
							forEachPolygon(face, spk::Vector3(position), p_function);
						}
					}
					result[currentCellIndex] = visibility;
				}
			}
		}
		return result;
	}

	template <typename TFunction>
	void forEachMaskedPolygon(
		const spk::VoxelGrid &p_grid,
		const spk::VoxelRegistry &p_registry,
		const std::vector<VisibilityMask> &p_visibilityMasks,
		TFunction &&p_function)
	{
		if (p_visibilityMasks.size() != p_grid.cells().size())
		{
			throw std::invalid_argument("Voxel visibility mask size does not match the grid");
		}

		std::size_t cellIndex = 0;
		for (int y = 0; y < p_grid.size().y; ++y)
		{
			for (int z = 0; z < p_grid.size().z; ++z)
			{
				for (int x = 0; x < p_grid.size().x; ++x)
				{
					const std::size_t currentCellIndex = cellIndex++;
					const VisibilityMask visibility = p_visibilityMasks[currentCellIndex];
					if (visibility == 0)
					{
						continue;
					}

					const spk::Vector3Int position{x, y, z};
					const spk::VoxelCell &cell = p_grid.cells()[currentCellIndex];
					const spk::VoxelShape &shape = p_registry.shape(cell.id);
					const spk::VoxelShapeFaceSet &faces = shape.renderFaces();
					const auto &localPlanes = planeMapping(cell.orientation, cell.flip);
					for (std::size_t planeIndex = 0; planeIndex < WorldPlanes.size(); ++planeIndex)
					{
						if ((visibility & outerFaceMask(planeIndex)) == 0)
						{
							continue;
						}

						const spk::VoxelAxisPlane localPlane = localPlanes[planeIndex];
						const auto &face = faces.outer(localPlane);
						if (!face.has_value())
						{
							throw std::logic_error("Voxel visibility mask references a missing outer face");
						}
						forEachPolygon(
							shape.transformedOuterFace(localPlane, cell.orientation, cell.flip),
							spk::Vector3(position),
							p_function);
					}

					if ((visibility & InnerFacesMask) != 0)
					{
						for (std::size_t faceIndex = 0; faceIndex < faces.innerFaces.size(); ++faceIndex)
						{
							forEachPolygon(
								shape.transformedInnerFace(faceIndex, cell.orientation, cell.flip),
								spk::Vector3(position),
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

	spk::TextureMesh3D VoxelMesher::buildRenderMesh(const spk::VoxelGrid &p_grid, const spk::VoxelRegistry &p_registry)
	{
		return _buildRenderMesh(p_grid, p_registry, nullptr, {});
	}

	spk::TextureMesh3D VoxelMesher::buildRenderMesh(
		const spk::VoxelGrid &p_grid,
		const spk::VoxelRegistry &p_registry,
		const spk::IVoxelCellLookup &p_worldLookup,
		const spk::Vector3Int &p_worldOrigin)
	{
		return _buildRenderMesh(p_grid, p_registry, &p_worldLookup, p_worldOrigin);
	}

	spk::TextureMesh3D VoxelMesher::_buildRenderMesh(
		const spk::VoxelGrid &p_grid,
		const spk::VoxelRegistry &p_registry,
		const spk::IVoxelCellLookup *p_worldLookup,
		const spk::Vector3Int &p_worldOrigin)
	{
		struct MeshCounts
		{
			std::size_t vertices = 0;
			std::size_t indexes = 0;
			std::size_t shapes = 0;
		} counts;

		const std::vector<VisibilityMask> visibilityMasks = computeVisibilityMasks(
			p_grid,
			p_registry,
			p_worldLookup,
			p_worldOrigin,
			[&counts](const spk::VoxelShapePolygon &p_polygon, const spk::Vector3 &, const spk::Vector3 &) {
				checkedAdd(counts.vertices, p_polygon.size(), "vertex");
				const std::size_t triangleCount = p_polygon.size() - 2;
				if (triangleCount > std::numeric_limits<std::size_t>::max() / 3)
				{
					throw std::overflow_error("Voxel mesh index count overflow");
				}
				checkedAdd(counts.indexes, triangleCount * 3, "index");
				checkedAdd(counts.shapes, 1, "shape");
			});
		if (counts.vertices > std::numeric_limits<std::uint32_t>::max())
		{
			throw std::overflow_error("Voxel mesh exceeds the 32-bit index range");
		}

		spk::TextureMesh3D::Builder builder;
		builder.resize(counts.vertices, counts.indexes, counts.shapes);
		std::span<spk::TextureVertex3D> vertices = builder.vertices().cast<spk::TextureVertex3D>();
		std::span<std::uint32_t> indexes = builder.indexes().cast<std::uint32_t>();
		std::size_t vertexCursor = 0;
		std::size_t indexCursor = 0;

		forEachMaskedPolygon(
			p_grid,
			p_registry,
			visibilityMasks,
			[&](const spk::VoxelShapePolygon &p_polygon, const spk::Vector3 &p_normal, const spk::Vector3 &p_offset) {
				const std::uint32_t first = static_cast<std::uint32_t>(vertexCursor);
				for (const spk::VoxelShapeVertex &vertex : p_polygon)
				{
					vertices[vertexCursor++] = {vertex.position + p_offset, p_normal, vertex.data};
				}
				for (std::size_t index = 1; index + 1 < p_polygon.size(); ++index)
				{
					indexes[indexCursor++] = first;
					indexes[indexCursor++] = first + static_cast<std::uint32_t>(index);
					indexes[indexCursor++] = first + static_cast<std::uint32_t>(index + 1);
				}
			});

		if (vertexCursor != counts.vertices || indexCursor != counts.indexes)
		{
			throw std::logic_error("Voxel mesh count and emission passes disagree");
		}
		return std::move(builder).bake();
	}
}
