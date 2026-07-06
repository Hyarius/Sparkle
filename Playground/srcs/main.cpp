#include <algorithm>
#include <iostream>
#include <limits>
#include <optional>
#include <sparkle.hpp>

#include "core/game_context.hpp"
#include "core/registries.hpp"
#include "game_scene_widget.hpp"
#include "world/generator/procedural_world.hpp"
#include "world/generator/procedural_world_image.hpp"
#include "world/generator/worldgen_params.hpp"

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
		std::optional<std::filesystem::path> dumpDirectory;
		std::uint64_t worldSeed = 1;
		for (int i = 1; i < argc; ++i)
		{
			const std::string argument = argv[i];
			if (argument == "--dump-plan")
			{
				dumpDirectory = std::filesystem::path(PG_RESOURCE_DIR).parent_path() / "output";
				if (i + 1 < argc && std::string(argv[i + 1]).starts_with("--") == false)
				{
					dumpDirectory = argv[++i];
				}
			}
			else if (argument == "--seed")
			{
				if (i + 1 >= argc)
				{
					throw std::invalid_argument("--seed requires an unsigned integer");
				}
				worldSeed = std::stoull(argv[++i]);
			}
			else
			{
				throw std::invalid_argument("unknown argument: " + argument);
			}
		}

		if (dumpDirectory)
		{
			const pg::WorldgenParams params = pg::WorldgenParams::load(std::filesystem::path(PG_RESOURCE_DIR) / "data" / "worldgen" / "default.json");
			const pg::ProceduralWorld world = pg::ProceduralWorld::generate(params, worldSeed);
			float minimumHeight = std::numeric_limits<float>::max();
			float maximumHeight = 0.0f;
			double heightSum = 0.0;
			std::size_t landCells = 0;
			std::size_t ports = 0;
			for (int y = 0; y < world.height(); ++y)
			{
				for (int x = 0; x < world.width(); ++x)
				{
					const pg::ProceduralTerrainSample sample = world.sampleTerrain({x, y});
					if (!sample.land)
					{
						continue;
					}
					minimumHeight = std::min(minimumHeight, static_cast<float>(sample.height));
					maximumHeight = std::max(maximumHeight, static_cast<float>(sample.height));
					heightSum += sample.height;
					++landCells;
				}
			}
			for (const pg::ProceduralCity &city : world.cities())
			{
				ports += city.port ? 1u : 0u;
			}
			const std::filesystem::path output = *dumpDirectory / ("macro-world-" + std::to_string(worldSeed) + ".png");
			pg::ProceduralWorldImage::writePng(world, output);
			std::cout << "Wrote " << output << " (world hash " << world.hash() << ")" << std::endl;
			std::cout << "Land height range " << minimumHeight << ".." << maximumHeight
					  << ", mean " << (landCells == 0 ? 0.0 : heightSum / static_cast<double>(landCells)) << std::endl;
			std::cout << "Landmasses " << world.continents().size() << ", cities " << world.cities().size()
					  << " (ports " << ports << ")"
					  << ", routes " << world.roads().size() << ", land ratio "
					  << static_cast<double>(landCells) /
							 static_cast<double>(static_cast<std::size_t>(world.width()) * static_cast<std::size_t>(world.height()))
					  << std::endl;
			std::cout << "Materialized manifest estimate " << world.manifestByteSize() << " bytes" << std::endl;
			return 0;
		}

		pg::Registries registries;
		registries.loadAll(std::filesystem::path(PG_RESOURCE_DIR) / "data");
		pg::GameContext gameContext;
		spk::Application application;

		spk::WindowHandle window = application.createWindow(
			"main window",
			spk::Window::Configuration{
				.rect = spk::Rect2D(100, 100, 800, 600),
				.title = "3D Engine Playground"});

		pg::GameSceneWidget scene("GameScene", &window.centralWidget(), gameContext, registries, worldSeed);
		scene.setGeometry(window.centralWidget().geometry());
		scene.activate();

		std::cout << "Exploration: left-click to move, right-drag or ZQSD to orbit, wheel to zoom. Close the window to exit." << std::endl;
		return application.run();
	} catch (const std::exception &exception)
	{
		std::cerr << "Playground failed: " << exception.what() << std::endl;
		return 1;
	}
}
