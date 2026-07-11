#include "structures/voxel/spk_voxel_shape.hpp"
#include "structures/voxel/spk_voxel_orientation.hpp"

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
					builder.addVertex({spk::transformVoxelPosition(vertex.position, p_orientation, p_flip), vertex.data});
				}
			}
			else
			{
				for (const spk::VoxelShapeVertex &vertex : polygon)
				{
					builder.addVertex({spk::transformVoxelPosition(vertex.position, p_orientation, p_flip), vertex.data});
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

	[[nodiscard]] spk::VoxelShapePolygon2D cellBoundaryPolygon2D()
	{
		spk::VoxelShapePolygon2D::Builder builder;
		builder.reserve(4);
		builder.addVertex({{0.0f, 0.0f}, {}});
		builder.addVertex({{1.0f, 0.0f}, {}});
		builder.addVertex({{1.0f, 1.0f}, {}});
		builder.addVertex({{0.0f, 1.0f}, {}});
		return std::move(builder).bake();
	}

	[[nodiscard]] spk::Vector2 interpolateUV(
		const spk::Vector2 &p_from,
		const spk::Vector2 &p_to,
		float p_interpolation) noexcept
	{
		return p_from + (p_to - p_from) * p_interpolation;
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

		const spk::VoxelShapeFace::ProjectedPolygonCache &projectedPolygons =
			p_face.projectedPolygons();
		if (!projectedPolygons.has_value())
		{
			return 0.0f;
		}

		std::vector<spk::VoxelShapePolygon2D> uncovered = {cellBoundaryPolygon2D()};
		for (const spk::VoxelShapePolygonProjection2D &projection : *projectedPolygons)
		{
			if (!projection.polygon.isConvex())
			{
				return 0.0f;
			}

			std::vector<spk::VoxelShapePolygon2D> next;
			for (const spk::VoxelShapePolygon2D &piece : uncovered)
			{
				std::vector<spk::VoxelShapePolygon2D> difference =
					piece.subtractConvex(
						projection.polygon,
						interpolateUV);
				next.insert(
					next.end(),
					std::make_move_iterator(difference.begin()),
					std::make_move_iterator(difference.end()));
			}
			uncovered = std::move(next);
			if (uncovered.empty())
			{
				return 1.0f;
			}
		}

		float uncoveredArea = 0.0f;
		for (const spk::VoxelShapePolygon2D &piece : uncovered)
		{
			uncoveredArea += piece.area();
		}
		return std::clamp(1.0f - uncoveredArea, 0.0f, 1.0f);
	}

}

namespace spk
{
	VoxelShapeFace::ProjectedPolygonCache VoxelShapeFace::_generateProjectedPolygons() const
	{
		ProjectedPolygons result;
		result.reserve(_polygons.size());

		std::optional<spk::VoxelShapePolygon::AxisAlignedPlane> commonPlane;
		for (const spk::VoxelShapePolygon &polygon : _polygons)
		{
			std::optional<spk::VoxelShapePolygonProjection2D> projection =
				polygon.tryProjectTo2D();
			if (!projection.has_value())
			{
				return std::nullopt;
			}

			if (commonPlane.has_value() && projection->plane != *commonPlane)
			{
				return std::nullopt;
			}
			commonPlane = projection->plane;
			result.push_back(std::move(*projection));
		}

		return result;
	}

	void VoxelShapeFace::_invalidateProjectionCache() const
	{
		_projectedPolygons.release();
	}

	VoxelShapeFace::VoxelShapeFace(spk::VoxelShapePolygon p_polygon)
	{
		addPolygon(std::move(p_polygon));
	}

	VoxelShapeFace::VoxelShapeFace(const VoxelShapeFace &p_other) :
		_polygons(p_other._polygons)
	{
		if (p_other._projectedPolygons.isCached())
		{
			_projectedPolygons.set(p_other._projectedPolygons.get());
		}
	}

	VoxelShapeFace &VoxelShapeFace::operator=(const VoxelShapeFace &p_other)
	{
		if (this != &p_other)
		{
			_invalidateProjectionCache();
			_polygons = p_other._polygons;
			if (p_other._projectedPolygons.isCached())
			{
				_projectedPolygons.set(p_other._projectedPolygons.get());
			}
		}
		return *this;
	}

	VoxelShapeFace::VoxelShapeFace(VoxelShapeFace &&p_other) noexcept :
		_polygons(std::move(p_other._polygons))
	{
		if (std::optional<ProjectedPolygonCache> cached = p_other._projectedPolygons.take();
			cached.has_value())
		{
			_projectedPolygons.set(std::move(*cached));
		}
	}

	VoxelShapeFace &VoxelShapeFace::operator=(VoxelShapeFace &&p_other) noexcept
	{
		if (this != &p_other)
		{
			_invalidateProjectionCache();
			_polygons = std::move(p_other._polygons);
			if (std::optional<ProjectedPolygonCache> cached = p_other._projectedPolygons.take();
				cached.has_value())
			{
				_projectedPolygons.set(std::move(*cached));
			}
		}
		return *this;
	}

	void VoxelShapeFace::reserve(std::size_t p_capacity)
	{
		_polygons.reserve(p_capacity);
	}

	void VoxelShapeFace::addPolygon(spk::VoxelShapePolygon p_polygon)
	{
		if (!_polygons.empty() && _polygons.front().normal() != p_polygon.normal())
		{
			throw std::logic_error("VoxelShapeFace polygons must share the same normal");
		}

		_polygons.push_back(std::move(p_polygon));
		_invalidateProjectionCache();
	}

	std::span<const spk::VoxelShapePolygon> VoxelShapeFace::polygons() const noexcept
	{
		return _polygons;
	}

	const VoxelShapeFace::ProjectedPolygonCache &VoxelShapeFace::projectedPolygons() const
	{
		if (!_projectedPolygons.isCached())
		{
			_projectedPolygons.set(_generateProjectedPolygons());
		}
		return _projectedPolygons.get();
	}

	bool VoxelShapeFace::empty() const noexcept
	{
		return _polygons.empty();
	}

	std::size_t VoxelShapeFace::size() const noexcept
	{
		return _polygons.size();
	}

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

	const VoxelShape::TextureSlots &VoxelShape::textures() const noexcept
	{
		return _textures;
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
			const bool liesOnBoundary = liesOnCellBoundary(*face, plane);
			const float coverage = liesOnBoundary ? boundaryCoverage(*face, plane) : 0.0f;
			_outerFacesOnCellBoundary[planeIndex] = liesOnBoundary;
			_outerFacesCoverCellBoundary[planeIndex] =
				liesOnBoundary && coverage >= 1.0f - spk::VoxelShape::BoundaryEpsilon;
			_outerFaceBoundaryCoverage[planeIndex] = coverage;

			auto &variants = _transformedOuterFaces[planeIndex].emplace();
			for (const auto orientation : {spk::VoxelOrientation::PositiveX, spk::VoxelOrientation::PositiveZ, spk::VoxelOrientation::NegativeX, spk::VoxelOrientation::NegativeZ})
			{
				for (const auto flip : {spk::VoxelFlip::PositiveY, spk::VoxelFlip::NegativeY})
				{
					spk::VoxelShapeFace &variant = variants[transformVariantIndex(orientation, flip)];
					variant = transformFace(*face, orientation, flip);
					(void)variant.projectedPolygons();
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
