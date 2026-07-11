#include <cstdint>
#include <iostream>
#include <sparkle.hpp>
#include <stdexcept>
#include <string>

#include "core/game_context.hpp"
#include "core/registries.hpp"
#include "game_scene_widget.hpp"
#include "world/generator/plan_chunk_provider.hpp"
#include "world/generator/world_map_image.hpp"
#include "world/generator/world_plan.hpp"

#include "structures/voxel/spk_voxel_chunk.hpp"

#include <map>
#include <memory>
#include <set>

namespace
{
	// Verification harness (--check-stairs): realizes chunks around every composed
	// staircase and walks it from the high plateau to the low ground, proving the
	// stamped voxels are traversable without opening a window.
	struct VoxelSampler
	{
		const pg::PlanChunkProvider &provider;
		const spk::VoxelRegistry &registry;
		std::map<std::tuple<int, int, int>, std::unique_ptr<spk::VoxelChunk>> chunks;

		const spk::VoxelCell &at(const spk::Vector3Int &p_world)
		{
			const spk::Vector3Int coord = spk::VoxelChunk::coordinatesFromWorldCell(p_world);
			const auto key = std::make_tuple(coord.x, coord.y, coord.z);
			auto found = chunks.find(key);
			if (found == chunks.end())
			{
				auto chunk = std::make_unique<spk::VoxelChunk>(coord, registry);
				provider.fill(*chunk);
				found = chunks.emplace(key, std::move(chunk)).first;
			}
			return found->second->cell(found->second->localFromWorld(p_world));
		}

		int standHeight(int p_x, int p_z, int p_yMax)
		{
			for (int y = p_yMax; y >= 0; --y)
			{
				if (!at({p_x, y, p_z}).isEmpty())
				{
					return y + 1;
				}
			}
			return 0;
		}
	};

	int checkComposedStairways(const pg::Registries &p_registries, const pg::WorldPlan &p_plan)
	{
		const pg::PlanChunkProvider provider(p_registries, p_plan);
		VoxelSampler sampler{provider, p_registries.voxels().renderRegistry(), {}};
		std::set<std::int32_t> roadIds{p_registries.voxels().numericId("road-block")};
		for (const std::string &biomeId : p_registries.biomes().ids())
		{
			for (const pg::BiomePalette::WeightedVoxel &roadBlock : p_registries.biomes().get(biomeId).palette.road)
			{
				roadIds.insert(p_registries.voxels().numericId(roadBlock.id));
			}
		}

		int groups = 0;
		int failures = 0;
		// The plan records every committed composed staircase as a PlanStairway; each
		// record is checked against the voxels the realization actually stamps, so
		// plan and realization cannot drift apart silently.
		for (const pg::PlanStairway &stairway : p_plan.stairways)
		{
			const spk::Vector3Int topAnchor = stairway.topAnchor;
			const spk::Vector3Int bottomAnchor = stairway.bottomAnchor;
			const int steps = stairway.steps;
			const bool alongX = stairway.alongX;
			const int acrossCenter = alongX ? topAnchor.z : topAnchor.x;
			const int topAlong = alongX ? topAnchor.x : topAnchor.z;
			const int bottomAlong = alongX ? bottomAnchor.x : bottomAnchor.z;
			const int tangent = stairway.tangent;
			const int highStand = topAnchor.y;
			const int lowStand = bottomAnchor.y;
			++groups;
			const auto standAt = [&](int p_across, int p_along) {
				return alongX ? sampler.standHeight(p_along, p_across, highStand + 2)
							  : sampler.standHeight(p_across, p_along, highStand + 2);
			};

			// 1. Exactly one across-side of the top platform is the high plateau, flush.
			const bool plateauPositive = standAt(acrossCenter + 2, topAlong) == highStand;
			const bool plateauNegative = standAt(acrossCenter - 2, topAlong) == highStand;
			if (plateauPositive == plateauNegative)
			{
				std::cerr << "FAIL group " << groups << ": top platform not flush against exactly one cliff side"
						  << " (top anchor " << topAnchor.x << "," << topAnchor.y << "," << topAnchor.z
						  << ", highStand " << highStand << ", side+ " << standAt(acrossCenter + 2, topAlong)
						  << ", side- " << standAt(acrossCenter - 2, topAlong) << ", steps " << steps << ")"
						  << std::endl;
				++failures;
				continue;
			}
			const int plateauSide = plateauPositive ? 1 : -1;

			// 2. Walking the strip's middle column descends at most one voxel per cell
			//    and ends on the bottom platform's stand height.
			int previous = standAt(acrossCenter, topAlong);
			bool walkable = previous == highStand;
			for (int along = topAlong + tangent; along != bottomAlong + tangent * 2 && walkable; along += tangent)
			{
				const int current = standAt(acrossCenter, along);
				walkable = current == previous || current == previous - 1;
				previous = current;
			}
			if (!walkable || previous != lowStand)
			{
				std::cerr << "FAIL group " << groups << ": strip walk broken (last stand " << previous
						  << ", expected " << lowStand << ")" << std::endl;
				++failures;
				continue;
			}

			// 3. The three-column approach band beside the structure is flat low
			//    ground end to end; road climbs must also have it paved with a road
			//    block, wild slope climbs keep natural ground.
			const bool roadGroup = stairway.road;
			bool bandClear = true;
			for (int outward = 2; outward <= 4 && bandClear; ++outward)
			{
				const int across = acrossCenter - outward * plateauSide;
				for (int along = topAlong - tangent; along != bottomAlong + tangent * 2 && bandClear; along += tangent)
				{
					bandClear = standAt(across, along) == lowStand;
					if (bandClear && roadGroup)
					{
						const spk::Vector3Int surfacePosition = alongX
																	? spk::Vector3Int{along, lowStand - 1, across}
																	: spk::Vector3Int{across, lowStand - 1, along};
						bandClear = roadIds.contains(sampler.at(surfacePosition).id);
					}
				}
			}
			if (!bandClear)
			{
				std::cerr << "FAIL group " << groups << ": approach band obstructed or unpaved" << std::endl;
				++failures;
				continue;
			}
			std::cout << "ok: composed " << (roadGroup ? "road" : "wild") << " stairway " << groups << " ("
					  << steps << " levels, " << (alongX ? "along x" : "along z") << ")" << std::endl;
		}
		std::cout << groups << " composed stairways checked, " << failures << " failures" << std::endl;
		return failures == 0 && groups > 0 ? 0 : 1;
	}
}

#ifdef _WIN32
#	include <windows.h>
#endif

int main(int argc, char **argv)
{
#ifdef _WIN32
	SetConsoleCP(CP_UTF8);
	SetConsoleOutputCP(CP_UTF8);
#endif

	try
	{
		std::uint64_t worldSeed = 1;
		int worldSize = 124;
		bool mapOnly = false;
		bool checkStairs = false;
		for (int i = 1; i < argc; ++i)
		{
			const std::string argument = argv[i];
			if (argument == "--seed")
			{
				if (i + 1 >= argc)
				{
					throw std::invalid_argument("--seed requires an unsigned integer");
				}
				worldSeed = std::stoull(argv[++i]);
			}
			else if (argument == "--size")
			{
				if (i + 1 >= argc)
				{
					throw std::invalid_argument("--size requires a plan-cell count");
				}
				worldSize = std::stoi(argv[++i]);
			}
			else if (argument == "--map-only")
			{
				mapOnly = true;
			}
			else if (argument == "--check-stairs")
			{
				checkStairs = true;
			}
			else
			{
				throw std::invalid_argument("unknown argument: " + argument);
			}
		}

		// Validate CLI world-generation requests before loading registries or entering
		// any allocation-heavy generation path. generateWorldPlan repeats this same
		// authoritative check for non-CLI callers.
		if (mapOnly || checkStairs)
		{
			pg::WorldGenConfig requestedConfig;
			requestedConfig.masterSeed = worldSeed;
			requestedConfig.size = worldSize;
			pg::validateWorldGenConfig(requestedConfig);
		}

		pg::Registries registries;
		registries.loadAll(std::filesystem::path(PG_RESOURCE_DIR) / "data");

		if (checkStairs)
		{
			pg::WorldGenConfig config;
			config.masterSeed = worldSeed;
			config.size = worldSize;
			const pg::WorldPlan plan = pg::generateWorldPlan(
				config,
				pg::planBiomesFrom(registries.biomes()),
				registries.placementRules(),
				registries.prefabs(),
				registries.interiors());
			return checkComposedStairways(registries, plan);
		}
		if (mapOnly)
		{
			// Generate the world skeleton and its preview image without opening a window;
			// handy to iterate on worldgen parameters and seeds.
			pg::WorldGenConfig config;
			config.masterSeed = worldSeed;
			config.size = worldSize;
			const pg::WorldPlan plan = pg::generateWorldPlan(
				config,
				pg::planBiomesFrom(registries.biomes()),
				registries.placementRules(),
				registries.prefabs(),
				registries.interiors());
			std::cout << plan.report();
			const std::filesystem::path mapPath =
				std::filesystem::path(PG_RESOURCE_DIR).parent_path() / "world_map.png";
			if (!pg::writeWorldMapPng(plan, mapPath))
			{
				std::cerr << "failed to write " << mapPath.generic_string() << std::endl;
				return 1;
			}
			std::cout << "world map written to " << mapPath.generic_string() << std::endl;
			return 0;
		}
		pg::GameContext gameContext;
		spk::Application application;

		spk::WindowHandle window = application.createWindow(
			"main window",
			spk::Window::Configuration{
				.rect = spk::Rect2D(100, 100, 800, 600),
				.title = "Sparkle Voxel Exploration"});

		pg::GameSceneWidget scene("GameScene", &window.centralWidget(), gameContext, registries, worldSeed);
		scene.setGeometry(window.centralWidget().geometry());
		scene.activate();

		std::cout << "Exploration: left-click to move, right-drag or AEZS to orbit, wheel to zoom. "
					 "F7 toggles the overlay. Close the window to exit."
				  << std::endl;
		return application.run();
	} catch (const std::exception &exception)
	{
		std::cerr << "Playground failed: " << exception.what() << std::endl;
		return 1;
	}
}
