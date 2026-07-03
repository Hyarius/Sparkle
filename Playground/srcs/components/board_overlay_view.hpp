#pragma once

#include "battle/board_overlay_state.hpp"
#include "structures/game_engine/spk_component.hpp"
#include "structures/graphics/geometry/spk_texture_mesh_3d.hpp"
#include "structures/math/spk_vector3.hpp"
#include "voxel/voxel_shape.hpp"

#include <array>
#include <cstdint>
#include <memory>

namespace pg
{
	class ICellLookup;

	// Component holding the baked overlay geometry: one translucent TextureMesh3D per overlay
	// category, rebuilt from a BoardOverlayState through VoxelMesher::buildMaskMesh so each
	// highlighted cell drapes over the real voxel surface (flat on cubes, slanted on slopes).
	// BoardOverlayLogic drives rebuildIfNeeded() in the update phase and the draws in render.
	class BoardOverlayView : public spk::Component
	{
	private:
		const BoardOverlayState *_state = nullptr;
		const ICellLookup *_lookup = nullptr; // world-space voxel lookup the masks drape onto
		spk::Vector3Int _worldAnchor{};		  // board-local + anchor == world cell
		std::array<AtlasCell, OverlayCategoryCount> _masks{};
		std::array<std::shared_ptr<const spk::TextureMesh3D>, OverlayCategoryCount> _meshes{};
		std::uint64_t _lastCounter = 0;

	public:
		// Bind to a live battle. masks[i] = atlas cell for category i (from game-rules overlayMasks).
		void bind(
			const BoardOverlayState &p_state,
			const ICellLookup &p_lookup,
			const spk::Vector3Int &p_worldAnchor,
			const std::array<AtlasCell, OverlayCategoryCount> &p_masks);
		void clearBinding();

		[[nodiscard]] bool bound() const noexcept
		{
			return _state != nullptr && _lookup != nullptr;
		}

		// Rebuild every category mesh if the state's change counter moved since the last rebuild.
		// Returns true when a rebuild happened. Safe to call from tests with a grid lookup.
		bool rebuildIfNeeded();

		[[nodiscard]] const std::shared_ptr<const spk::TextureMesh3D> &mesh(OverlayCategory p_category) const noexcept
		{
			return _meshes[static_cast<std::size_t>(p_category)];
		}
	};
}
