#include "structures/voxel/spk_voxel_chunk_streamer_logic.hpp"

#include "structures/voxel/spk_voxel_chunk.hpp"
#include "structures/voxel/spk_voxel_map.hpp"

#include <utility>
#include <vector>

namespace spk
{
	VoxelChunkStreamerLogic::VoxelChunkStreamerLogic(bool p_retainInactiveChunks) :
		_retainInactiveChunks(p_retainInactiveChunks)
	{
		setPriority(DefaultPriority);
	}

	void VoxelChunkStreamerLogic::setRetainInactiveChunks(bool p_retainInactiveChunks) noexcept
	{
		if (_retainInactiveChunks == p_retainInactiveChunks)
		{
			return;
		}
		_retainInactiveChunks = p_retainInactiveChunks;
		_configurationDirty = true;
	}

	bool VoxelChunkStreamerLogic::retainsInactiveChunks() const noexcept
	{
		return _retainInactiveChunks;
	}

	void VoxelChunkStreamerLogic::_onUpdateStarted(const spk::UpdateTick &p_tick)
	{
		(void)p_tick;
		_mapStreamings.clear();
	}

	void VoxelChunkStreamerLogic::_parseComponentForUpdate(
		const spk::UpdateTick &p_tick,
		spk::VoxelChunkStreamer &p_streamer)
	{
		(void)p_tick;
		MapStreaming &streaming = _mapStreamings[&p_streamer.map()];
		streaming.dirty = streaming.dirty || p_streamer.needsStreaming();
		p_streamer.markStreamed();

		const spk::Vector3Int &origin = p_streamer.originPosition();
		const spk::Vector3Int &range = p_streamer.viewRange();
		for (int y = origin.y - range.y; y <= origin.y + range.y; ++y)
		{
			for (int z = origin.z - range.z; z <= origin.z + range.z; ++z)
			{
				for (int x = origin.x - range.x; x <= origin.x + range.x; ++x)
				{
					streaming.wantedChunks.insert({x, y, z});
				}
			}
		}
	}

	void VoxelChunkStreamerLogic::_executeUpdate(const spk::UpdateTick &p_tick)
	{
		(void)p_tick;
		for (auto &[map, streaming] : _mapStreamings)
		{
			const auto previous = _previousWantedChunks.find(map);
			streaming.dirty = streaming.dirty || _configurationDirty || previous == _previousWantedChunks.end() ||
							  previous->second != streaming.wantedChunks;
			if (streaming.dirty == false)
			{
				continue;
			}

			for (const spk::Vector3Int &coordinates : streaming.wantedChunks)
			{
				spk::VoxelChunk &chunk = map->chunk(coordinates);
				if (chunk.isActivated() == false)
				{
					map->setChunkActive(coordinates, true);
				}
			}

			std::vector<spk::Vector3Int> unwantedChunks;
			map->forEachLoadedChunk([&](const spk::VoxelChunk &p_chunk) {
				if (streaming.wantedChunks.contains(p_chunk.coordinates()) == false)
				{
					unwantedChunks.push_back(p_chunk.coordinates());
				}
			});

			for (const spk::Vector3Int &coordinates : unwantedChunks)
			{
				if (_retainInactiveChunks)
				{
					map->setChunkActive(coordinates, false);
				}
				else
				{
					map->unloadChunk(coordinates);
				}
			}

			_previousWantedChunks[map] = std::move(streaming.wantedChunks);
		}
		_configurationDirty = false;
	}
}
