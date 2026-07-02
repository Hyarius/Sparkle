#pragma once

#include "voxel/voxel_enums.hpp"

#include "structures/math/spk_vector2.hpp"
#include "structures/math/spk_vector3.hpp"

#include <array>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace pg
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

	using VoxelShapePolygon = std::vector<VoxelShapeVertex>;

	struct VoxelShapeFace
	{
		std::vector<VoxelShapePolygon> polygons;
	};

	struct VoxelShapeFaceSet
	{
		std::vector<VoxelShapeFace> innerFaces;
		std::array<std::optional<VoxelShapeFace>, static_cast<std::size_t>(VoxelAxisPlane::Count)> outerShell;

		[[nodiscard]] std::optional<VoxelShapeFace> &outer(VoxelAxisPlane p_plane);
		[[nodiscard]] const std::optional<VoxelShapeFace> &outer(VoxelAxisPlane p_plane) const;
		[[nodiscard]] std::size_t outerFaceCount() const noexcept;
	};

	struct VoxelShapeMaskSet
	{
		std::vector<VoxelShapeFace> positiveY;
		std::vector<VoxelShapeFace> negativeY;

		[[nodiscard]] const std::vector<VoxelShapeFace> &faces(VoxelFlip p_flip) const noexcept;
	};

	struct CardinalHeightSet
	{
		float positiveX = 0.0f;
		float negativeX = 0.0f;
		float positiveZ = 0.0f;
		float negativeZ = 0.0f;
		float stationary = 0.0f;

		[[nodiscard]] float get(VoxelOrientation p_direction) const;
	};

	struct CardinalHeightCollection
	{
		CardinalHeightSet positiveY;
		CardinalHeightSet negativeY;

		[[nodiscard]] const CardinalHeightSet &get(VoxelFlip p_flip) const noexcept;
	};

	class VoxelShape
	{
	public:
		using TextureSlots = std::map<std::string, AtlasCell>;
		static constexpr int AtlasColumns = 8;
		static constexpr int AtlasRows = 8;

	private:
		TextureSlots _textures;
		VoxelShapeFaceSet _renderFaces;
		VoxelShapeMaskSet _maskFaces;
		CardinalHeightCollection _cardinalHeights;
		bool _initialized = false;

	protected:
		explicit VoxelShape(TextureSlots p_textures);

		[[nodiscard]] VoxelShapeFaceSet &mutableRenderFaces() noexcept;
		[[nodiscard]] VoxelShapeMaskSet &mutableMaskFaces() noexcept;
		[[nodiscard]] const AtlasCell &texture(const std::string &p_slot) const;

		[[nodiscard]] VoxelShapePolygon createPolygon(
			const std::string &p_slot,
			std::span<const spk::Vector3> p_positions,
			std::span<const spk::Vector2> p_localUVs) const;
		[[nodiscard]] VoxelShapePolygon createRectangle(
			const std::string &p_slot,
			const spk::Vector3 &p_a,
			const spk::Vector3 &p_b,
			const spk::Vector3 &p_c,
			const spk::Vector3 &p_d) const;
		[[nodiscard]] VoxelShapePolygon createTriangle(
			const std::string &p_slot,
			const spk::Vector3 &p_a,
			const spk::Vector3 &p_b,
			const spk::Vector3 &p_c) const;
		[[nodiscard]] static VoxelShapeFace createFace(VoxelShapePolygon p_polygon);

		virtual void _constructRenderFaces() = 0;
		virtual void _constructMask() = 0;
		[[nodiscard]] virtual CardinalHeightSet _constructPositiveYHeights() const = 0;
		[[nodiscard]] virtual CardinalHeightSet _constructNegativeYHeights() const;

	public:
		virtual ~VoxelShape() = default;

		VoxelShape(const VoxelShape &) = delete;
		VoxelShape &operator=(const VoxelShape &) = delete;
		VoxelShape(VoxelShape &&) noexcept = default;
		VoxelShape &operator=(VoxelShape &&) noexcept = default;

		void initialize();

		[[nodiscard]] bool initialized() const noexcept;
		[[nodiscard]] const VoxelShapeFaceSet &renderFaces() const noexcept;
		[[nodiscard]] const VoxelShapeMaskSet &maskFaces() const noexcept;
		[[nodiscard]] const CardinalHeightSet &heights(VoxelFlip p_flip) const noexcept;

		[[nodiscard]] static spk::Vector2 atlasUV(const AtlasCell &p_cell, const spk::Vector2 &p_localUV);
	};
}
