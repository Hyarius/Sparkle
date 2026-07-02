#include "voxel/voxel_mesher.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>

namespace
{
	using FaceCacheKey = std::tuple<const pg::VoxelShapeFace *, pg::VoxelOrientation, pg::VoxelFlip>;

	[[nodiscard]] pg::VoxelAxisPlane opposite(pg::VoxelAxisPlane p_plane)
	{
		using P = pg::VoxelAxisPlane;
		switch (p_plane)
		{
		case P::PositiveX: return P::NegativeX;
		case P::NegativeX: return P::PositiveX;
		case P::PositiveY: return P::NegativeY;
		case P::NegativeY: return P::PositiveY;
		case P::PositiveZ: return P::NegativeZ;
		case P::NegativeZ: return P::PositiveZ;
		case P::Count: break;
		}
		throw std::invalid_argument("VoxelAxisPlane::Count is not a geometric plane");
	}

	[[nodiscard]] spk::Vector3Int planeOffset(pg::VoxelAxisPlane p_plane)
	{
		using P = pg::VoxelAxisPlane;
		switch (p_plane)
		{
		case P::PositiveX: return {1, 0, 0};
		case P::NegativeX: return {-1, 0, 0};
		case P::PositiveY: return {0, 1, 0};
		case P::NegativeY: return {0, -1, 0};
		case P::PositiveZ: return {0, 0, 1};
		case P::NegativeZ: return {0, 0, -1};
		case P::Count: break;
		}
		throw std::invalid_argument("VoxelAxisPlane::Count is not a geometric plane");
	}

	[[nodiscard]] pg::VoxelAxisPlane mapLocalPlaneToWorld(
		pg::VoxelAxisPlane p_localPlane,
		pg::VoxelOrientation p_orientation,
		pg::VoxelFlip p_flip)
	{
		using O = pg::VoxelOrientation;
		using P = pg::VoxelAxisPlane;

		if (p_localPlane == P::PositiveY || p_localPlane == P::NegativeY)
		{
			if (p_flip == pg::VoxelFlip::PositiveY)
			{
				return p_localPlane;
			}
			return opposite(p_localPlane);
		}

		switch (p_orientation)
		{
		case O::PositiveZ:
			return p_localPlane;
		case O::PositiveX:
			switch (p_localPlane)
			{
			case P::PositiveX: return P::NegativeZ;
			case P::NegativeX: return P::PositiveZ;
			case P::PositiveZ: return P::PositiveX;
			case P::NegativeZ: return P::NegativeX;
			default: break;
			}
			break;
		case O::NegativeZ:
			switch (p_localPlane)
			{
			case P::PositiveX: return P::NegativeX;
			case P::NegativeX: return P::PositiveX;
			case P::PositiveZ: return P::NegativeZ;
			case P::NegativeZ: return P::PositiveZ;
			default: break;
			}
			break;
		case O::NegativeX:
			switch (p_localPlane)
			{
			case P::PositiveX: return P::PositiveZ;
			case P::NegativeX: return P::NegativeZ;
			case P::PositiveZ: return P::NegativeX;
			case P::NegativeZ: return P::PositiveX;
			default: break;
			}
			break;
		}
		throw std::invalid_argument("Invalid local voxel plane");
	}

	[[nodiscard]] spk::Vector3 transformPosition(
		const spk::Vector3 &p_position,
		pg::VoxelOrientation p_orientation,
		pg::VoxelFlip p_flip)
	{
		spk::Vector3 result;
		switch (p_orientation)
		{
		case pg::VoxelOrientation::PositiveZ:
			result = p_position;
			break;
		case pg::VoxelOrientation::PositiveX:
			result = {p_position.z, p_position.y, 1.0f - p_position.x};
			break;
		case pg::VoxelOrientation::NegativeZ:
			result = {1.0f - p_position.x, p_position.y, 1.0f - p_position.z};
			break;
		case pg::VoxelOrientation::NegativeX:
			result = {1.0f - p_position.z, p_position.y, p_position.x};
			break;
		}

		if (p_flip == pg::VoxelFlip::NegativeY)
		{
			result.y = 1.0f - result.y;
		}
		return result;
	}

	[[nodiscard]] pg::VoxelShapeFace transformFace(
		const pg::VoxelShapeFace &p_face,
		pg::VoxelOrientation p_orientation,
		pg::VoxelFlip p_flip)
	{
		pg::VoxelShapeFace result = p_face;
		for (pg::VoxelShapePolygon &polygon : result.polygons)
		{
			for (pg::VoxelShapeVertex &vertex : polygon)
			{
				vertex.position = transformPosition(vertex.position, p_orientation, p_flip);
			}
			if (p_flip == pg::VoxelFlip::NegativeY)
			{
				std::ranges::reverse(polygon);
			}
		}
		return result;
	}

	class FaceTransformCache
	{
	private:
		std::map<FaceCacheKey, pg::VoxelShapeFace> _faces;

	public:
		[[nodiscard]] const pg::VoxelShapeFace &get(
			const pg::VoxelShapeFace &p_face,
			pg::VoxelOrientation p_orientation,
			pg::VoxelFlip p_flip)
		{
			const FaceCacheKey key{&p_face, p_orientation, p_flip};
			auto [iterator, inserted] = _faces.try_emplace(key);
			if (inserted)
			{
				iterator->second = transformFace(p_face, p_orientation, p_flip);
			}
			return iterator->second;
		}
	};

	[[nodiscard]] float planeCoordinate(const spk::Vector3 &p_position, pg::VoxelAxisPlane p_plane)
	{
		switch (p_plane)
		{
		case pg::VoxelAxisPlane::PositiveX:
		case pg::VoxelAxisPlane::NegativeX: return p_position.x;
		case pg::VoxelAxisPlane::PositiveY:
		case pg::VoxelAxisPlane::NegativeY: return p_position.y;
		case pg::VoxelAxisPlane::PositiveZ:
		case pg::VoxelAxisPlane::NegativeZ: return p_position.z;
		case pg::VoxelAxisPlane::Count: break;
		}
		throw std::invalid_argument("VoxelAxisPlane::Count is not a geometric plane");
	}

	[[nodiscard]] bool isPositivePlane(pg::VoxelAxisPlane p_plane)
	{
		return p_plane == pg::VoxelAxisPlane::PositiveX ||
			   p_plane == pg::VoxelAxisPlane::PositiveY ||
			   p_plane == pg::VoxelAxisPlane::PositiveZ;
	}

	[[nodiscard]] bool liesOnCellBoundary(const pg::VoxelShapeFace &p_face, pg::VoxelAxisPlane p_plane)
	{
		const float expected = isPositivePlane(p_plane) ? 1.0f : 0.0f;
		for (const pg::VoxelShapePolygon &polygon : p_face.polygons)
		{
			for (const pg::VoxelShapeVertex &vertex : polygon)
			{
				if (std::abs(planeCoordinate(vertex.position, p_plane) - expected) > 0.0001f)
				{
					return false;
				}
			}
		}
		return !p_face.polygons.empty();
	}

	[[nodiscard]] bool isFullBoundaryQuad(const pg::VoxelShapeFace &p_face, pg::VoxelAxisPlane p_plane)
	{
		if (p_face.polygons.size() != 1 || p_face.polygons.front().size() != 4 ||
			!liesOnCellBoundary(p_face, p_plane))
		{
			return false;
		}

		float firstMinimum = 1.0f;
		float firstMaximum = 0.0f;
		float secondMinimum = 1.0f;
		float secondMaximum = 0.0f;
		for (const pg::VoxelShapeVertex &vertex : p_face.polygons.front())
		{
			float first = 0.0f;
			float second = 0.0f;
			switch (p_plane)
			{
			case pg::VoxelAxisPlane::PositiveX:
			case pg::VoxelAxisPlane::NegativeX:
				first = vertex.position.y;
				second = vertex.position.z;
				break;
			case pg::VoxelAxisPlane::PositiveY:
			case pg::VoxelAxisPlane::NegativeY:
				first = vertex.position.x;
				second = vertex.position.z;
				break;
			case pg::VoxelAxisPlane::PositiveZ:
			case pg::VoxelAxisPlane::NegativeZ:
				first = vertex.position.x;
				second = vertex.position.y;
				break;
			case pg::VoxelAxisPlane::Count:
				break;
			}
			firstMinimum = std::min(firstMinimum, first);
			firstMaximum = std::max(firstMaximum, first);
			secondMinimum = std::min(secondMinimum, second);
			secondMaximum = std::max(secondMaximum, second);
		}
		return std::abs(firstMinimum) < 0.0001f && std::abs(secondMinimum) < 0.0001f &&
			   std::abs(firstMaximum - 1.0f) < 0.0001f && std::abs(secondMaximum - 1.0f) < 0.0001f;
	}

	[[nodiscard]] bool isFaceOccludedByNeighbor(
		const pg::VoxelGrid &p_grid,
		const pg::VoxelRegistry &p_registry,
		const pg::ICellLookup *p_worldLookup,
		const spk::Vector3Int &p_worldOrigin,
		const spk::Vector3Int &p_position,
		pg::VoxelAxisPlane p_worldPlane,
		const pg::VoxelCell &p_cell,
		const pg::VoxelShapeFace &p_localFace)
	{
		const pg::VoxelAxisPlane localPlane = pg::VoxelMesher::mapWorldPlaneToLocal(
			p_worldPlane, p_cell.orientation, p_cell.flip);
		if (!liesOnCellBoundary(p_localFace, localPlane))
		{
			return false;
		}

		const spk::Vector3Int neighborPosition = p_position + planeOffset(p_worldPlane);
		const pg::VoxelCell *neighbor = nullptr;
		if (p_grid.isWithinBounds(neighborPosition))
		{
			neighbor = &p_grid.cell(neighborPosition);
		}
		else if (p_worldLookup != nullptr)
		{
			neighbor = p_worldLookup->tryCell(p_worldOrigin + neighborPosition);
		}
		if (neighbor == nullptr || neighbor->isEmpty())
		{
			return false;
		}

		const pg::VoxelDefinition &neighborDefinition = p_registry.get(neighbor->id);
		const pg::VoxelAxisPlane neighborLocalPlane = pg::VoxelMesher::mapWorldPlaneToLocal(
			opposite(p_worldPlane), neighbor->orientation, neighbor->flip);
		const auto &neighborFace = neighborDefinition.shape->renderFaces().outer(neighborLocalPlane);
		return neighborFace.has_value() && isFullBoundaryQuad(*neighborFace, neighborLocalPlane);
	}

	void emitFace(
		pg::Mesh3D &p_mesh,
		const pg::VoxelShapeFace &p_face,
		const spk::Vector3 &p_offset,
		const pg::AtlasCell *p_maskCell = nullptr)
	{
		for (const pg::VoxelShapePolygon &polygon : p_face.polygons)
		{
			if (polygon.size() < 3)
			{
				continue;
			}

			const spk::Vector3 normal =
				(polygon[1].position - polygon[0].position)
					.cross(polygon[2].position - polygon[0].position)
					.normalized();

			float minimumU = 0.0f;
			float maximumU = 1.0f;
			float minimumV = 0.0f;
			float maximumV = 1.0f;
			if (p_maskCell != nullptr)
			{
				minimumU = maximumU = polygon.front().uv.x;
				minimumV = maximumV = polygon.front().uv.y;
				for (const pg::VoxelShapeVertex &vertex : polygon)
				{
					minimumU = std::min(minimumU, vertex.uv.x);
					maximumU = std::max(maximumU, vertex.uv.x);
					minimumV = std::min(minimumV, vertex.uv.y);
					maximumV = std::max(maximumV, vertex.uv.y);
				}
			}

			std::vector<pg::MeshVertex3D> vertices;
			vertices.reserve(polygon.size());
			for (const pg::VoxelShapeVertex &vertex : polygon)
			{
				spk::Vector2 uv = vertex.uv;
				if (p_maskCell != nullptr)
				{
					const float localU = maximumU > minimumU ? (uv.x - minimumU) / (maximumU - minimumU) : 0.0f;
					const float localV = maximumV > minimumV ? (uv.y - minimumV) / (maximumV - minimumV) : 0.0f;
					uv = {
						(static_cast<float>(p_maskCell->column) + localU) / 4.0f,
						(static_cast<float>(p_maskCell->row) + localV) / 4.0f};
				}
				vertices.push_back({vertex.position + p_offset, normal, uv});
			}
			p_mesh.addShape(vertices);
		}
	}
}

namespace pg
{
	VoxelGridCellLookup::VoxelGridCellLookup(const VoxelGrid &p_grid, const VoxelRegistry &p_registry) :
		_grid(p_grid),
		_registry(p_registry)
	{
	}

	const VoxelCell *VoxelGridCellLookup::tryCell(const spk::Vector3Int &p_position) const
	{
		return _grid.isWithinBounds(p_position) ? &_grid.cell(p_position) : nullptr;
	}

	const VoxelDefinition *VoxelGridCellLookup::tryDefinition(const VoxelCell &p_cell) const
	{
		return p_cell.isEmpty() ? nullptr : _registry.tryGet(p_cell.id);
	}

	VoxelAxisPlane VoxelMesher::mapWorldPlaneToLocal(
		VoxelAxisPlane p_worldPlane,
		VoxelOrientation p_orientation,
		VoxelFlip p_flip)
	{
		if (p_worldPlane == VoxelAxisPlane::Count)
		{
			throw std::invalid_argument("VoxelAxisPlane::Count is not a geometric plane");
		}
		for (std::size_t index = 0; index < static_cast<std::size_t>(VoxelAxisPlane::Count); ++index)
		{
			const auto localPlane = static_cast<VoxelAxisPlane>(index);
			if (mapLocalPlaneToWorld(localPlane, p_orientation, p_flip) == p_worldPlane)
			{
				return localPlane;
			}
		}
		throw std::logic_error("Unable to map voxel plane");
	}

	Mesh3D VoxelMesher::buildRenderMesh(const VoxelGrid &p_grid, const VoxelRegistry &p_registry)
	{
		return buildRenderMesh(p_grid, p_registry, nullptr, {});
	}

	Mesh3D VoxelMesher::buildRenderMesh(
		const VoxelGrid &p_grid,
		const VoxelRegistry &p_registry,
		const ICellLookup &p_worldLookup,
		const spk::Vector3Int &p_worldOrigin)
	{
		return buildRenderMesh(p_grid, p_registry, &p_worldLookup, p_worldOrigin);
	}

	Mesh3D VoxelMesher::buildRenderMesh(
		const VoxelGrid &p_grid,
		const VoxelRegistry &p_registry,
		const ICellLookup *p_worldLookup,
		const spk::Vector3Int &p_worldOrigin)
	{
		Mesh3D mesh;
		// Typical cube-like cells settle at <=24 vertices / 36 indices before culling.
		// Reserving once avoids repeated buffer and vertex-lookup growth during chunk builds.
		mesh.reserve(p_grid.cells().size() * 24, p_grid.cells().size() * 36);
		FaceTransformCache cache;
		const std::array worldPlanes = {
			VoxelAxisPlane::PositiveX,
			VoxelAxisPlane::NegativeX,
			VoxelAxisPlane::PositiveY,
			VoxelAxisPlane::NegativeY,
			VoxelAxisPlane::PositiveZ,
			VoxelAxisPlane::NegativeZ};

		for (int y = 0; y < p_grid.size().y; ++y)
		{
			for (int z = 0; z < p_grid.size().z; ++z)
			{
				for (int x = 0; x < p_grid.size().x; ++x)
				{
					const spk::Vector3Int position{x, y, z};
					const VoxelCell &cell = p_grid.cell(position);
					if (cell.isEmpty())
					{
						continue;
					}

					const VoxelDefinition &definition = p_registry.get(cell.id);
					const VoxelShapeFaceSet &faces = definition.shape->renderFaces();
					bool hasVisibleShell = false;
					for (VoxelAxisPlane worldPlane : worldPlanes)
					{
						const VoxelAxisPlane localPlane = mapWorldPlaneToLocal(worldPlane, cell.orientation, cell.flip);
						const auto &face = faces.outer(localPlane);
						if (!face.has_value() || isFaceOccludedByNeighbor(
								p_grid, p_registry, p_worldLookup, p_worldOrigin, position, worldPlane, cell, *face))
						{
							continue;
						}
						hasVisibleShell = true;
						emitFace(
							mesh,
							cache.get(*face, cell.orientation, cell.flip),
							spk::Vector3(position));
					}

					if (hasVisibleShell || faces.outerFaceCount() == 0)
					{
						for (const VoxelShapeFace &face : faces.innerFaces)
						{
							emitFace(
								mesh,
								cache.get(face, cell.orientation, cell.flip),
								spk::Vector3(position));
						}
					}
				}
			}
		}
		return mesh;
	}

	Mesh3D VoxelMesher::buildMaskMesh(
		std::span<const spk::Vector3Int> p_cells,
		const MaskAtlasLookup &p_maskOf,
		const ICellLookup &p_lookup)
	{
		if (!p_maskOf)
		{
			throw std::invalid_argument("Mask atlas lookup cannot be empty");
		}

		Mesh3D mesh;
		mesh.reserve(p_cells.size() * 8, p_cells.size() * 12);
		FaceTransformCache cache;
		std::map<std::tuple<int, int, int>, std::size_t> layers;
		for (const spk::Vector3Int &position : p_cells)
		{
			const VoxelCell *cell = p_lookup.tryCell(position);
			if (cell == nullptr || cell->isEmpty())
			{
				continue;
			}
			const VoxelDefinition *definition = p_lookup.tryDefinition(*cell);
			if (definition == nullptr || definition->shape == nullptr)
			{
				throw std::out_of_range("Mask cell references an unknown voxel definition");
			}

			const AtlasCell maskCell = p_maskOf(*cell);
			if (maskCell.column < 0 || maskCell.column >= 4 || maskCell.row < 0 || maskCell.row >= 4)
			{
				throw std::out_of_range("Mask atlas cell must be within the 4x4 atlas");
			}

			const auto layerKey = std::tuple{position.x, position.y, position.z};
			const std::size_t layer = layers[layerKey]++;
			const spk::Vector3 offset{
				static_cast<float>(position.x),
				static_cast<float>(position.y) + 0.001f * static_cast<float>(layer),
				static_cast<float>(position.z)};
			for (const VoxelShapeFace &face : definition->shape->maskFaces().faces(cell->flip))
			{
				emitFace(mesh, cache.get(face, cell->orientation, cell->flip), offset, &maskCell);
			}
		}
		return mesh;
	}
}
