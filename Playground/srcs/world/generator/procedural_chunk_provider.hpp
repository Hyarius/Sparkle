#pragma once

#include "world/chunk_provider.hpp"
#include "world/generator/procedural_world.hpp"

#include "structures/math/spk_vector3.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace pg
{
	struct BiomeDefinition;
	struct PrefabDefinition;
	class VoxelRegistry;
	template <typename TDefinition>
	class Registry;

	struct ProceduralStamp
	{
		std::string prefabId;
		PlanCell planCell;
		spk::Vector3Int origin{};
	};

	class ProceduralChunkProvider final : public IChunkProvider
	{
	private:
		struct NumericPalette
		{
			std::int32_t surface = -1;
			std::int32_t subsurface = -1;
			std::int32_t deep = -1;
			std::vector<std::int32_t> flora;
			float floraDensity = 0.0f;
		};

		const ProceduralWorld &_world;
		const VoxelRegistry &_voxels;
		const Registry<PrefabDefinition> &_prefabs;
		std::uint64_t _seed = 0;
		std::unordered_map<std::string, NumericPalette> _palettes;
		std::vector<ProceduralStamp> _stamps;
		std::int32_t _water = -1;
		std::int32_t _road = -1;
		std::int32_t _roadSlope = -1;

		[[nodiscard]] const NumericPalette &_palette(const ProceduralTerrainSample &p_sample) const;
		[[nodiscard]] bool _columnIntersectsStamp(PlanCell p_cell) const;
		void _prepareStamps();

	public:
		ProceduralChunkProvider(
			const ProceduralWorld &p_world,
			const Registry<BiomeDefinition> &p_biomes,
			const Registry<PrefabDefinition> &p_prefabs,
			const VoxelRegistry &p_voxels,
			std::uint64_t p_seed);

		void fill(Chunk &p_chunk) const override;
		[[nodiscard]] int surfaceHeight(PlanCell p_cell) const;
		[[nodiscard]] bool shouldPlaceFlora(PlanCell p_cell) const;
		[[nodiscard]] spk::Vector3Int spawnCell() const;
		[[nodiscard]] const std::vector<ProceduralStamp> &stamps() const noexcept;
	};
}
