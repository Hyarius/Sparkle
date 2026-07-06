#pragma once

#include "structures/math/spk_vector2.hpp"
#include "structures/math/spk_vector3.hpp"
#include "structures/voxel/spk_voxel_enums.hpp"

#include <array>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace spk
{
	struct AtlasCell
	{
		int column = 0;
		int row = 0;

		[[nodiscard]] bool operator==(const AtlasCell &) const noexcept = default;
	};

	struct VoxelShapeVertex
	{
		spk::Vector3 position{};
		spk::Vector2 uv{};
	};

	using VoxelShapePolygon = std::vector<spk::VoxelShapeVertex>;

	struct VoxelShapeFace
	{
		std::vector<spk::VoxelShapePolygon> polygons;
	};

	// A voxel's render geometry is split in two shells: the outer shell holds at most one
	// face per axis plane and is the only geometry considered by the neighbor occlusion
	// culling, while the inner faces are always emitted as long as the voxel is visible.
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

	private:
		TextureSlots _textures;
		spk::Vector2Int _atlasSize = DefaultAtlasSize;
		spk::VoxelShapeFaceSet _renderFaces;
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
		[[nodiscard]] static spk::VoxelShapeFace createFace(spk::VoxelShapePolygon p_polygon);

		virtual void _constructRenderFaces() = 0;

	public:
		virtual ~VoxelShape() = default;

		VoxelShape(const VoxelShape &) = delete;
		VoxelShape &operator=(const VoxelShape &) = delete;
		VoxelShape(VoxelShape &&) noexcept = default;
		VoxelShape &operator=(VoxelShape &&) noexcept = default;

		void initialize();

		[[nodiscard]] bool initialized() const noexcept;
		[[nodiscard]] const spk::VoxelShapeFaceSet &renderFaces() const noexcept;
		[[nodiscard]] const spk::Vector2Int &atlasSize() const noexcept;

		[[nodiscard]] spk::Vector2 atlasUV(const spk::AtlasCell &p_cell, const spk::Vector2 &p_localUV) const;
	};
}
