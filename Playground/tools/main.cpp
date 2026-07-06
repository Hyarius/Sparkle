#include "tools/core/tool_services.hpp"
#include "tools/core/tools_window.hpp"
#include "tools/pages/voxel_modeler_page.hpp"

#include <filesystem>
#include <iostream>
#include <memory>

#include <sparkle.hpp>

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
		pg::tools::ToolServices services(std::filesystem::path(PG_RESOURCE_DIR) / "data");
		spk::Application application;
		spk::WindowHandle window = application.createWindow(
			"erelia-tools",
			spk::Window::Configuration{
				.rect = spk::Rect2D(60, 60, 1440, 900),
				.title = "Erelia Tools"});

		pg::tools::ToolsWindow tools("EreliaTools", &window.centralWidget(), services);
		tools.setGeometry(window.centralWidget().geometry());
		tools.addPage(std::make_unique<pg::tools::VoxelModelerPage>("VoxelModeler", &tools.pageArea(), services));
		tools.activate();

		std::cout << "Erelia Tools opened. Close the window to exit." << std::endl;
		return application.run();
	} catch (const std::exception &exception)
	{
		std::cerr << "Erelia Tools failed: " << exception.what() << std::endl;
		return 1;
	}
}
