#include "structures/voxel/spk_voxel_mesher_occlusion.hpp"

#include <iterator>
#include <stdexcept>

namespace
{
	[[nodiscard]] spk::Vector2 interpolateUV(
		const spk::Vector2 &p_from,
		const spk::Vector2 &p_to,
		float p_interpolation) noexcept
	{
		return p_from + (p_to - p_from) * p_interpolation;
	}

	using VoxelShapeAxisAlignedPlane = spk::VoxelShapePolygon::AxisAlignedPlane;
	using VoxelShapePolygon2D = spk::VoxelShapePolygon2D;

	[[nodiscard]] spk::Vector3 restorePosition(
		const spk::Vector2 &p_position,
		VoxelShapeAxisAlignedPlane p_plane,
		float p_coordinate)
	{
		switch (p_plane)
		{
		case VoxelShapeAxisAlignedPlane::XY:
			return {p_position.x, p_position.y, p_coordinate};

		case VoxelShapeAxisAlignedPlane::XZ:
			return {p_position.x, p_coordinate, p_position.y};

		case VoxelShapeAxisAlignedPlane::YZ:
			return {p_coordinate, p_position.x, p_position.y};
		}

		throw std::logic_error("Unsupported Polygon3D projection plane");
	}

	[[nodiscard]] spk::VoxelShapePolygon restorePolygon3D(
		const VoxelShapePolygon2D &p_polygon,
		VoxelShapeAxisAlignedPlane p_plane,
		float p_coordinate)
	{
		spk::VoxelShapePolygon::Builder builder;
		builder.reserve(p_polygon.size());

		for (const auto &vertex : p_polygon)
		{
			builder.addVertex({.position = restorePosition(vertex.position, p_plane, p_coordinate), .data = vertex.data});
		}

		return builder.bake();
	}

	[[nodiscard]] std::vector<spk::VoxelShapePolygon> visibleDifference(
		const spk::VoxelShapeFace &p_face,
		std::size_t p_polygonIndex,
		const spk::VoxelShapeFace &p_occluder)
	{
		const std::span<const spk::VoxelShapePolygon> polygons = p_face.polygons();
		const spk::VoxelShapePolygon &polygon = polygons[p_polygonIndex];

		const spk::VoxelShapeFace::ProjectedPolygonCache &polygonProjections =
			p_face.projectedPolygons();
		const spk::VoxelShapeFace::ProjectedPolygonCache &occluderProjections =
			p_occluder.projectedPolygons();

		if (!polygonProjections.has_value() ||
			p_polygonIndex >= polygonProjections->size() ||
			!occluderProjections.has_value())
		{
			return {polygon};
		}

		const spk::VoxelShapePolygonProjection2D &polygonProjection =
			polygonProjections->at(p_polygonIndex);
		if (!polygonProjection.polygon.isConvex())
		{
			return {polygon};
		}

		for (const spk::VoxelShapePolygonProjection2D &occluderProjection : *occluderProjections)
		{
			if (occluderProjection.plane != polygonProjection.plane ||
				!occluderProjection.polygon.isConvex())
			{
				return {polygon};
			}
		}

		std::vector<spk::VoxelShapePolygon2D> visible = {polygonProjection.polygon};
		for (const spk::VoxelShapePolygonProjection2D &occluderProjection : *occluderProjections)
		{
			std::vector<spk::VoxelShapePolygon2D> next;
			for (const spk::VoxelShapePolygon2D &piece : visible)
			{
				std::vector<spk::VoxelShapePolygon2D> difference =
					piece.subtractConvex(
						occluderProjection.polygon,
						interpolateUV);
				next.insert(
					next.end(),
					std::make_move_iterator(difference.begin()),
					std::make_move_iterator(difference.end()));
			}

			visible = std::move(next);
			if (visible.empty())
			{
				break;
			}
		}

		std::vector<spk::VoxelShapePolygon> result;
		result.reserve(visible.size());
		for (const spk::VoxelShapePolygon2D &visiblePolygon : visible)
		{
			result.push_back(restorePolygon3D(
				visiblePolygon,
				polygonProjection.plane,
				polygonProjection.coordinate));
		}
		return result;
	}
}

namespace spk
{
	std::vector<spk::VoxelShapePolygon> visibleFaceRemnants(
		const spk::VoxelShapeFace &p_face,
		const spk::VoxelShapeFace &p_occluder)
	{
		std::vector<spk::VoxelShapePolygon> result;
		for (std::size_t polygonIndex = 0; polygonIndex < p_face.size(); ++polygonIndex)
		{
			std::vector<spk::VoxelShapePolygon> visible = visibleDifference(p_face, polygonIndex, p_occluder);
			result.insert(
				result.end(),
				std::make_move_iterator(visible.begin()),
				std::make_move_iterator(visible.end()));
		}
		return result;
	}

	std::size_t VoxelFaceRemnantCache::FacePairHash::operator()(const FacePair &p_pair) const noexcept
	{
		const std::size_t first = std::hash<const void *>{}(p_pair.first);
		const std::size_t second = std::hash<const void *>{}(p_pair.second);
		return first ^ (second + 0x9e3779b97f4a7c15ULL + (first << 6) + (first >> 2));
	}

	const std::vector<spk::VoxelShapePolygon> &VoxelFaceRemnantCache::remnants(
		const spk::VoxelShapeFace &p_face,
		const spk::VoxelShapeFace &p_occluder)
	{
		const FacePair key{&p_face, &p_occluder};
		auto iterator = _remnants.find(key);
		if (iterator == _remnants.end())
		{
			iterator = _remnants.emplace(key, visibleFaceRemnants(p_face, p_occluder)).first;
		}
		return iterator->second;
	}
}
