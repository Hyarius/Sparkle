#include "structures/voxel/spk_voxel_shape.hpp"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <stdexcept>
#include <utility>

namespace
{
	[[nodiscard]] std::size_t transformVariantIndex(
		spk::VoxelOrientation p_orientation,
		spk::VoxelFlip p_flip)
	{
		const std::size_t orientation = static_cast<std::size_t>(p_orientation);
		const std::size_t flip = static_cast<std::size_t>(p_flip);
		if (orientation >= 4 || flip >= 2)
		{
			throw std::invalid_argument("Invalid voxel face transform");
		}
		return orientation * 2 + flip;
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
		default:
			throw std::invalid_argument("Invalid voxel orientation");
		}

		if (p_flip == spk::VoxelFlip::NegativeY)
		{
			result.y = 1.0f - result.y;
		}
		else if (p_flip != spk::VoxelFlip::PositiveY)
		{
			throw std::invalid_argument("Invalid voxel flip");
		}
		return result;
	}

	[[nodiscard]] spk::VoxelShapeFace transformFace(
		const spk::VoxelShapeFace &p_face,
		spk::VoxelOrientation p_orientation,
		spk::VoxelFlip p_flip)
	{
		spk::VoxelShapeFace result;
		result.reserve(p_face.size());
		for (const spk::VoxelShapePolygon &polygon : p_face.polygons())
		{
			spk::VoxelShapePolygon::Builder builder;
			builder.reserve(polygon.size());
			if (p_flip == spk::VoxelFlip::NegativeY)
			{
				for (std::size_t index = polygon.size(); index > 0; --index)
				{
					const spk::VoxelShapeVertex &vertex = polygon[index - 1];
					builder.addVertex({transformPosition(vertex.position, p_orientation, p_flip), vertex.data});
				}
			}
			else
			{
				for (const spk::VoxelShapeVertex &vertex : polygon)
				{
					builder.addVertex({transformPosition(vertex.position, p_orientation, p_flip), vertex.data});
				}
			}
			result.addPolygon(std::move(builder).bake());
		}
		return result;
	}

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

	[[nodiscard]] bool liesOnCellBoundary(const spk::VoxelShapeFace &p_face, spk::VoxelAxisPlane p_plane)
	{
		const bool positive = p_plane == spk::VoxelAxisPlane::PositiveX ||
							  p_plane == spk::VoxelAxisPlane::PositiveY ||
							  p_plane == spk::VoxelAxisPlane::PositiveZ;
		const float expected = positive ? 1.0f : 0.0f;
		for (const spk::VoxelShapePolygon &polygon : p_face.polygons())
		{
			for (const spk::VoxelShapeVertex &vertex : polygon)
			{
				if (std::abs(planeCoordinate(vertex.position, p_plane) - expected) > spk::VoxelShape::BoundaryEpsilon)
				{
					return false;
				}
			}
		}
		return !p_face.empty();
	}

	[[nodiscard]] std::pair<float, float> boundaryPlaneCoordinates(const spk::Vector3 &p_position, spk::VoxelAxisPlane p_plane)
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

	struct BoundaryPoint
	{
		float first = 0.0f;
		float second = 0.0f;
	};

	using BoundaryPolygon = std::vector<BoundaryPoint>;

	[[nodiscard]] float cross(const BoundaryPoint &p_a, const BoundaryPoint &p_b, const BoundaryPoint &p_c)
	{
		return (p_b.first - p_a.first) * (p_c.second - p_a.second) -
			   (p_b.second - p_a.second) * (p_c.first - p_a.first);
	}

	[[nodiscard]] float signedArea(const BoundaryPolygon &p_polygon)
	{
		float doubledArea = 0.0f;
		for (std::size_t index = 0; index < p_polygon.size(); ++index)
		{
			const BoundaryPoint &a = p_polygon[index];
			const BoundaryPoint &b = p_polygon[(index + 1) % p_polygon.size()];
			doubledArea += a.first * b.second - b.first * a.second;
		}
		return doubledArea * 0.5f;
	}

	[[nodiscard]] bool samePoint(const BoundaryPoint &p_left, const BoundaryPoint &p_right)
	{
		return std::abs(p_left.first - p_right.first) <= spk::VoxelShape::BoundaryEpsilon &&
			   std::abs(p_left.second - p_right.second) <= spk::VoxelShape::BoundaryEpsilon;
	}

	void appendDistinct(BoundaryPolygon &p_polygon, BoundaryPoint p_point)
	{
		if (p_polygon.empty() || !samePoint(p_polygon.back(), p_point))
		{
			p_polygon.push_back(p_point);
		}
	}

	[[nodiscard]] BoundaryPolygon cleanPolygon(BoundaryPolygon p_polygon)
	{
		if (p_polygon.size() > 1 && samePoint(p_polygon.front(), p_polygon.back()))
		{
			p_polygon.pop_back();
		}

		bool changed = true;
		while (changed && p_polygon.size() >= 3)
		{
			changed = false;
			for (std::size_t index = 0; index < p_polygon.size(); ++index)
			{
				const BoundaryPoint &previous = p_polygon[(index + p_polygon.size() - 1) % p_polygon.size()];
				const BoundaryPoint &current = p_polygon[index];
				const BoundaryPoint &next = p_polygon[(index + 1) % p_polygon.size()];
				if (samePoint(previous, current) ||
					std::abs(cross(previous, current, next)) <= spk::VoxelShape::BoundaryEpsilon)
				{
					p_polygon.erase(p_polygon.begin() + static_cast<std::ptrdiff_t>(index));
					changed = true;
					break;
				}
			}
		}
		return p_polygon;
	}

	[[nodiscard]] bool isConvex(const BoundaryPolygon &p_polygon)
	{
		if (p_polygon.size() < 3 || std::abs(signedArea(p_polygon)) <= spk::VoxelShape::BoundaryEpsilon)
		{
			return false;
		}

		float turn = 0.0f;
		for (std::size_t index = 0; index < p_polygon.size(); ++index)
		{
			const float current = cross(
				p_polygon[index],
				p_polygon[(index + 1) % p_polygon.size()],
				p_polygon[(index + 2) % p_polygon.size()]);
			if (std::abs(current) <= spk::VoxelShape::BoundaryEpsilon)
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

	[[nodiscard]] BoundaryPolygon clipHalfPlane(
		const BoundaryPolygon &p_polygon,
		const BoundaryPoint &p_edgeStart,
		const BoundaryPoint &p_edgeEnd,
		float p_orientation,
		bool p_keepInside)
	{
		BoundaryPolygon result;
		if (p_polygon.empty())
		{
			return result;
		}

		const auto distance = [&](const BoundaryPoint &p_point) {
			return p_orientation * cross(p_edgeStart, p_edgeEnd, p_point);
		};
		const auto accepted = [p_keepInside](float p_distance) {
			return p_keepInside ? p_distance >= -spk::VoxelShape::BoundaryEpsilon : p_distance <= spk::VoxelShape::BoundaryEpsilon;
		};

		BoundaryPoint previous = p_polygon.back();
		float previousDistance = distance(previous);
		bool previousAccepted = accepted(previousDistance);
		for (const BoundaryPoint &current : p_polygon)
		{
			const float currentDistance = distance(current);
			const bool currentAccepted = accepted(currentDistance);
			if (currentAccepted != previousAccepted)
			{
				const float denominator = previousDistance - currentDistance;
				if (std::abs(denominator) > spk::VoxelShape::BoundaryEpsilon)
				{
					const float t = previousDistance / denominator;
					appendDistinct(result, {.first = previous.first + (current.first - previous.first) * t, .second = previous.second + (current.second - previous.second) * t});
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
		return cleanPolygon(std::move(result));
	}

	[[nodiscard]] std::vector<BoundaryPolygon> subtractConvex(
		const BoundaryPolygon &p_subject,
		const BoundaryPolygon &p_clip)
	{
		std::vector<BoundaryPolygon> result;
		BoundaryPolygon remaining = p_subject;
		const float orientation = signedArea(p_clip) > 0.0f ? 1.0f : -1.0f;
		for (std::size_t edge = 0; edge < p_clip.size(); ++edge)
		{
			const BoundaryPoint &start = p_clip[edge];
			const BoundaryPoint &end = p_clip[(edge + 1) % p_clip.size()];
			BoundaryPolygon outside = clipHalfPlane(remaining, start, end, orientation, false);
			if (outside.size() >= 3 && std::abs(signedArea(outside)) > spk::VoxelShape::BoundaryEpsilon)
			{
				result.push_back(std::move(outside));
			}

			remaining = clipHalfPlane(remaining, start, end, orientation, true);
			if (remaining.size() < 3 || std::abs(signedArea(remaining)) <= spk::VoxelShape::BoundaryEpsilon)
			{
				break;
			}
		}
		return result;
	}

	// Area of the union of boundary polygons, clipped to the unit cell boundary. Faces
	// not on the boundary cover nothing. Unsupported non-convex polygons conservatively
	// report no coverage so they can never delete neighboring geometry.
	[[nodiscard]] float boundaryCoverage(const spk::VoxelShapeFace &p_face, spk::VoxelAxisPlane p_plane)
	{
		if (!liesOnCellBoundary(p_face, p_plane))
		{
			return 0.0f;
		}

		std::vector<BoundaryPolygon> uncovered = {
			{{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}}};
		for (const spk::VoxelShapePolygon &polygon : p_face.polygons())
		{
			BoundaryPolygon boundary;
			boundary.reserve(polygon.size());
			for (const spk::VoxelShapeVertex &vertex : polygon)
			{
				const auto [first, second] = boundaryPlaneCoordinates(vertex.position, p_plane);
				boundary.push_back({first, second});
			}
			boundary = cleanPolygon(std::move(boundary));
			if (!isConvex(boundary))
			{
				return 0.0f;
			}

			std::vector<BoundaryPolygon> next;
			for (const BoundaryPolygon &piece : uncovered)
			{
				std::vector<BoundaryPolygon> difference = subtractConvex(piece, boundary);
				next.insert(next.end(), std::make_move_iterator(difference.begin()), std::make_move_iterator(difference.end()));
			}
			uncovered = std::move(next);
			if (uncovered.empty())
			{
				return 1.0f;
			}
		}

		float uncoveredArea = 0.0f;
		for (const BoundaryPolygon &piece : uncovered)
		{
			uncoveredArea += std::abs(signedArea(piece));
		}
		return std::clamp(1.0f - uncoveredArea, 0.0f, 1.0f);
	}

	[[nodiscard]] bool coversCellBoundary(const spk::VoxelShapeFace &p_face, spk::VoxelAxisPlane p_plane)
	{
		return liesOnCellBoundary(p_face, p_plane) &&
			   boundaryCoverage(p_face, p_plane) >= 1.0f - spk::VoxelShape::BoundaryEpsilon;
	}
}

namespace spk
{
	std::optional<spk::VoxelShapeFace> &VoxelShapeFaceSet::outer(spk::VoxelAxisPlane p_plane)
	{
		return outerShell.at(static_cast<std::size_t>(p_plane));
	}

	const std::optional<spk::VoxelShapeFace> &VoxelShapeFaceSet::outer(spk::VoxelAxisPlane p_plane) const
	{
		return outerShell.at(static_cast<std::size_t>(p_plane));
	}

	std::size_t VoxelShapeFaceSet::outerFaceCount() const noexcept
	{
		return static_cast<std::size_t>(std::ranges::count_if(outerShell, [](const auto &p_face) {
			return p_face.has_value();
		}));
	}

	VoxelShape::VoxelShape(TextureSlots p_textures, const spk::Vector2Int &p_atlasSize) :
		_textures(std::move(p_textures)),
		_atlasSize(p_atlasSize)
	{
		if (p_atlasSize.x <= 0 || p_atlasSize.y <= 0)
		{
			throw std::invalid_argument("Voxel shape atlas size must be strictly positive");
		}
	}

	spk::VoxelShapeFaceSet &VoxelShape::mutableRenderFaces() noexcept
	{
		return _renderFaces;
	}

	const spk::AtlasCell &VoxelShape::texture(const std::string &p_slot) const
	{
		const auto iterator = _textures.find(p_slot);
		if (iterator == _textures.end())
		{
			throw std::logic_error("Voxel shape texture slot was not provided: " + p_slot);
		}
		return iterator->second;
	}

	spk::VoxelShapePolygon VoxelShape::createPolygon(
		const std::string &p_slot,
		std::span<const spk::Vector3> p_positions,
		std::span<const spk::Vector2> p_localUVs) const
	{
		if (p_positions.size() < 3 || p_positions.size() != p_localUVs.size())
		{
			throw std::invalid_argument("Voxel shape polygon requires matching position/UV lists with at least three vertices");
		}

		spk::VoxelShapePolygon::Builder builder;
		builder.reserve(p_positions.size());
		for (std::size_t index = 0; index < p_positions.size(); ++index)
		{
			builder.addVertex({p_positions[index], atlasUV(texture(p_slot), p_localUVs[index])});
		}
		return std::move(builder).bake();
	}

	spk::VoxelShapePolygon VoxelShape::createRectangle(
		const std::string &p_slot,
		const spk::Vector3 &p_a,
		const spk::Vector3 &p_b,
		const spk::Vector3 &p_c,
		const spk::Vector3 &p_d) const
	{
		const std::array positions = {p_a, p_b, p_c, p_d};
		const std::array<spk::Vector2, 4> uvs = {
			spk::Vector2{0.0f, 0.0f},
			spk::Vector2{1.0f, 0.0f},
			spk::Vector2{1.0f, 1.0f},
			spk::Vector2{0.0f, 1.0f}};
		return createPolygon(p_slot, positions, uvs);
	}

	spk::VoxelShapePolygon VoxelShape::createVerticalRectangle(
		const std::string &p_slot,
		const spk::Vector3 &p_a,
		const spk::Vector3 &p_b,
		const spk::Vector3 &p_c,
		const spk::Vector3 &p_d,
		float p_bottomV,
		float p_topV) const
	{
		const std::array positions = {p_a, p_b, p_c, p_d};
		const std::array<spk::Vector2, 4> uvs = {
			spk::Vector2{0.0f, p_bottomV},
			spk::Vector2{1.0f, p_bottomV},
			spk::Vector2{1.0f, p_topV},
			spk::Vector2{0.0f, p_topV}};
		return createPolygon(p_slot, positions, uvs);
	}

	spk::VoxelShapePolygon VoxelShape::createTriangle(
		const std::string &p_slot,
		const spk::Vector3 &p_a,
		const spk::Vector3 &p_b,
		const spk::Vector3 &p_c) const
	{
		const std::array positions = {p_a, p_b, p_c};
		const std::array<spk::Vector2, 3> uvs = {
			spk::Vector2{0.0f, 0.0f},
			spk::Vector2{1.0f, 0.0f},
			spk::Vector2{1.0f, 1.0f}};
		return createPolygon(p_slot, positions, uvs);
	}

	spk::VoxelShapePolygon VoxelShape::createVerticalTriangle(
		const std::string &p_slot,
		const spk::Vector3 &p_a,
		const spk::Vector3 &p_b,
		const spk::Vector3 &p_c) const
	{
		const std::array positions = {p_a, p_b, p_c};
		const std::array<spk::Vector2, 3> uvs = {
			spk::Vector2{0.0f, 1.0f},
			spk::Vector2{1.0f, 1.0f},
			spk::Vector2{1.0f, 0.0f}};
		return createPolygon(p_slot, positions, uvs);
	}

	void VoxelShape::initialize()
	{
		if (_initialized)
		{
			return;
		}
		_constructRenderFaces();
		_hasOuterFaces = _renderFaces.outerFaceCount() != 0;

		for (std::size_t planeIndex = 0; planeIndex < _renderFaces.outerShell.size(); ++planeIndex)
		{
			auto &face = _renderFaces.outerShell[planeIndex];
			if (!face.has_value())
			{
				continue;
			}

			const auto plane = static_cast<spk::VoxelAxisPlane>(planeIndex);
			_outerFacesOnCellBoundary[planeIndex] = liesOnCellBoundary(*face, plane);
			_outerFacesCoverCellBoundary[planeIndex] = coversCellBoundary(*face, plane);
			_outerFaceBoundaryCoverage[planeIndex] = boundaryCoverage(*face, plane);
			auto &variants = _transformedOuterFaces[planeIndex].emplace();
			for (const auto orientation : {spk::VoxelOrientation::PositiveX, spk::VoxelOrientation::PositiveZ, spk::VoxelOrientation::NegativeX, spk::VoxelOrientation::NegativeZ})
			{
				for (const auto flip : {spk::VoxelFlip::PositiveY, spk::VoxelFlip::NegativeY})
				{
					variants[transformVariantIndex(orientation, flip)] = transformFace(*face, orientation, flip);
				}
			}
		}

		_transformedInnerFaces.resize(_renderFaces.innerFaces.size());
		for (std::size_t faceIndex = 0; faceIndex < _renderFaces.innerFaces.size(); ++faceIndex)
		{
			for (const auto orientation : {spk::VoxelOrientation::PositiveX, spk::VoxelOrientation::PositiveZ, spk::VoxelOrientation::NegativeX, spk::VoxelOrientation::NegativeZ})
			{
				for (const auto flip : {spk::VoxelFlip::PositiveY, spk::VoxelFlip::NegativeY})
				{
					_transformedInnerFaces[faceIndex][transformVariantIndex(orientation, flip)] =
						transformFace(_renderFaces.innerFaces[faceIndex], orientation, flip);
				}
			}
		}
		_initialized = true;
	}

	void VoxelShape::setTransparency(float p_transparency)
	{
		if (p_transparency < 0.0f || p_transparency > 1.0f)
		{
			throw std::invalid_argument("Voxel shape transparency must be between 0 (opaque) and 1 (invisible)");
		}
		_transparency = p_transparency;
	}

	float VoxelShape::transparency() const noexcept
	{
		return _transparency;
	}

	bool VoxelShape::isTransparent() const noexcept
	{
		return _transparency > 0.0f;
	}

	void VoxelShape::setTransparentOcclusionGroup(std::string p_group)
	{
		_transparentOcclusionGroup = std::move(p_group);
	}

	const std::string &VoxelShape::transparentOcclusionGroup() const noexcept
	{
		return _transparentOcclusionGroup;
	}

	bool VoxelShape::initialized() const noexcept
	{
		return _initialized;
	}

	const spk::VoxelShapeFaceSet &VoxelShape::renderFaces() const noexcept
	{
		return _renderFaces;
	}

	const spk::VoxelShapeFace &VoxelShape::transformedOuterFace(
		spk::VoxelAxisPlane p_plane,
		spk::VoxelOrientation p_orientation,
		spk::VoxelFlip p_flip) const
	{
		const auto &variants = _transformedOuterFaces.at(static_cast<std::size_t>(p_plane));
		if (!variants.has_value())
		{
			throw std::logic_error("Requested transform for a missing voxel outer face");
		}
		return variants->at(transformVariantIndex(p_orientation, p_flip));
	}

	const spk::VoxelShapeFace &VoxelShape::transformedInnerFace(
		std::size_t p_faceIndex,
		spk::VoxelOrientation p_orientation,
		spk::VoxelFlip p_flip) const
	{
		return _transformedInnerFaces.at(p_faceIndex).at(transformVariantIndex(p_orientation, p_flip));
	}

	bool VoxelShape::outerFaceLiesOnCellBoundary(spk::VoxelAxisPlane p_plane) const
	{
		return _outerFacesOnCellBoundary.at(static_cast<std::size_t>(p_plane));
	}

	bool VoxelShape::outerFaceCoversCellBoundary(spk::VoxelAxisPlane p_plane) const
	{
		return _outerFacesCoverCellBoundary.at(static_cast<std::size_t>(p_plane));
	}

	float VoxelShape::outerFaceBoundaryCoverage(spk::VoxelAxisPlane p_plane) const
	{
		return _outerFaceBoundaryCoverage.at(static_cast<std::size_t>(p_plane));
	}

	bool VoxelShape::hasOuterFaces() const noexcept
	{
		return _hasOuterFaces;
	}

	const spk::Vector2Int &VoxelShape::atlasSize() const noexcept
	{
		return _atlasSize;
	}

	spk::Vector2 VoxelShape::atlasUV(const spk::AtlasCell &p_cell, const spk::Vector2 &p_localUV) const
	{
		return {
			(static_cast<float>(p_cell.column) + p_localUV.x) / static_cast<float>(_atlasSize.x),
			(static_cast<float>(p_cell.row) + p_localUV.y) / static_cast<float>(_atlasSize.y)};
	}
}
