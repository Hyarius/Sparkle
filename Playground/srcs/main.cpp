#include <iostream>
#include <sparkle.hpp>

int main()
{
	spk::Application application;

	spk::WindowHandle window = application.createWindow(
		"main window",
		spk::Window::Configuration{
			.rect = spk::Rect2D(100, 100, 600, 500),
			.title = "Widget Playground"});

	spk::GameEngineWidget gameEngineWidget("GameEngineWidget", &window.centralWidget());

	return application.run();
}
