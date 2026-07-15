#pragma once

#include "battle/presentation/battle_presentation_cell_source.hpp"

#include "structures/graphics/geometry/spk_texture_mesh_3d.hpp"

#include <span>

namespace pg
{
	struct MaskAtlasLayout
	{
		int columns = 4;
		int rows = 4;

		[[nodiscard]] bool contains(const spk::AtlasCell &p_cell) const noexcept;
	};

	inline constexpr MaskAtlasLayout DefaultMaskAtlasLayout{4, 4};

	struct WalkSurfaceMaskInstance
	{
		spk::Vector3Int supportCell;
		spk::AtlasCell atlasCell;
		float lift = 0.02f;
	};

	// Produces a new immutable mesh.  Callers swap the returned value through a shared_ptr rather
	// than mutate a mesh which may still be in use by the render thread.
	class WalkSurfaceMaskMeshBuilder
	{
	public:
		[[nodiscard]] static spk::TextureMesh3D build(
			const IBattlePresentationCellSource &p_cells,
			std::span<const WalkSurfaceMaskInstance> p_instances,
			MaskAtlasLayout p_atlas = DefaultMaskAtlasLayout);

		[[nodiscard]] static spk::TextureMesh3D buildOne(
			const IBattlePresentationCellSource &p_cells,
			const spk::Vector3Int &p_supportCell,
			const spk::AtlasCell &p_atlasCell,
			float p_lift = 0.02f,
			MaskAtlasLayout p_atlas = DefaultMaskAtlasLayout);
	};
}
