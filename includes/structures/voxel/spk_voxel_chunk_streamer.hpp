#pragma once

#include "structures/game_engine/spk_component.hpp"
#include "structures/math/spk_vector3.hpp"

namespace spk
{
	class VoxelMap;

	// Describes the window of chunks an observer wants active in a VoxelMap: an origin
	// (in chunk coordinates) and an inclusive view range around it, per axis. Attach it to
	// any entity (a player controller typically re-emits setOriginPosition whenever it
	// crosses a chunk boundary); the VoxelChunkStreamerLogic reconciles the map's chunks
	// with the requested window before the render logic runs.
	class VoxelChunkStreamer final : public spk::Component
	{
	private:
		spk::VoxelMap *_map = nullptr;
		spk::Vector3Int _viewRange{0, 0, 0};
		spk::Vector3Int _originPosition{0, 0, 0};
		bool _needsStreaming = true;

	public:
		explicit VoxelChunkStreamer(spk::VoxelMap &p_map);

		// Inclusive radius in chunks: the window spans [origin - range, origin + range]
		// on every axis. A negative component is rejected.
		void setViewRange(const spk::Vector3Int &p_viewRange);
		// Chunk coordinates of the window's center.
		void setOriginPosition(const spk::Vector3Int &p_originChunkPosition);

		[[nodiscard]] const spk::Vector3Int &viewRange() const noexcept;
		[[nodiscard]] const spk::Vector3Int &originPosition() const noexcept;

		[[nodiscard]] spk::VoxelMap &map() noexcept;
		[[nodiscard]] const spk::VoxelMap &map() const noexcept;

		[[nodiscard]] bool needsStreaming() const noexcept;
		void markStreamed() noexcept;
	};
}
