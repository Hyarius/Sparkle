#include "world/chunk_provider.hpp"

#include "world/chunk.hpp"
#include "world/map_definition.hpp"

namespace pg
{
	MapChunkProvider::MapChunkProvider(const MapDefinition &p_map) :
		_map(p_map)
	{
	}

	void MapChunkProvider::fill(Chunk &p_chunk) const
	{
		const spk::Vector3Int origin = p_chunk.coordinates().worldOrigin();
		for (int y = 0; y < Chunk::Size.y; ++y)
		{
			for (int z = 0; z < Chunk::Size.z; ++z)
			{
				for (int x = 0; x < Chunk::Size.x; ++x)
				{
					const spk::Vector3Int world = origin + spk::Vector3Int{x, y, z};
					p_chunk.grid().cell(x, y, z) =
						_map.grid.isWithinBounds(world) ? _map.grid.cell(world) : VoxelCell{};
				}
			}
		}
		p_chunk.requestSynchronization();
	}
}
