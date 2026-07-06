#include "world/generator/generated_chunk_provider.hpp"

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
		return splitMix64(p_seed ^ (static_cast<std::uint64_t>(static_cast<std::uint32_t>(p_cell.x)) << 32) ^ static_cast<std::uint32_t>(p_cell.y));
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

	pg::VoxelOrientation opposite(pg::VoxelOrientation p_orientation)
	{
		switch (p_orientation)
		{
		case pg::VoxelOrientation::PositiveX:
			return pg::VoxelOrientation::NegativeX;
		case pg::VoxelOrientation::NegativeX:
			return pg::VoxelOrientation::PositiveX;
		case pg::VoxelOrientation::PositiveZ:
			return pg::VoxelOrientation::NegativeZ;
		case pg::VoxelOrientation::NegativeZ:
			return pg::VoxelOrientation::PositiveZ;
		}
		return pg::VoxelOrientation::PositiveZ;
	}

	pg::VoxelOrientation direction(pg::PlanCell p_from, pg::PlanCell p_to)
	{
		if (p_to.x > p_from.x)
		{
			return pg::VoxelOrientation::PositiveX;
		}
		if (p_to.x < p_from.x)
		{
			return pg::VoxelOrientation::NegativeX;
		}
		if (p_to.y > p_from.y)
		{
			return pg::VoxelOrientation::PositiveZ;
		}
		return pg::VoxelOrientation::NegativeZ;
	}
}

namespace pg
{
	GeneratedChunkProvider::GeneratedChunkProvider(
		const MacroWorldPlan &p_plan,
		const Registry<BiomeDefinition> &p_biomes,
		const Registry<PrefabDefinition> &p_prefabs,
		const VoxelRegistry &p_voxels,
		std::uint64_t p_seed) :
		_plan(p_plan),
		_voxels(p_voxels),
		_prefabs(p_prefabs),
		_seed(p_seed)
	{
		_water = _voxels.numericId("water");
		_road = _voxels.numericId("road-block");
		_roadSlope = _voxels.numericId("road-slope");
		for (const std::string &biomeId : _plan.biomeIds)
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

		_surfaceHeights.resize(_plan.heights.size());
		std::ranges::transform(_plan.heights, _surfaceHeights.begin(), [](float p_height) {
			return std::max(0, static_cast<int>(std::lround(p_height)));
		});
		_roadSlopeOrientations.assign(_plan.heights.size(), -1);
		_prepareRoadHeights();
		_prepareStamps();
	}

	const GeneratedChunkProvider::NumericPalette &GeneratedChunkProvider::_palette(PlanCell p_cell) const
	{
		const PlanInfo info = _plan.queryCell(p_cell);
		const auto found = _palettes.find(info.biome);
		if (found == _palettes.end())
		{
			throw std::runtime_error("generated cell refers to unknown biome '" + info.biome + "'");
		}
		return found->second;
	}

	void GeneratedChunkProvider::_prepareRoadHeights()
	{
		const auto clampGrades = [&] {
			for (int pass = 0; pass < 8; ++pass)
			{
				bool changed = false;
				for (const PlanRoad &road : _plan.roads)
				{
					for (std::size_t i = 1; i < road.cells.size(); ++i)
					{
						int &previous = _surfaceHeights[_plan.index(road.cells[i - 1])];
						int &current = _surfaceHeights[_plan.index(road.cells[i])];
						if (current > previous + 1)
						{
							current = previous + 1;
							changed = true;
						}
						else if (previous > current + 1)
						{
							previous = current + 1;
							changed = true;
						}
					}
					for (std::size_t i = road.cells.size(); i > 1; --i)
					{
						int &previous = _surfaceHeights[_plan.index(road.cells[i - 1])];
						int &current = _surfaceHeights[_plan.index(road.cells[i - 2])];
						if (current > previous + 1)
						{
							current = previous + 1;
							changed = true;
						}
						else if (previous > current + 1)
						{
							previous = current + 1;
							changed = true;
						}
					}
				}
				if (!changed)
				{
					break;
				}
			}
		};
		clampGrades();

		bool resolved = false;
		for (int conflictPass = 0; conflictPass < 64; ++conflictPass)
		{
			std::vector<std::int8_t> desiredOrientations(_roadSlopeOrientations.size(), -1);
			bool conflict = false;
			for (const PlanRoad &road : _plan.roads)
			{
				for (std::size_t i = 1; i < road.cells.size(); ++i)
				{
					const PlanCell a = road.cells[i - 1];
					const PlanCell b = road.cells[i];
					const int aHeight = surfaceHeight(a);
					const int bHeight = surfaceHeight(b);
					if (std::abs(aHeight - bHeight) != 1)
					{
						continue;
					}
					const PlanCell high = aHeight > bHeight ? a : b;
					const PlanCell low = aHeight > bHeight ? b : a;
					const std::size_t highIndex = _plan.index(high);
					const std::int8_t orientation = static_cast<std::int8_t>(opposite(direction(high, low)));
					if (desiredOrientations[highIndex] >= 0 && desiredOrientations[highIndex] != orientation)
					{
						_surfaceHeights[highIndex] = surfaceHeight(low);
						conflict = true;
					}
					else
					{
						desiredOrientations[highIndex] = orientation;
					}
				}
			}
			if (!conflict)
			{
				_roadSlopeOrientations = std::move(desiredOrientations);
				resolved = true;
				break;
			}
			clampGrades();
		}
		if (!resolved)
		{
			throw std::runtime_error("could not resolve generated road slope junctions");
		}
	}

	void GeneratedChunkProvider::_prepareStamps()
	{
		const auto add = [&](const std::string &p_id, PlanCell p_cell, int p_offsetX, int p_offsetZ, bool p_avoidRoads) {
			const PrefabDefinition &prefab = _prefabs.get(p_id);
			PlanCell origin{p_cell.x + p_offsetX - prefab.size().x / 2, p_cell.y + p_offsetZ - prefab.size().z / 2};
			const auto usable = [&](PlanCell p_origin) {
				for (int z = 0; z < prefab.size().z; ++z)
				{
					for (int x = 0; x < prefab.size().x; ++x)
					{
						const PlanCell cell{p_origin.x + x, p_origin.y + z};
						if (!_plan.contains(cell))
						{
							return false;
						}
						const PlanInfo info = _plan.queryCell(cell);
						if (!info.land || (p_avoidRoads && (info.road || info.river)))
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
				for (int radius = 1; radius <= 20 && !found; ++radius)
				{
					for (int dz = -radius; dz <= radius && !found; ++dz)
					{
						for (int dx = -radius; dx <= radius; ++dx)
						{
							if (std::max(std::abs(dx), std::abs(dz)) == radius && usable({origin.x + dx, origin.y + dz}))
							{
								origin = {origin.x + dx, origin.y + dz};
								found = true;
								break;
							}
						}
					}
				}
				if (!found)
				{
					throw std::runtime_error("could not place generated prefab '" + p_id + "' on suitable terrain");
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
			_stamps.push_back({.prefabId = p_id, .planCell = p_cell, .origin = {origin.x, baseHeight + 1, origin.y}});
		};
		for (const PlanSettlement &settlement : _plan.settlements)
		{
			add("town-house", settlement.cell, 6, 6, true);
		}
		for (const PlanCity &city : _plan.cities)
		{
			add("gym-shell", city.cell, 9, 0, true);
		}
		for (const PlanFeature &feature : _plan.features)
		{
			if (feature.type == PlanFeatureType::Tunnel)
			{
				add("tunnel-entrance", feature.cell, 0, 0, false);
			}
		}
	}

	bool GeneratedChunkProvider::_columnIntersectsStamp(PlanCell p_cell) const
	{
		for (const GeneratedStamp &stamp : _stamps)
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

	int GeneratedChunkProvider::surfaceHeight(PlanCell p_cell) const
	{
		return _plan.contains(p_cell) ? _surfaceHeights[_plan.index(p_cell)] : -1;
	}

	bool GeneratedChunkProvider::shouldPlaceFlora(PlanCell p_cell) const
	{
		if (!_plan.contains(p_cell))
		{
			return false;
		}
		const PlanInfo info = _plan.queryCell(p_cell);
		if (!info.land || info.road || info.river || _columnIntersectsStamp(p_cell))
		{
			return false;
		}
		const NumericPalette &palette = _palette(p_cell);
		if (palette.flora.empty())
		{
			return false;
		}
		return static_cast<float>(columnHash(_seed, p_cell) % 10000ULL) < palette.floraDensity * 10000.0f;
	}

	spk::Vector3Int GeneratedChunkProvider::spawnCell() const
	{
		if (_plan.cities.empty())
		{
			throw std::runtime_error("generated world has no start city");
		}
		const PlanCell cell = _plan.cities.front().cell;
		return {cell.x, surfaceHeight(cell), cell.y};
	}

	const std::vector<GeneratedStamp> &GeneratedChunkProvider::stamps() const noexcept
	{
		return _stamps;
	}

	void GeneratedChunkProvider::fill(Chunk &p_chunk) const
	{
		const spk::Vector3Int origin = p_chunk.coordinates().worldOrigin();
		for (int z = 0; z < Chunk::Size.z; ++z)
		{
			for (int x = 0; x < Chunk::Size.x; ++x)
			{
				const PlanCell planCell{origin.x + x, origin.z + z};
				if (!_plan.contains(planCell))
				{
					continue;
				}
				const PlanInfo info = _plan.queryCell(planCell);
				if (!info.land)
				{
					continue;
				}
				const NumericPalette &palette = _palette(planCell);
				const int top = surfaceHeight(planCell);
				const int terrainTop = info.river && !info.road ? top - 1 : top;
				for (int y = 0; y < Chunk::Size.y; ++y)
				{
					const int worldY = origin.y + y;
					if (worldY < 0 || worldY > top)
					{
						continue;
					}
					VoxelCell value;
					if (info.river && !info.road && worldY == top)
					{
						value.id = _water;
					}
					else if (worldY <= terrainTop)
					{
						const int depth = terrainTop - worldY;
						value.id = depth == 0 ? palette.surface : (depth <= 3 ? palette.subsurface : palette.deep);
						if (info.road && depth == 0)
						{
							const std::int8_t slope = _roadSlopeOrientations[_plan.index(planCell)];
							value.id = slope < 0 ? _road : _roadSlope;
							if (slope >= 0)
							{
								value.orientation = static_cast<VoxelOrientation>(slope);
							}
						}
					}
					p_chunk.grid().cell(x, y, z) = value;
				}
				if (shouldPlaceFlora(planCell))
				{
					const int localY = top + 1 - origin.y;
					if (localY >= 0 && localY < Chunk::Size.y)
					{
						const auto &flora = palette.flora;
						p_chunk.grid().cell(x, localY, z).id = flora[static_cast<std::size_t>(columnHash(_seed, planCell) % flora.size())];
					}
				}
			}
		}

		const spk::Vector3Int maximum = origin + Chunk::Size;
		for (const GeneratedStamp &stamp : _stamps)
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
