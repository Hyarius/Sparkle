#include "world/generator/procedural_chunk_provider.hpp"

#include "core/registry.hpp"
#include "world/biome_definition.hpp"
#include "voxel/voxel_registry.hpp"
#include "world/chunk.hpp"
#include "world/prefab_definition.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace
{
	std::uint64_t splitMix64(std::uint64_t p_value) noexcept
	{
		p_value += 0x9e3779b97f4a7c15ULL;
		p_value = (p_value ^ (p_value >> 30)) * 0xbf58476d1ce4e5b9ULL;
		p_value = (p_value ^ (p_value >> 27)) * 0x94d049bb133111ebULL;
		return p_value ^ (p_value >> 31);
	}

	std::uint64_t columnHash(std::uint64_t p_seed, pg::PlanCell p_cell) noexcept
	{
		return splitMix64(
			p_seed ^ (static_cast<std::uint64_t>(static_cast<std::uint32_t>(p_cell.x)) << 32) ^
			static_cast<std::uint32_t>(p_cell.y));
	}

	float densityForBiome(const std::string &p_id)
	{
		if (p_id == "forest")
		{
			return 0.08f;
		}
		if (p_id == "meadow")
		{
			return 0.06f;
		}
		if (p_id == "swamp")
		{
			return 0.05f;
		}
		if (p_id == "volcano" || p_id == "coast")
		{
			return 0.015f;
		}
		return 0.03f;
	}
}

namespace pg
{
	ProceduralChunkProvider::ProceduralChunkProvider(
		const ProceduralWorld &p_world,
		const Registry<BiomeDefinition> &p_biomes,
		const Registry<PrefabDefinition> &p_prefabs,
		const VoxelRegistry &p_voxels,
		std::uint64_t p_seed) :
		_world(p_world),
		_voxels(p_voxels),
		_prefabs(p_prefabs),
		_seed(p_seed)
	{
		_water = _voxels.numericId("water");
		_road = _voxels.numericId("road-block");
		_roadSlope = _voxels.numericId("road-slope");
		for (const std::string &biomeId : _world.params().biomes)
		{
			const BiomeDefinition &biome = p_biomes.get(biomeId);
			NumericPalette palette{
				.surface = _voxels.numericId(biome.palette.surface),
				.subsurface = _voxels.numericId(biome.palette.subsurface),
				.deep = _voxels.numericId(biome.palette.deep),
				.floraDensity = densityForBiome(biomeId)};
			for (const std::string &flora : biome.palette.flora)
			{
				palette.flora.push_back(_voxels.numericId(flora));
			}
			_palettes.emplace(biomeId, std::move(palette));
		}
		_prepareStamps();
	}

	const ProceduralChunkProvider::NumericPalette &ProceduralChunkProvider::_palette(
		const ProceduralTerrainSample &p_sample) const
	{
		const auto found = _palettes.find(p_sample.biome);
		if (found == _palettes.end())
		{
			throw std::runtime_error("procedural cell refers to unknown biome '" + p_sample.biome + "'");
		}
		return found->second;
	}

	void ProceduralChunkProvider::_prepareStamps()
	{
		for (const ProceduralCity &city : _world.cities())
		{
			const PrefabDefinition &prefab = _prefabs.get("gym-shell");
			PlanCell origin{city.cell.x + 9 - prefab.size().x / 2, city.cell.y - prefab.size().z / 2};
			const auto usable = [&](PlanCell p_origin) {
				for (int z = 0; z < prefab.size().z; ++z)
				{
					for (int x = 0; x < prefab.size().x; ++x)
					{
						const ProceduralColumnSample sample = _world.sample({p_origin.x + x, p_origin.y + z});
						if (!sample.land)
						{
							return false;
						}
					}
				}
				return true;
			};
			if (!usable(origin))
			{
				bool found = false;
				for (int radius = 1; radius <= 24 && !found; ++radius)
				{
					for (int z = -radius; z <= radius && !found; ++z)
					{
						for (int x = -radius; x <= radius; ++x)
						{
							if (std::max(std::abs(x), std::abs(z)) == radius && usable({origin.x + x, origin.y + z}))
							{
								origin = {origin.x + x, origin.y + z};
								found = true;
								break;
							}
						}
					}
				}
				if (!found)
				{
					throw std::runtime_error("could not place procedural city prefab");
				}
			}
			int baseHeight = 0;
			for (int z = 0; z < prefab.size().z; ++z)
			{
				for (int x = 0; x < prefab.size().x; ++x)
				{
					baseHeight = std::max(baseHeight, surfaceHeight({origin.x + x, origin.y + z}));
				}
			}
			_stamps.push_back({.prefabId = "gym-shell", .planCell = city.cell, .origin = {origin.x, baseHeight + 1, origin.y}});
		}
	}

	bool ProceduralChunkProvider::_columnIntersectsStamp(PlanCell p_cell) const
	{
		for (const ProceduralStamp &stamp : _stamps)
		{
			const PrefabDefinition &prefab = _prefabs.get(stamp.prefabId);
			if (p_cell.x >= stamp.origin.x && p_cell.y >= stamp.origin.z &&
				p_cell.x < stamp.origin.x + prefab.size().x && p_cell.y < stamp.origin.z + prefab.size().z)
			{
				return true;
			}
		}
		return false;
	}

	int ProceduralChunkProvider::surfaceHeight(PlanCell p_cell) const
	{
		return _world.contains(p_cell) ? _world.surfaceHeight(p_cell) : -1;
	}

	bool ProceduralChunkProvider::shouldPlaceFlora(PlanCell p_cell) const
	{
		if (!_world.contains(p_cell))
		{
			return false;
		}
		const ProceduralColumnSample sample = _world.sample(p_cell);
		if (!sample.land || sample.road || _columnIntersectsStamp(p_cell))
		{
			return false;
		}
		const NumericPalette &palette = _palette(sample);
		return !palette.flora.empty() &&
			   static_cast<float>(columnHash(_seed, p_cell) % 10000ULL) < palette.floraDensity * 10000.0f;
	}

	spk::Vector3Int ProceduralChunkProvider::spawnCell() const
	{
		const PlanCell cell = _world.spawnCell();
		return {cell.x, surfaceHeight(cell), cell.y};
	}

	const std::vector<ProceduralStamp> &ProceduralChunkProvider::stamps() const noexcept
	{
		return _stamps;
	}

	void ProceduralChunkProvider::fill(Chunk &p_chunk) const
	{
		const spk::Vector3Int origin = p_chunk.coordinates().worldOrigin();
		for (int z = 0; z < Chunk::Size.z; ++z)
		{
			for (int x = 0; x < Chunk::Size.x; ++x)
			{
				const PlanCell worldCell{origin.x + x, origin.z + z};
				const ProceduralColumnSample sample = _world.sample(worldCell);
				if (!sample.land)
				{
					continue;
				}
				const NumericPalette &palette = _palette(sample);
				const int top = sample.height;
				for (int y = 0; y < Chunk::Size.y; ++y)
				{
					const int worldY = origin.y + y;
					if (worldY < 0 || worldY > top)
					{
						continue;
					}
					const int depth = top - worldY;
					VoxelCell value;
					value.id = depth == 0 ? palette.surface : (depth <= 3 ? palette.subsurface : palette.deep);
					if (sample.road && depth == 0)
					{
						value.id = sample.roadSlopeOrientation < 0 ? _road : _roadSlope;
						if (sample.roadSlopeOrientation >= 0)
						{
							value.orientation = static_cast<VoxelOrientation>(sample.roadSlopeOrientation);
						}
					}
					p_chunk.grid().cell(x, y, z) = value;
				}
				if (shouldPlaceFlora(worldCell))
				{
					const int localY = top + 1 - origin.y;
					if (localY >= 0 && localY < Chunk::Size.y)
					{
						const auto &flora = palette.flora;
						p_chunk.grid().cell(x, localY, z).id =
							flora[static_cast<std::size_t>(columnHash(_seed, worldCell) % flora.size())];
					}
				}
			}
		}

		const spk::Vector3Int maximum = origin + Chunk::Size;
		for (const ProceduralStamp &stamp : _stamps)
		{
			const PrefabDefinition &prefab = _prefabs.get(stamp.prefabId);
			const spk::Vector3Int stampMaximum = stamp.origin + prefab.size();
			if (stampMaximum.x <= origin.x || stampMaximum.y <= origin.y || stampMaximum.z <= origin.z ||
				stamp.origin.x >= maximum.x || stamp.origin.y >= maximum.y || stamp.origin.z >= maximum.z)
			{
				continue;
			}
			for (int y = 0; y < prefab.size().y; ++y)
			{
				for (int z = 0; z < prefab.size().z; ++z)
				{
					for (int x = 0; x < prefab.size().x; ++x)
					{
						const spk::Vector3Int world = stamp.origin + spk::Vector3Int{x, y, z};
						const spk::Vector3Int local = world - origin;
						if (p_chunk.grid().isWithinBounds(local))
						{
							p_chunk.grid().cell(local) = prefab.grid.cell(x, y, z);
						}
					}
				}
			}
		}
		p_chunk.requestSynchronization();
	}
}
