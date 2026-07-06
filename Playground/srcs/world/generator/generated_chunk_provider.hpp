#pragma once

#include "world/chunk_provider.hpp"
#include "world/generator/macro_world_plan.hpp"

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

	struct GeneratedStamp
	{
		std::string prefabId;
		PlanCell planCell;
		spk::Vector3Int origin{};
	};

	class GeneratedChunkProvider final : public IChunkProvider
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

		const MacroWorldPlan &_plan;
		const VoxelRegistry &_voxels;
		const Registry<PrefabDefinition> &_prefabs;
		std::uint64_t _seed = 0;
		std::unordered_map<std::string, NumericPalette> _palettes;
		std::vector<int> _surfaceHeights;
		std::vector<std::int8_t> _roadSlopeOrientations;
		std::vector<GeneratedStamp> _stamps;
		std::int32_t _water = -1;
		std::int32_t _road = -1;
		std::int32_t _roadSlope = -1;

		[[nodiscard]] const NumericPalette &_palette(PlanCell p_cell) const;
		[[nodiscard]] bool _columnIntersectsStamp(PlanCell p_cell) const;
		void _prepareRoadHeights();
		void _prepareStamps();

	public:
		GeneratedChunkProvider(
			const MacroWorldPlan &p_plan,
			const Registry<BiomeDefinition> &p_biomes,
			const Registry<PrefabDefinition> &p_prefabs,
			const VoxelRegistry &p_voxels,
			std::uint64_t p_seed);

		void fill(Chunk &p_chunk) const override;

		[[nodiscard]] int surfaceHeight(PlanCell p_cell) const;
		[[nodiscard]] bool shouldPlaceFlora(PlanCell p_cell) const;
		[[nodiscard]] spk::Vector3Int spawnCell() const;
		[[nodiscard]] const std::vector<GeneratedStamp> &stamps() const noexcept;
	};
}
