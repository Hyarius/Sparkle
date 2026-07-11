#include "structures/voxel/spk_voxel_chunk_streamer_logic.hpp"

#include "structures/voxel/spk_voxel_chunk.hpp"
#include "structures/voxel/spk_voxel_map.hpp"

#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace
{
	bool _sameLifetime(
		const std::weak_ptr<const void> &p_lhs,
		const std::weak_ptr<const void> &p_rhs) noexcept
	{
		return p_lhs.owner_before(p_rhs) == false && p_rhs.owner_before(p_lhs) == false;
	}
}

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
	}

	bool VoxelChunkStreamerLogic::retainsInactiveChunks() const noexcept
	{
		return _retainInactiveChunks;
	}

	void VoxelChunkStreamerLogic::_onUpdateStarted(const spk::UpdateContext &p_tick)
	{
		(void)p_tick;
		_expandedCoordinatesThisFrame = 0;
		for (auto &[map, streaming] : _mapStreamings)
		{
			(void)map;
			streaming.seenThisFrame = false;
			streaming.wantedChunks.clear();
		}
	}

	void VoxelChunkStreamerLogic::_parseComponentForUpdate(
		const spk::UpdateContext &p_tick,
		spk::VoxelChunkStreamer &p_streamer)
	{
		(void)p_tick;
		if (p_streamer.hasLiveMap() == false)
		{
			p_streamer.markStreamed();
			return;
		}

		spk::VoxelMap &map = p_streamer.map();
		const std::weak_ptr<const void> lifetime = p_streamer.mapLifetime();
		const std::size_t windowChunkCount = p_streamer.windowChunkCount();
		if (windowChunkCount > MaximumRequestedChunksPerUpdate - _expandedCoordinatesThisFrame)
		{
			throw std::length_error(
				"VoxelChunkStreamer requests exceed the aggregate per-update maximum of " +
				std::to_string(MaximumRequestedChunksPerUpdate) + " expanded coordinates");
		}
		_expandedCoordinatesThisFrame += windowChunkCount;

		auto [iterator, inserted] = _mapStreamings.try_emplace(&map);
		if (inserted == false && _sameLifetime(iterator->second.lifetime, lifetime) == false)
		{
			iterator->second = MapStreaming{};
		}
		MapStreaming &streaming = iterator->second;
		streaming.lifetime = lifetime;
		streaming.seenThisFrame = true;

		const spk::Vector3Int &origin = p_streamer.originPosition();
		const spk::Vector3Int &range = p_streamer.viewRange();
		const std::int64_t minX = static_cast<std::int64_t>(origin.x) - range.x;
		const std::int64_t maxX = static_cast<std::int64_t>(origin.x) + range.x;
		const std::int64_t minY = static_cast<std::int64_t>(origin.y) - range.y;
		const std::int64_t maxY = static_cast<std::int64_t>(origin.y) + range.y;
		const std::int64_t minZ = static_cast<std::int64_t>(origin.z) - range.z;
		const std::int64_t maxZ = static_cast<std::int64_t>(origin.z) + range.z;

		std::vector<spk::Vector3Int> requested;
		requested.reserve(windowChunkCount);
		std::size_t additions = 0;
		for (std::int64_t y = minY; y <= maxY; ++y)
		{
			for (std::int64_t z = minZ; z <= maxZ; ++z)
			{
				for (std::int64_t x = minX; x <= maxX; ++x)
				{
					const spk::Vector3Int coordinates{
						static_cast<std::int32_t>(x),
						static_cast<std::int32_t>(y),
						static_cast<std::int32_t>(z)};
					requested.push_back(coordinates);
					additions += streaming.wantedChunks.contains(coordinates) == false ? 1u : 0u;
				}
			}
		}

		if (streaming.wantedChunks.size() + additions > VoxelChunkStreamer::MaximumChunkWindowVolume)
		{
			throw std::length_error(
				"Combined VoxelChunkStreamer windows exceed the maximum of " +
				std::to_string(VoxelChunkStreamer::MaximumChunkWindowVolume) +
				" chunks for one map");
		}
		streaming.wantedChunks.insert(requested.begin(), requested.end());
		p_streamer.markStreamed();
	}

	void VoxelChunkStreamerLogic::_executeUpdate(const spk::UpdateContext &p_tick)
	{
		(void)p_tick;
		for (auto iterator = _mapStreamings.begin(); iterator != _mapStreamings.end();)
		{
			spk::VoxelMap *map = iterator->first;
			MapStreaming &streaming = iterator->second;
			if (streaming.lifetime.expired())
			{
				iterator = _mapStreamings.erase(iterator);
				continue;
			}

			// This runs on the update thread. Claim mutation ownership of the map (idempotent)
			// before streaming touches any chunk, so chunks warmed up on the construction thread
			// are re-stamped to this thread and freshly generated chunks are stamped from birth.
			map->bindMutationThread();

			if (streaming.seenThisFrame == false)
			{
				for (const spk::Vector3Int &coordinates : streaming.ownedChunks)
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
				iterator = _mapStreamings.erase(iterator);
				continue;
			}

			for (const spk::Vector3Int &coordinates : streaming.wantedChunks)
			{
				spk::VoxelChunk *chunk = map->tryChunk(coordinates);
				if (chunk == nullptr)
				{
					(void)map->chunk(coordinates);
					streaming.ownedChunks.insert(coordinates);
				}
				else if (chunk->isActivated() == false)
				{
					map->setChunkActive(coordinates, true);
					streaming.ownedChunks.insert(coordinates);
				}
			}

			for (auto owned = streaming.ownedChunks.begin(); owned != streaming.ownedChunks.end();)
			{
				if (streaming.wantedChunks.contains(*owned))
				{
					++owned;
					continue;
				}
				if (_retainInactiveChunks)
				{
					map->setChunkActive(*owned, false);
				}
				else
				{
					map->unloadChunk(*owned);
				}
				owned = streaming.ownedChunks.erase(owned);
			}

			++iterator;
		}
	}
}
