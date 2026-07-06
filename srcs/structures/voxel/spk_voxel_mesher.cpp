#include "structures/voxel/spk_voxel_mesher.hpp"

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
	using FaceCacheKey = std::tuple<const spk::VoxelShapeFace *, spk::VoxelOrientation, spk::VoxelFlip>;

	[[nodiscard]] spk::VoxelAxisPlane opposite(spk::VoxelAxisPlane p_plane)
	{
		using P = spk::VoxelAxisPlane;
		switch (p_plane)
		{
		case P::PositiveX:
			return P::NegativeX;
		case P::NegativeX:
			return P::PositiveX;
		case P::PositiveY:
			return P::NegativeY;
		case P::NegativeY:
			return P::PositiveY;
		case P::PositiveZ:
			return P::NegativeZ;
		case P::NegativeZ:
			return P::PositiveZ;
		case P::Count:
			break;
		}
		throw std::invalid_argument("VoxelAxisPlane::Count is not a geometric plane");
	}

	[[nodiscard]] spk::Vector3Int planeOffset(spk::VoxelAxisPlane p_plane)
	{
		using P = spk::VoxelAxisPlane;
		switch (p_plane)
		{
		case P::PositiveX:
			return {1, 0, 0};
		case P::NegativeX:
			return {-1, 0, 0};
		case P::PositiveY:
			return {0, 1, 0};
		case P::NegativeY:
			return {0, -1, 0};
		case P::PositiveZ:
			return {0, 0, 1};
		case P::NegativeZ:
			return {0, 0, -1};
		case P::Count:
			break;
		}
		throw std::invalid_argument("VoxelAxisPlane::Count is not a geometric plane");
	}

	[[nodiscard]] spk::VoxelAxisPlane mapLocalPlaneToWorld(
		spk::VoxelAxisPlane p_localPlane,
		spk::VoxelOrientation p_orientation,
		spk::VoxelFlip p_flip)
	{
		using O = spk::VoxelOrientation;
		using P = spk::VoxelAxisPlane;

		if (p_localPlane == P::PositiveY || p_localPlane == P::NegativeY)
		{
			if (p_flip == spk::VoxelFlip::PositiveY)
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
			case P::PositiveX:
				return P::NegativeZ;
			case P::NegativeX:
				return P::PositiveZ;
			case P::PositiveZ:
				return P::PositiveX;
			case P::NegativeZ:
				return P::NegativeX;
			default:
				break;
			}
			break;
		case O::NegativeZ:
			switch (p_localPlane)
			{
			case P::PositiveX:
				return P::NegativeX;
			case P::NegativeX:
				return P::PositiveX;
			case P::PositiveZ:
				return P::NegativeZ;
			case P::NegativeZ:
				return P::PositiveZ;
			default:
				break;
			}
			break;
		case O::NegativeX:
			switch (p_localPlane)
			{
			case P::PositiveX:
				return P::PositiveZ;
			case P::NegativeX:
				return P::NegativeZ;
			case P::PositiveZ:
				return P::NegativeX;
			case P::NegativeZ:
				return P::PositiveX;
			default:
				break;
			}
			break;
		}
		throw std::invalid_argument("Invalid local voxel plane");
	}

	[[nodiscard]] spk::Vector3 transformPosition(
		const spk::Vector3 &p_position,
		spk::VoxelOrientation p_orientation,
		spk::VoxelFlip p_flip)
	{
		spk::Vector3 result;
		switch (p_orientation)
		{
		case spk::VoxelOrientation::PositiveZ:
			result = p_position;
			break;
		case spk::VoxelOrientation::PositiveX:
			result = {p_position.z, p_position.y, 1.0f - p_position.x};
			break;
		case spk::VoxelOrientation::NegativeZ:
			result = {1.0f - p_position.x, p_position.y, 1.0f - p_position.z};
			break;
		case spk::VoxelOrientation::NegativeX:
			result = {1.0f - p_position.z, p_position.y, p_position.x};
			break;
		}

		if (p_flip == spk::VoxelFlip::NegativeY)
		{
			result.y = 1.0f - result.y;
		}
		return result;
	}

	[[nodiscard]] spk::VoxelShapeFace transformFace(
		const spk::VoxelShapeFace &p_face,
		spk::VoxelOrientation p_orientation,
		spk::VoxelFlip p_flip)
	{
		spk::VoxelShapeFace result = p_face;
		for (spk::VoxelShapePolygon &polygon : result.polygons)
		{
			for (spk::VoxelShapeVertex &vertex : polygon)
			{
				vertex.position = transformPosition(vertex.position, p_orientation, p_flip);
			}
			if (p_flip == spk::VoxelFlip::NegativeY)
			{
				std::ranges::reverse(polygon);
			}
		}
		return result;
	}

	class FaceTransformCache
	{
	private:
		std::map<FaceCacheKey, spk::VoxelShapeFace> _faces;

	public:
		[[nodiscard]] const spk::VoxelShapeFace &get(
			const spk::VoxelShapeFace &p_face,
			spk::VoxelOrientation p_orientation,
			spk::VoxelFlip p_flip)
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

	[[nodiscard]] float planeCoordinate(const spk::Vector3 &p_position, spk::VoxelAxisPlane p_plane)
	{
		switch (p_plane)
		{
		case spk::VoxelAxisPlane::PositiveX:
		case spk::VoxelAxisPlane::NegativeX:
			return p_position.x;
		case spk::VoxelAxisPlane::PositiveY:
		case spk::VoxelAxisPlane::NegativeY:
			return p_position.y;
		case spk::VoxelAxisPlane::PositiveZ:
		case spk::VoxelAxisPlane::NegativeZ:
			return p_position.z;
		case spk::VoxelAxisPlane::Count:
			break;
		}
		throw std::invalid_argument("VoxelAxisPlane::Count is not a geometric plane");
	}

	[[nodiscard]] bool isPositivePlane(spk::VoxelAxisPlane p_plane)
	{
		return p_plane == spk::VoxelAxisPlane::PositiveX ||
			   p_plane == spk::VoxelAxisPlane::PositiveY ||
			   p_plane == spk::VoxelAxisPlane::PositiveZ;
	}

	[[nodiscard]] bool liesOnCellBoundary(const spk::VoxelShapeFace &p_face, spk::VoxelAxisPlane p_plane)
	{
		const float expected = isPositivePlane(p_plane) ? 1.0f : 0.0f;
		for (const spk::VoxelShapePolygon &polygon : p_face.polygons)
		{
			for (const spk::VoxelShapeVertex &vertex : polygon)
			{
				if (std::abs(planeCoordinate(vertex.position, p_plane) - expected) > 0.0001f)
				{
					return false;
				}
			}
		}
		return !p_face.polygons.empty();
	}

	[[nodiscard]] bool isFullBoundaryQuad(const spk::VoxelShapeFace &p_face, spk::VoxelAxisPlane p_plane)
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
		for (const spk::VoxelShapeVertex &vertex : p_face.polygons.front())
		{
			float first = 0.0f;
			float second = 0.0f;
			switch (p_plane)
			{
			case spk::VoxelAxisPlane::PositiveX:
			case spk::VoxelAxisPlane::NegativeX:
				first = vertex.position.y;
				second = vertex.position.z;
				break;
			case spk::VoxelAxisPlane::PositiveY:
			case spk::VoxelAxisPlane::NegativeY:
				first = vertex.position.x;
				second = vertex.position.z;
				break;
			case spk::VoxelAxisPlane::PositiveZ:
			case spk::VoxelAxisPlane::NegativeZ:
				first = vertex.position.x;
				second = vertex.position.y;
				break;
			case spk::VoxelAxisPlane::Count:
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
		const spk::VoxelGrid &p_grid,
		const spk::VoxelRegistry &p_registry,
		const spk::IVoxelCellLookup *p_worldLookup,
		const spk::Vector3Int &p_worldOrigin,
		const spk::Vector3Int &p_position,
		spk::VoxelAxisPlane p_worldPlane,
		const spk::VoxelCell &p_cell,
		const spk::VoxelShapeFace &p_localFace)
	{
		const spk::VoxelAxisPlane localPlane = spk::VoxelMesher::mapWorldPlaneToLocal(
			p_worldPlane, p_cell.orientation, p_cell.flip);
		if (!liesOnCellBoundary(p_localFace, localPlane))
		{
			return false;
		}

		const spk::Vector3Int neighborPosition = p_position + planeOffset(p_worldPlane);
		const spk::VoxelCell *neighbor = nullptr;
		if (p_grid.isWithinBounds(neighborPosition))
		{
			neighbor = &p_grid.cell(neighborPosition);
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
		const spk::VoxelAxisPlane neighborLocalPlane = spk::VoxelMesher::mapWorldPlaneToLocal(
			opposite(p_worldPlane), neighbor->orientation, neighbor->flip);
		const auto &neighborFace = neighborShape.renderFaces().outer(neighborLocalPlane);
		return neighborFace.has_value() && isFullBoundaryQuad(*neighborFace, neighborLocalPlane);
	}

	void emitFace(
		spk::TextureMesh3D::Builder &p_builder,
		const spk::VoxelShapeFace &p_face,
		const spk::Vector3 &p_offset)
	{
		for (const spk::VoxelShapePolygon &polygon : p_face.polygons)
		{
			if (polygon.size() < 3)
			{
				continue;
			}

			const spk::Vector3 normal =
				(polygon[1].position - polygon[0].position)
					.cross(polygon[2].position - polygon[0].position)
					.normalized();

			std::vector<spk::TextureVertex3D> vertices;
			vertices.reserve(polygon.size());
			for (const spk::VoxelShapeVertex &vertex : polygon)
			{
				vertices.push_back({vertex.position + p_offset, normal, vertex.uv});
			}
			p_builder.addShape(vertices);
		}
	}
}

namespace spk
{
	spk::VoxelAxisPlane VoxelMesher::mapWorldPlaneToLocal(
		spk::VoxelAxisPlane p_worldPlane,
		spk::VoxelOrientation p_orientation,
		spk::VoxelFlip p_flip)
	{
		if (p_worldPlane == spk::VoxelAxisPlane::Count)
		{
			throw std::invalid_argument("VoxelAxisPlane::Count is not a geometric plane");
		}
		for (std::size_t index = 0; index < static_cast<std::size_t>(spk::VoxelAxisPlane::Count); ++index)
		{
			const auto localPlane = static_cast<spk::VoxelAxisPlane>(index);
			if (mapLocalPlaneToWorld(localPlane, p_orientation, p_flip) == p_worldPlane)
			{
				return localPlane;
			}
		}
		throw std::logic_error("Unable to map voxel plane");
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
		spk::TextureMesh3D::Builder builder;
		const std::size_t nonEmptyCellCount = static_cast<std::size_t>(std::ranges::count_if(
			p_grid.cells(),
			[](const spk::VoxelCell &p_cell) {
				return p_cell.isEmpty() == false;
			}));
		// Reserve for typical visible surfaces without multiplying the worst-case
		// allocation by every parallel worker. The buffers still grow normally for
		// sparse checkerboards and unusually complex shapes.
		builder.reserve(nonEmptyCellCount * 8, nonEmptyCellCount * 12);
		FaceTransformCache cache;
		const std::array worldPlanes = {
			spk::VoxelAxisPlane::PositiveX,
			spk::VoxelAxisPlane::NegativeX,
			spk::VoxelAxisPlane::PositiveY,
			spk::VoxelAxisPlane::NegativeY,
			spk::VoxelAxisPlane::PositiveZ,
			spk::VoxelAxisPlane::NegativeZ};

		for (int y = 0; y < p_grid.size().y; ++y)
		{
			for (int z = 0; z < p_grid.size().z; ++z)
			{
				for (int x = 0; x < p_grid.size().x; ++x)
				{
					const spk::Vector3Int position{x, y, z};
					const spk::VoxelCell &cell = p_grid.cell(position);
					if (cell.isEmpty())
					{
						continue;
					}

					const spk::VoxelShape &shape = p_registry.shape(cell.id);
					const spk::VoxelShapeFaceSet &faces = shape.renderFaces();
					bool hasVisibleShell = false;
					for (spk::VoxelAxisPlane worldPlane : worldPlanes)
					{
						const spk::VoxelAxisPlane localPlane = mapWorldPlaneToLocal(worldPlane, cell.orientation, cell.flip);
						const auto &face = faces.outer(localPlane);
						if (!face.has_value() || isFaceOccludedByNeighbor(
													 p_grid, p_registry, p_worldLookup, p_worldOrigin, position, worldPlane, cell, *face))
						{
							continue;
						}
						hasVisibleShell = true;
						emitFace(
							builder,
							cache.get(*face, cell.orientation, cell.flip),
							spk::Vector3(position));
					}

					if (hasVisibleShell || faces.outerFaceCount() == 0)
					{
						for (const spk::VoxelShapeFace &face : faces.innerFaces)
						{
							emitFace(
								builder,
								cache.get(face, cell.orientation, cell.flip),
								spk::Vector3(position));
						}
					}
				}
			}
		}
		return std::move(builder).bake();
	}
}
