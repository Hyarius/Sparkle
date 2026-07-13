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
		std::set<spk::VoxelRuntimeId> roadIds{p_registries.voxels().runtimeId("road-block")};
		for (const std::string &biomeId : p_registries.biomes().ids())
		{
			for (const auto &roadBlock : p_registries.biomes().get(biomeId).palette.road)
			{
				roadIds.insert(p_registries.voxels().runtimeId(roadBlock.value.id, roadBlock.value.state));
			}
		}

		int groups = 0;
		int failures = 0;
		// The plan records every committed staircase as a PlanStairway; each composed
		// record is checked against the voxels the realization actually stamps, so
		// plan and realization cannot drift apart silently.
		for (const pg::PlanStairway &stairway : p_plan.stairways)
		{
			if (stairway.steps < 2)
			{
				// One-level ramps carry a record too, but their surroundings (plateau
				// cell, path ends) are not reserved against later placements, so the
				// composed walk checks below do not apply to them.
				continue;
			}
			++groups;
			const int highStand = stairway.topAnchor.y;
			const int lowStand = stairway.bottomAnchor.y;
			const auto standAt = [&](const spk::Vector3Int &p_position) {
				return sampler.standHeight(p_position.x, p_position.z, highStand + 2);
			};

			// 1. The recorded exit beyond the top platform is flush with the plateau.
			if (standAt(stairway.plateauCell) != highStand)
			{
				std::cerr << "FAIL group " << groups << ": top platform is not flush with its plateau exit" << std::endl;
				++failures;
				continue;
			}

			// 2. Follow the recorded route, including switchback landings: every next
			//    column stays level or descends one voxel, ending on the low platform.
			if (stairway.centerPath.empty())
			{
				std::cerr << "FAIL group " << groups << ": staircase has no recorded center path" << std::endl;
				++failures;
				continue;
			}
			int previous = standAt(stairway.centerPath.front());
			bool walkable = previous == highStand;
			for (std::size_t index = 1; index < stairway.centerPath.size() && walkable; ++index)
			{
				const int current = standAt(stairway.centerPath[index]);
				walkable = current == previous || current == previous - 1;
				previous = current;
			}
			if (!walkable || previous != lowStand)
			{
				std::cerr << "FAIL group " << groups << ": recorded stair route is broken (last stand " << previous
						  << ", expected " << lowStand << ")" << std::endl;
				++failures;
				continue;
			}

			// 3. A paved road approach is flat low ground and paved end to end.
			bool bandClear = true;
			if (stairway.pavedApproach.has_value())
			{
				const pg::PlanStairRect &band = *stairway.pavedApproach;
				for (int z = band.minZ; z <= band.maxZ && bandClear; ++z)
				{
					for (int x = band.minX; x <= band.maxX && bandClear; ++x)
					{
						bandClear = standAt({x, 0, z}) == lowStand &&
								roadIds.contains(sampler.at({x, lowStand - 1, z}).id);
					}
				}
			}
			if (!bandClear)
			{
				std::cerr << "FAIL group " << groups << ": approach band obstructed or unpaved" << std::endl;
				++failures;
				continue;
			}
			const char *layout = "one pass";
			switch (stairway.layout)
			{
			case pg::StairLayout::Switchback:
				layout = "switchback";
				break;
			case pg::StairLayout::Perpendicular:
				layout = "perpendicular";
				break;
			case pg::StairLayout::OnePass:
				break;
			}
			std::cout << "ok: composed " << (stairway.road ? "road" : "wild") << " stairway " << groups << " ("
					  << stairway.steps << " levels, " << layout << ")"
					  << std::endl;
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
		spk::CommandLineParser parser;
		using CliOption = spk::CommandLineParser::Option;
		parser.addOption("--seed", {.type = CliOption::Type::String, .help = "world master seed"}, std::string{"1"});
		parser.addOption("--size", {.type = CliOption::Type::Integer, .help = "world size in plan cells"}, std::int64_t{248});
		parser.addOption("--map-only", {.type = CliOption::Type::Flag, .help = "write the world map PNG and exit"}, false);
		parser.addOption(
			"--check-stairs", {.type = CliOption::Type::Flag, .help = "verify composed stairways headless and exit"}, false);

		try
		{
			parser.parse(argc, argv);
		} catch (const spk::CommandLineParser::Error &error)
		{
			std::cerr << error.what() << "\n\n" << parser.usage();
			return 1;
		}

		const spk::JSON::Reader cli(parser.arguments(), "<cli>");
		const std::uint64_t worldSeed = std::stoull(cli.optional<std::string>("seed", "1"));
		const int worldSize = cli.optional<int>("size", 248);
		const bool mapOnly = cli.optional<bool>("map-only", false);
		const bool checkStairs = cli.optional<bool>("check-stairs", false);

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
				registries.townCompositions(),
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
				registries.townCompositions(),
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
