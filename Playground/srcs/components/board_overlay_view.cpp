#include "components/board_overlay_view.hpp"

#include "voxel/voxel_mesher.hpp"

namespace pg
{
	void BoardOverlayView::bind(
		const BoardOverlayState &p_state,
		const ICellLookup &p_lookup,
		const spk::Vector3Int &p_worldAnchor,
		const std::array<AtlasCell, OverlayCategoryCount> &p_masks)
	{
		_state = &p_state;
		_lookup = &p_lookup;
		_worldAnchor = p_worldAnchor;
		_masks = p_masks;
		_lastCounter = 0; // force a rebuild on the next update
	}

	void BoardOverlayView::clearBinding()
	{
		_state = nullptr;
		_lookup = nullptr;
		for (std::shared_ptr<const spk::TextureMesh3D> &mesh : _meshes)
		{
			mesh.reset();
		}
		_lastCounter = 0;
	}

	bool BoardOverlayView::rebuildIfNeeded()
	{
		if (bound() == false || _state->changeCounter == _lastCounter)
		{
			return false;
		}
		_lastCounter = _state->changeCounter;

		for (std::size_t index = 0; index < OverlayCategoryCount; ++index)
		{
			const std::vector<spk::Vector3Int> &local = _state->layers[index];
			if (local.empty())
			{
				// Swap in an empty mesh rather than mutating: the render thread may still hold the
				// previous one (same discipline as TextureMeshRenderer3D / the exploration hover mesh).
				_meshes[index] = std::make_shared<spk::TextureMesh3D>();
				continue;
			}

			std::vector<spk::Vector3Int> world;
			world.reserve(local.size());
			for (const spk::Vector3Int &cell : local)
			{
				world.push_back(cell + _worldAnchor);
			}

			const AtlasCell mask = _masks[index];
			_meshes[index] = std::make_shared<spk::TextureMesh3D>(VoxelMesher::buildMaskMesh(
				world,
				[mask](const VoxelCell &) {
					return mask;
				},
				*_lookup));
		}
		return true;
	}
}
