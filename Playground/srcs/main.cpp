/*
 ============================================================================
  Sparkle - 2D Engine Playground
 ----------------------------------------------------------------------------
  A minimal slice of the game engine: a ZQSD-controlled animated sprite plus
  two other objects, rendered through a 2D camera into a GameEngineWidget,
  with a DebugOverlay on top.

  Built on the engine model: data-only spk::Component + spk::ComponentLogic<T>.
  Everything here lives in namespace pg:: (Playground), not the spk:: library.

+----------------------------------+------------------------+------------------------------------------------+
| Data component                   | Logic                  | Job                                            |
+----------------------------------+------------------------+------------------------------------------------+
| Transform2D                      | (none)                 | data only                                      |
| (position, rotation, scale,     |                        |                                                |
|  layer)                         |                        |                                                |
+----------------------------------+------------------------+------------------------------------------------+
| SpriteRenderer2D                 | SpriteRenderLogic      | world -> screen via the main                   |
| (mesh, sheet, sprite)            | [render phase]         | camera; emits SpriteRenderCommand              |
+----------------------------------+------------------------+------------------------------------------------+
| AnimationController2D            | AnimationLogic         | advance time -> write the                      |
| (anims, elapsed, playing)        | [update phase]         | current sprite into SpriteRenderer2D           |
+----------------------------------+------------------------+------------------------------------------------+
| PlayerController                 | PlayerControlLogic     | ZQSD -> move Transform2D and                   |
| (speed)                          | [update, high prio]    | pick the walk animation                        |
+----------------------------------+------------------------+------------------------------------------------+
| Camera2D                         | (none)                 | world -> pixel projection; the                 |
| (viewport rect, px/unit)         |                        | widget feeds it geometry()                     |
+----------------------------------+------------------------+------------------------------------------------+

  Camera / viewport  : the camera entity is parented to the player and reads its
	  composed global transform, so it follows the player automatically. It maps
	  world -> a pixel rect inside the widget's geometry().
	  GameSceneWidget pushes its geometry() into the active camera
	  (Camera2D::mainCamera()) on every resize; SpriteRenderLogic reads that
	  camera to project the sprites. No engine-library change was needed.

  Entity2D  : a spk::Entity that owns its local Transform2D. modelTransform()
			  lazily caches the complete parent-composed affine transform and layer.

  GameSceneWidget  : times update / buildRenderUnit, counts sprites & polygons,
	  drives the child DebugOverlay, and re-invalidates its render unit each
	  tick so motion and animation actually redraw.

  Controls  : Z / Q / S / D  ->  up / left / down / right.
 ============================================================================
*/

#include <algorithm>
#include <iostream>
#include <limits>
#include <optional>
#include <set>
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
		bool generatedWorld = true;
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
			else if (argument == "--testground")
			{
				generatedWorld = false;
			}
			else if (argument == "--generated-world")
			{
				generatedWorld = true;
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
		gameContext.newGame(registries);
		gameContext.world.seed = static_cast<int>(worldSeed);
		spk::Application application;

		spk::WindowHandle window = application.createWindow(
			"main window",
			spk::Window::Configuration{
				.rect = spk::Rect2D(100, 100, 800, 600),
				.title = "3D Engine Playground"});

		pg::GameSceneWidget scene("GameScene", &window.centralWidget(), gameContext, registries, generatedWorld, worldSeed);
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
