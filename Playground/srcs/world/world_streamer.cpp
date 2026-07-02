#include "world/world_streamer.hpp"

#include "world/chunk_provider.hpp"
#include "world/voxel_world.hpp"

#include <set>
#include <stdexcept>

namespace pg
{
	WorldStreamer::WorldStreamer(
		VoxelWorld &p_world,
		const IChunkProvider &p_provider,
		spk::Vector3Int p_radius) :
		_world(p_world),
		_provider(p_provider),
		_radius(p_radius)
	{
		if (_radius.x < 0 || _radius.y < 0 || _radius.z < 0)
		{
			throw std::invalid_argument("World streamer radius cannot be negative");
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
		for (const ChunkCoordinates &coordinates : desired)
		{
			(void)_world.loadChunk(coordinates, _provider);
		}
	}
}
