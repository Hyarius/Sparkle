#include "structures/voxel/spk_data_voxel_shape.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
#include <string>
#include <utility>

namespace
{
	struct BoundaryPlane
	{
		spk::VoxelAxisPlane plane;
		std::size_t axis;
		float boundary;
	};

	constexpr std::array<BoundaryPlane, 6> BoundaryPlanes = {{
		{spk::VoxelAxisPlane::PositiveX, 0, 1.0f},
		{spk::VoxelAxisPlane::NegativeX, 0, 0.0f},
		{spk::VoxelAxisPlane::PositiveY, 1, 1.0f},
		{spk::VoxelAxisPlane::NegativeY, 1, 0.0f},
		{spk::VoxelAxisPlane::PositiveZ, 2, 1.0f},
		{spk::VoxelAxisPlane::NegativeZ, 2, 0.0f},
	}};

	[[nodiscard]] float component(const spk::Vector3 &p_vector, std::size_t p_axis)
	{
		switch (p_axis)
		{
		case 0:
			return p_vector.x;
		case 1:
			return p_vector.y;
		default:
			return p_vector.z;
		}
	}

	// The outward direction of the plane: +boundary axis for the 1 planes, - for the 0 planes.
	[[nodiscard]] float outwardComponent(const BoundaryPlane &p_plane, const spk::Vector3 &p_normal)
	{
		const float axisComponent = component(p_normal, p_plane.axis);
		return p_plane.boundary == 1.0f ? axisComponent : -axisComponent;
	}

	[[nodiscard]] float snapCoordinate(float p_value)
	{
		if (std::abs(p_value) <= spk::VoxelShape::BoundaryEpsilon)
		{
			return 0.0f;
		}
		if (std::abs(p_value - 1.0f) <= spk::VoxelShape::BoundaryEpsilon)
		{
			return 1.0f;
		}
		return p_value;
	}

	void validateAndSnap(spk::VoxelShapeDescription &p_description, const spk::VoxelShape::TextureSlots &p_textures)
	{
		if (p_description.polygons.empty())
		{
			throw std::invalid_argument("DataVoxelShape description must contain at least one polygon");
		}

		for (spk::VoxelShapeDescription::Polygon &polygon : p_description.polygons)
		{
			if (polygon.positions.size() < 3)
			{
				throw std::invalid_argument("DataVoxelShape polygons require at least three vertices");
			}
			if (polygon.positions.size() != polygon.uvs.size())
			{
				throw std::invalid_argument("DataVoxelShape polygons require one UV per vertex");
			}
			if (!p_textures.contains(polygon.slot))
			{
				throw std::invalid_argument("DataVoxelShape polygon references a texture slot that was not provided: " + polygon.slot);
			}

			for (spk::Vector3 &position : polygon.positions)
			{
				position = {snapCoordinate(position.x), snapCoordinate(position.y), snapCoordinate(position.z)};
				if (position.x < 0.0f || position.x > 1.0f ||
					position.y < 0.0f || position.y > 1.0f ||
					position.z < 0.0f || position.z > 1.0f)
				{
					throw std::invalid_argument("DataVoxelShape vertex positions must stay inside the unit voxel cell");
				}
			}
		}
	}
}

namespace spk
{
	DataVoxelShape::DataVoxelShape(
		TextureSlots p_textures,
		spk::VoxelShapeDescription p_description,
		const spk::Vector2Int &p_atlasSize) :
		spk::VoxelShape(std::move(p_textures), p_atlasSize),
		_description(std::move(p_description))
	{
		validateAndSnap(_description, textures());
	}

	void DataVoxelShape::_constructRenderFaces()
	{
		auto &faces = mutableRenderFaces();
		for (const spk::VoxelShapeDescription::Polygon &described : _description.polygons)
		{
			spk::VoxelShapePolygon polygon = createPolygon(described.slot, described.positions, described.uvs);

			// A polygon can lie on at most one boundary plane (lying on two would make it
			// degenerate, which Polygon3D rejects), so the first outward match wins.
			const BoundaryPlane *outerPlane = nullptr;
			for (const BoundaryPlane &candidate : BoundaryPlanes)
			{
				const bool onPlane = std::ranges::all_of(described.positions, [&candidate](const spk::Vector3 &p_position) {
					return component(p_position, candidate.axis) == candidate.boundary;
				});
				if (onPlane && outwardComponent(candidate, polygon.normal()) > 0.0f)
				{
					outerPlane = &candidate;
					break;
				}
			}

			if (outerPlane == nullptr)
			{
				faces.innerFaces.emplace_back(std::move(polygon));
				continue;
			}

			auto &face = faces.outer(outerPlane->plane);
			if (!face.has_value())
			{
				face.emplace();
			}
			face->addPolygon(std::move(polygon));
		}
	}

	const spk::VoxelShapeDescription &DataVoxelShape::description() const noexcept
	{
		return _description;
	}
}
