#pragma once

#include "structures/game_engine/spk_component_logic.hpp"
#include "structures/voxel/spk_voxel_chunk_streamer.hpp"

#include <unordered_map>
#include <unordered_set>

namespace spk
{
	class VoxelMap;

	class VoxelChunkStreamerLogic : public spk::ComponentLogic<spk::VoxelChunkStreamer>
	{
	public:
		static constexpr spk::PriorizableTrait::Priority DefaultPriority = 100;

	private:
		struct MapStreaming
		{
			bool dirty = false;
			std::unordered_set<spk::Vector3Int> wantedChunks;
		};

		bool _retainInactiveChunks = true;
		bool _configurationDirty = false;
		std::unordered_map<spk::VoxelMap *, MapStreaming> _mapStreamings;
		std::unordered_map<spk::VoxelMap *, std::unordered_set<spk::Vector3Int>> _previousWantedChunks;

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
