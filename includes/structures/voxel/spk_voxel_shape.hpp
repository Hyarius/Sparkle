#pragma once

#include "structures/graphics/geometry/spk_polygon_3d.hpp"
#include "structures/math/spk_vector2.hpp"
#include "structures/math/spk_vector3.hpp"
#include "structures/voxel/spk_voxel_enums.hpp"

#include <array>
#include <map>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace spk
{
	struct AtlasCell
	{
		int column = 0;
		int row = 0;

		[[nodiscard]] bool operator==(const AtlasCell &) const noexcept = default;
	};

	using VoxelShapePolygonData = spk::Vector2;
	using VoxelShapePolygon = spk::Polygon3D<VoxelShapePolygonData>;
	using VoxelShapeVertex = spk::VoxelShapePolygon::Vertex;

	class VoxelShapeFace
	{
	private:
		std::vector<spk::VoxelShapePolygon> _polygons;

	public:
		VoxelShapeFace() = default;
		VoxelShapeFace(spk::VoxelShapePolygon p_polygon)
		{
			addPolygon(std::move(p_polygon));
		}

		void reserve(std::size_t p_capacity)
		{
			_polygons.reserve(p_capacity);
		}

		void addPolygon(spk::VoxelShapePolygon p_polygon)
		{
			if (!_polygons.empty() && _polygons.front().normal() != p_polygon.normal())
			{
				throw std::logic_error("VoxelShapeFace polygons must share the same normal");
			}
			_polygons.push_back(std::move(p_polygon));
		}

		[[nodiscard]] std::span<const spk::VoxelShapePolygon> polygons() const noexcept
		{
			return _polygons;
		}

		[[nodiscard]] bool empty() const noexcept
		{
			return _polygons.empty();
		}

		[[nodiscard]] std::size_t size() const noexcept
		{
			return _polygons.size();
		}
	};

	// A voxel's render geometry is split in two shells: the outer shell holds at most one
	// face per axis plane and participates in exact boundary-plane occlusion. Inner-only
	// shapes are always emitted; for mixed shapes, inner faces are suppressed only when
	// complete compatible neighbor faces seal all six cell boundary planes.
	struct VoxelShapeFaceSet
	{
		std::vector<spk::VoxelShapeFace> innerFaces;
		std::array<std::optional<spk::VoxelShapeFace>, static_cast<std::size_t>(spk::VoxelAxisPlane::Count)> outerShell;

		[[nodiscard]] std::optional<spk::VoxelShapeFace> &outer(spk::VoxelAxisPlane p_plane);
		[[nodiscard]] const std::optional<spk::VoxelShapeFace> &outer(spk::VoxelAxisPlane p_plane) const;
		[[nodiscard]] std::size_t outerFaceCount() const noexcept;
	};

	class VoxelShape
	{
	public:
		using TextureSlots = std::map<std::string, spk::AtlasCell>;

		static constexpr spk::Vector2Int DefaultAtlasSize{8, 8};
		// Boundary membership, polygon clipping, and coverage comparisons use this
		// tolerance in normalized cell coordinates (and their unit-square area).
		static constexpr float BoundaryEpsilon = 0.0001f;

	private:
		static constexpr std::size_t TransformVariantCount = 8;
		using FaceTransformVariants = std::array<spk::VoxelShapeFace, TransformVariantCount>;

		TextureSlots _textures;
		spk::Vector2Int _atlasSize = DefaultAtlasSize;
		float _transparency = 0.0f;
		std::string _transparentOcclusionGroup;
		spk::VoxelShapeFaceSet _renderFaces;
		std::array<std::optional<FaceTransformVariants>, static_cast<std::size_t>(spk::VoxelAxisPlane::Count)> _transformedOuterFaces;
		std::vector<FaceTransformVariants> _transformedInnerFaces;
		std::array<bool, static_cast<std::size_t>(spk::VoxelAxisPlane::Count)> _outerFacesOnCellBoundary{};
		std::array<bool, static_cast<std::size_t>(spk::VoxelAxisPlane::Count)> _outerFacesCoverCellBoundary{};
		std::array<float, static_cast<std::size_t>(spk::VoxelAxisPlane::Count)> _outerFaceBoundaryCoverage{};
		bool _hasOuterFaces = false;
		bool _initialized = false;

	protected:
		explicit VoxelShape(TextureSlots p_textures, const spk::Vector2Int &p_atlasSize = DefaultAtlasSize);

		[[nodiscard]] spk::VoxelShapeFaceSet &mutableRenderFaces() noexcept;
		[[nodiscard]] const spk::AtlasCell &texture(const std::string &p_slot) const;

		[[nodiscard]] spk::VoxelShapePolygon createPolygon(
			const std::string &p_slot,
			std::span<const spk::Vector3> p_positions,
			std::span<const spk::Vector2> p_localUVs) const;
		[[nodiscard]] spk::VoxelShapePolygon createRectangle(
			const std::string &p_slot,
			const spk::Vector3 &p_a,
			const spk::Vector3 &p_b,
			const spk::Vector3 &p_c,
			const spk::Vector3 &p_d) const;
		// Vertical faces pass their bottom edge first and their top edge last. Their V
		// coordinate therefore runs in the opposite direction from horizontal faces.
		[[nodiscard]] spk::VoxelShapePolygon createVerticalRectangle(
			const std::string &p_slot,
			const spk::Vector3 &p_a,
			const spk::Vector3 &p_b,
			const spk::Vector3 &p_c,
			const spk::Vector3 &p_d,
			float p_bottomV = 1.0f,
			float p_topV = 0.0f) const;
		[[nodiscard]] spk::VoxelShapePolygon createTriangle(
			const std::string &p_slot,
			const spk::Vector3 &p_a,
			const spk::Vector3 &p_b,
			const spk::Vector3 &p_c) const;
		[[nodiscard]] spk::VoxelShapePolygon createVerticalTriangle(
			const std::string &p_slot,
			const spk::Vector3 &p_a,
			const spk::Vector3 &p_b,
			const spk::Vector3 &p_c) const;
		virtual void _constructRenderFaces() = 0;

	public:
		virtual ~VoxelShape() = default;

		VoxelShape(const VoxelShape &) = delete;
		VoxelShape &operator=(const VoxelShape &) = delete;
		VoxelShape(VoxelShape &&) noexcept = default;
		VoxelShape &operator=(VoxelShape &&) noexcept = default;

		void initialize();

		// How see-through the voxel renders: 0 is fully opaque, 1 fully invisible. Any value
		// above 0 routes the voxel's faces to the transparent render mesh. Transparency is
		// deliberately not used to decide whether two materials share an internal interface.
		void setTransparency(float p_transparency);
		[[nodiscard]] float transparency() const noexcept;
		[[nodiscard]] bool isTransparent() const noexcept;

		// Transparent faces cull one another only when both shapes have the same non-empty
		// group, or when both cells use the exact same VoxelShape instance. This explicit
		// medium identity keeps unrelated equal-alpha materials from erasing their interface;
		// alpha values within one named group may differ by any representable amount.
		void setTransparentOcclusionGroup(std::string p_group);
		[[nodiscard]] const std::string &transparentOcclusionGroup() const noexcept;

		[[nodiscard]] bool initialized() const noexcept;
		[[nodiscard]] const spk::VoxelShapeFaceSet &renderFaces() const noexcept;
		[[nodiscard]] const spk::VoxelShapeFace &transformedOuterFace(
			spk::VoxelAxisPlane p_plane,
			spk::VoxelOrientation p_orientation,
			spk::VoxelFlip p_flip) const;
		[[nodiscard]] const spk::VoxelShapeFace &transformedInnerFace(
			std::size_t p_faceIndex,
			spk::VoxelOrientation p_orientation,
			spk::VoxelFlip p_flip) const;
		[[nodiscard]] bool outerFaceLiesOnCellBoundary(spk::VoxelAxisPlane p_plane) const;
		[[nodiscard]] bool outerFaceCoversCellBoundary(spk::VoxelAxisPlane p_plane) const;
		// Fraction of the unit cell boundary the outer face covers: 0 when there is no face
		// on that plane or it does not lie on the boundary, 1 when it covers it completely.
		[[nodiscard]] float outerFaceBoundaryCoverage(spk::VoxelAxisPlane p_plane) const;
		[[nodiscard]] bool hasOuterFaces() const noexcept;
		[[nodiscard]] const spk::Vector2Int &atlasSize() const noexcept;

		[[nodiscard]] spk::Vector2 atlasUV(const spk::AtlasCell &p_cell, const spk::Vector2 &p_localUV) const;
	};
}
