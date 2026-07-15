#pragma once

#include "world/generator/town_composition.hpp"
#include "world/generator/world_plan.hpp"

#include <cstdint>
#include <vector>

namespace pg
{
	struct PlanCell
	{
		int row = 0;
		int col = 0;
		friend bool operator==(const PlanCell &, const PlanCell &) = default;
	};
	struct WorldColumn
	{
		int x = 0;
		int z = 0;
		friend bool operator==(const WorldColumn &, const WorldColumn &) = default;
		friend bool operator<(const WorldColumn &p_left, const WorldColumn &p_right)
		{
			return std::pair{p_left.z, p_left.x} < std::pair{p_right.z, p_right.x};
		}
	};
	struct TownColumn
	{
		int x = 0;
		int z = 0;
		friend bool operator==(const TownColumn &, const TownColumn &) = default;
		friend bool operator<(const TownColumn &p_left, const TownColumn &p_right)
		{
			return std::pair{p_left.z, p_left.x} < std::pair{p_right.z, p_right.x};
		}
	};

	enum class TownWorkspaceLayer : std::uint32_t
	{
		Inside = 1u << 0,
		Land = 1u << 1,
		Water = 1u << 2,
		TerrainBlocked = 1u << 3,
		MainRoad = 1u << 4,
		UrbanRoadCenterline = 1u << 5,
		UrbanRoadSurface = 1u << 6,
		BuildingSolid = 1u << 7,
		BuildingClearance = 1u << 8,
		DoorThreshold = 1u << 9,
		DoorApproach = 1u << 10,
		RouteBlocked = 1u << 11,
		RoadsideReserved = 1u << 12,
		RoadScenery = 1u << 13,
		GroundScenery = 1u << 14,
		Dock = 1u << 15,
		Stair = 1u << 16
	};

	[[nodiscard]] constexpr std::uint32_t townLayer(TownWorkspaceLayer p_layer)
	{
		return static_cast<std::uint32_t>(p_layer);
	}

	// A transactional raster at one entry per world voxel column.  It samples only
	// immutable macro data from WorldPlan, so none of its mutations leak out of an
	// unsuccessful layout attempt.
	class TownWorkspace
	{
	private:
		const WorldPlan &_plan;
		PlanTownSite _site;
		TownComposition _composition;
		WorldColumn _minimum{};
		int _width = 0;
		std::vector<std::uint32_t> _layers;
		std::vector<int> _surfaceHeights;

		[[nodiscard]] std::size_t _index(TownColumn p_column) const;
		void _projectMainRoad();

	public:
		TownWorkspace(const WorldPlan &p_plan, PlanTownSite p_site, TownComposition p_composition);

		[[nodiscard]] const PlanTownSite &site() const noexcept
		{
			return _site;
		}
		[[nodiscard]] const TownComposition &composition() const noexcept
		{
			return _composition;
		}
		[[nodiscard]] int width() const noexcept
		{
			return _width;
		}
		[[nodiscard]] TownWorldBounds bounds() const noexcept;
		[[nodiscard]] bool contains(TownColumn p_column) const noexcept;
		[[nodiscard]] TownColumn townFromWorld(WorldColumn p_column) const;
		[[nodiscard]] WorldColumn worldFromTown(TownColumn p_column) const;
		[[nodiscard]] PlanCell planCellFromWorld(WorldColumn p_column) const;
		[[nodiscard]] std::uint32_t layers(TownColumn p_column) const;
		[[nodiscard]] bool has(TownColumn p_column, TownWorkspaceLayer p_layer) const;
		void add(TownColumn p_column, TownWorkspaceLayer p_layer);
		void remove(TownColumn p_column, TownWorkspaceLayer p_layer);
		[[nodiscard]] int surfaceHeight(TownColumn p_column) const;
		[[nodiscard]] std::vector<TownColumn> columnsWith(TownWorkspaceLayer p_layer) const;
		[[nodiscard]] std::vector<PlanPavedColumn> mainRoadSurface() const;
	};
}
