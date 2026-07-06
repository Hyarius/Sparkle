#include "world/generator/perlin_chunk_provider.hpp"

#include "voxel/voxel_cell.hpp"
#include "voxel/voxel_registry.hpp"

#include "structures/voxel/spk_voxel_chunk.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace pg
{
	PerlinChunkProvider::PerlinChunkProvider(
		const VoxelRegistry &p_voxels, std::uint64_t p_seed, PerlinTerrainParameters p_parameters) :
		_voxels(p_voxels),
		_parameters(p_parameters),
		_height(spk::Perlin::Parameters{
			.seed = static_cast<std::uint32_t>(p_seed ^ (p_seed >> 32)),
			.octaves = p_parameters.octaves,
			.persistence = p_parameters.persistence,
			.lacunarity = p_parameters.lacunarity,
			.frequency = p_parameters.frequency})
	{
		_grass = _voxels.numericId("grass-block");
		_dirt = _voxels.numericId("dirt-block");
		_stone = _voxels.numericId("stone-block");
		_sand = _voxels.numericId("sand-block");
		_water = _voxels.numericId("water");
	}

	int PerlinChunkProvider::surfaceHeight(int p_x, int p_z) const
	{
		// sample2D returns the octave-summed noise remapped to [0, 1].
		const float noise = _height.sample2D(static_cast<float>(p_x), static_cast<float>(p_z), 0.0f, 1.0f);
		return _parameters.baseHeight + static_cast<int>(std::lround(noise * static_cast<float>(_parameters.amplitude)));
	}

	int PerlinChunkProvider::maxHeight() const noexcept
	{
		return _parameters.baseHeight + _parameters.amplitude;
	}

	spk::Vector3Int PerlinChunkProvider::spawnCell() const
	{
		// Spiral outwards from the origin for the first column that sits above the water so the
		// player never spawns submerged.
		for (int radius = 0; radius < 64; ++radius)
		{
			for (int z = -radius; z <= radius; ++z)
			{
				for (int x = -radius; x <= radius; ++x)
				{
					if (std::max(std::abs(x), std::abs(z)) != radius)
					{
						continue;
					}
					const int top = surfaceHeight(x, z);
					if (top > _parameters.seaLevel)
					{
						return {x, top, z};
					}
				}
			}
		}
		return {0, surfaceHeight(0, 0), 0};
	}

	void PerlinChunkProvider::fill(spk::VoxelChunk &p_chunk) const
	{
		const spk::Vector3Int origin = p_chunk.worldOrigin();
		for (int z = 0; z < spk::VoxelChunk::Size.z; ++z)
		{
			for (int x = 0; x < spk::VoxelChunk::Size.x; ++x)
			{
				const int top = surfaceHeight(origin.x + x, origin.z + z);
				for (int y = 0; y < spk::VoxelChunk::Size.y; ++y)
				{
					const int worldY = origin.y + y;
					VoxelCell value;
					if (worldY <= top)
					{
						const int depth = top - worldY;
						if (depth == 0)
						{
							value.id = top <= _parameters.seaLevel ? _sand : _grass;
						}
						else if (depth <= 3)
						{
							value.id = _dirt;
						}
						else
						{
							value.id = _stone;
						}
					}
					else if (worldY <= _parameters.seaLevel)
					{
						value.id = _water;
					}
					else
					{
						continue; // above both terrain and water: leave as air
					}
					p_chunk.grid().cell(x, y, z) = value;
				}
			}
		}
	}
}
