#pragma once

#include "structures/game_engine/spk_component_logic.hpp"
#include "structures/voxel/spk_voxel_chunk_streamer.hpp"

#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace spk
{
	class VoxelMap;

	// Reconciles the union of every processable streamer's window. Ownership is scoped:
	// the logic owns chunks it loaded or reactivated, but never claims a chunk that was
	// already manually active. Owned chunks are repaired if manually deactivated. Once
	// they leave the union (or the final streamer disappears), they become inactive when
	// retention is enabled or unload otherwise, and ownership ends. Manually owned chunks
	// remain untouched. The map is then forgotten after zero-streamer cleanup, and weak
	// lifetime tokens prevent reused addresses from inheriting historical state.
	class VoxelChunkStreamerLogic : public spk::ComponentLogic<spk::VoxelChunkStreamer>
	{
	public:
		static constexpr spk::PriorizableTrait::Priority DefaultPriority = 100;
		// Bounds total coordinate expansion, activation, and owned-chunk release across
		// every streamer/map handled by one update.
		static constexpr std::size_t MaximumRequestedChunksPerUpdate =
			spk::VoxelChunkStreamer::MaximumChunkWindowVolume;

	private:
		struct MapStreaming
		{
			std::weak_ptr<const void> lifetime;
			bool seenThisFrame = false;
			std::unordered_set<spk::Vector3Int> wantedChunks;
			std::unordered_set<spk::Vector3Int> ownedChunks;
		};

		bool _retainInactiveChunks = true;
		std::size_t _expandedCoordinatesThisFrame = 0;
		std::unordered_map<spk::VoxelMap *, MapStreaming> _mapStreamings;

	public:
		explicit VoxelChunkStreamerLogic(bool p_retainInactiveChunks = true);

		void setRetainInactiveChunks(bool p_retainInactiveChunks) noexcept;
		[[nodiscard]] bool retainsInactiveChunks() const noexcept;

	protected:
		void _onUpdateStarted(const spk::UpdateContext &p_tick) override;
		void _parseComponentForUpdate(const spk::UpdateContext &p_tick, spk::VoxelChunkStreamer &p_streamer) override;
		void _executeUpdate(const spk::UpdateContext &p_tick) override;
	};
}
