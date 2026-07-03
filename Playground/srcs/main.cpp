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

#include <iostream>
#include <sparkle.hpp>

#include "core/game_context.hpp"
#include "core/registries.hpp"
#include "game_scene_widget.hpp"

#ifdef _WIN32
#	include <windows.h>
#endif

int main()
{
#ifdef _WIN32
	SetConsoleCP(CP_UTF8);
	SetConsoleOutputCP(CP_UTF8);
#endif

	try
	{
		pg::Registries registries;
		registries.loadAll(std::filesystem::path(PG_RESOURCE_DIR) / "data");
		pg::GameContext gameContext;
		gameContext.newGame(registries);
		spk::Application application;

		spk::WindowHandle window = application.createWindow(
			"main window",
			spk::Window::Configuration{
				.rect = spk::Rect2D(100, 100, 800, 600),
				.title = "3D Engine Playground"});

		pg::GameSceneWidget scene("GameScene", &window.centralWidget(), gameContext, registries);
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
