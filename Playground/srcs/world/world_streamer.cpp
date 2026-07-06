#include "world/world_streamer.hpp"

#include "world/chunk_provider.hpp"
#include "world/voxel_world.hpp"

#include <algorithm>
#include <set>
#include <stdexcept>
#include <vector>

namespace pg
{
	WorldStreamer::WorldStreamer(
		VoxelWorld &p_world,
		const IChunkProvider &p_provider,
		spk::Vector3Int p_radius,
		std::size_t p_loadBudget) :
		_world(p_world),
		_provider(p_provider),
		_radius(p_radius),
		_loadBudget(p_loadBudget)
	{
		if (_radius.x < 0 || _radius.y < 0 || _radius.z < 0)
		{
			throw std::invalid_argument("World streamer radius cannot be negative");
		}
		if (_loadBudget == 0)
		{
			throw std::invalid_argument("World streamer load budget must be positive");
		}
	}

	void WorldStreamer::update(const spk::Vector3Int &p_focusCell)
	{
		const ChunkCoordinates focus = ChunkCoordinates::fromWorldCell(p_focusCell);
		std::set<ChunkCoordinates> desired;
		for (int y = -_radius.y; y <= _radius.y; ++y)
		{
			for (int z = -_radius.z; z <= _radius.z; ++z)
			{
				for (int x = -_radius.x; x <= _radius.x; ++x)
				{
					desired.insert({focus.value + spk::Vector3Int{x, y, z}});
				}
			}
		}

		for (const ChunkCoordinates &coordinates : _world.loadedChunkCoordinates())
		{
			if (!desired.contains(coordinates))
			{
				(void)_world.unloadChunk(coordinates);
			}
		}
		std::vector<ChunkCoordinates> missing;
		for (const ChunkCoordinates &coordinates : desired)
		{
			if (_world.chunk(coordinates) == nullptr)
			{
				missing.push_back(coordinates);
			}
		}
		std::ranges::sort(missing, [&](const ChunkCoordinates &p_left, const ChunkCoordinates &p_right) {
			const spk::Vector3Int left = p_left.value - focus.value;
			const spk::Vector3Int right = p_right.value - focus.value;
			const int leftDistance = left.x * left.x + left.y * left.y + left.z * left.z;
			const int rightDistance = right.x * right.x + right.y * right.y + right.z * right.z;
			return leftDistance == rightDistance ? p_left < p_right : leftDistance < rightDistance;
		});
		const std::size_t loadCount = std::min(_loadBudget, missing.size());
		for (std::size_t i = 0; i < loadCount; ++i)
		{
			(void)_world.loadChunk(missing[i], _provider);
		}
		_pendingLoadCount = missing.size() - loadCount;
	}

	std::size_t WorldStreamer::pendingLoadCount() const noexcept
	{
		return _pendingLoadCount;
	}
}
