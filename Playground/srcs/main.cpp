#include <cstdint>
#include <iostream>
#include <sparkle.hpp>
#include <stdexcept>
#include <string>

#include "core/game_context.hpp"
#include "core/registries.hpp"
#include "game_scene_widget.hpp"
#include "world/generator/world_map_image.hpp"
#include "world/generator/world_plan.hpp"

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
			else
			{
				throw std::invalid_argument("unknown argument: " + argument);
			}
		}

		pg::Registries registries;
		registries.loadAll(std::filesystem::path(PG_RESOURCE_DIR) / "data");

		if (mapOnly)
		{
			// Generate the world skeleton and its preview image without opening a window;
			// handy to iterate on worldgen parameters and seeds.
			pg::WorldGenConfig config;
			config.masterSeed = worldSeed;
			config.size = worldSize;
			const pg::WorldPlan plan = pg::generateWorldPlan(
				config, pg::planBiomesFrom(registries.biomes()), registries.placementRules());
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

		std::cout << "Exploration: left-click to move, right-drag or ZQSD to orbit, wheel to zoom. "
					 "F7 toggles the overlay. Close the window to exit."
				  << std::endl;
		return application.run();
	} catch (const std::exception &exception)
	{
		std::cerr << "Playground failed: " << exception.what() << std::endl;
		return 1;
	}
}
