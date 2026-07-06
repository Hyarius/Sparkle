#pragma once

#include "world/chunk_provider.hpp"

#include "structures/math/spk_perlin.hpp"
#include "structures/math/spk_vector3.hpp"

#include <cstdint>

namespace pg
{
	class VoxelRegistry;

	struct PerlinTerrainParameters
	{
		int baseHeight = 8;      // elevation of the flattest terrain
		int amplitude = 24;      // blocks the highest peaks rise above the base
		int seaLevel = 12;       // columns whose surface is below this are flooded
		float frequency = 0.012f;// base octave frequency (smaller == wider hills)
		int octaves = 5;         // number of noise octaves stacked for detail
		float persistence = 0.5f;// amplitude falloff between octaves
		float lacunarity = 2.0f; // frequency growth between octaves
	};

	// Minimal terrain generator: a single multi-octave Perlin field drives the surface
	// elevation of every column. Each column is then filled with grass on top, a few layers of
	// dirt, and stone below; anything under the sea level that is not solid becomes water, and
	// surfaces sitting at or below the sea level are turned into sand for a simple shoreline.
	//
	// The "finer detail" comes for free from the Perlin octaves: octave 1 lays down the broad
	// hills, each successive octave adds a higher-frequency, lower-amplitude ripple on top.
	class PerlinChunkProvider final : public IChunkProvider
	{
	private:
		const VoxelRegistry &_voxels;
		PerlinTerrainParameters _parameters;
		spk::Perlin _height;
		std::int32_t _grass = -1;
		std::int32_t _dirt = -1;
		std::int32_t _stone = -1;
		std::int32_t _sand = -1;
		std::int32_t _water = -1;

	public:
		PerlinChunkProvider(
			const VoxelRegistry &p_voxels, std::uint64_t p_seed, PerlinTerrainParameters p_parameters = {});

		void fill(spk::VoxelChunk &p_chunk) const override;

		[[nodiscard]] int surfaceHeight(int p_x, int p_z) const;
		[[nodiscard]] int maxHeight() const noexcept;
		[[nodiscard]] spk::Vector3Int spawnCell() const;
	};
}
