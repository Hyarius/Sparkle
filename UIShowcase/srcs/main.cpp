#include <iostream>

#include <sparkle.hpp>

#include "showcase_page_registry.hpp"
#include "showcase_root.hpp"

int main()
{
	try
	{
		spk::Application application;

		spk::WindowHandle window = application.createWindow(
			"Sparkle UI Showcase",
			spk::Window::Configuration{
				.rect = spk::Rect2D(80, 80, 1280, 800),
				.title = "Sparkle UI Showcase"});

		showcase::ShowcasePageRegistry registry;
		showcase::registerDefaultPages(registry);

		showcase::ShowcaseRoot root("ShowcaseRoot", registry, &window.centralWidget());
		root.setGeometry(window.centralWidget().geometry());

		std::cout << "Sparkle UI Showcase opened. Close the window to exit." << std::endl;
		return application.run();
	}
	catch (const std::exception &exception)
	{
		std::cerr << "Sparkle UI Showcase failed: " << exception.what() << std::endl;
		return 1;
	}
}
