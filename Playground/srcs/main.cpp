#include <iostream>
#include <sparkle.hpp>

class MainWidget
{
private:

public:
	
};

int main()
{
	spk::Application application;

	spk::WindowHandle window = application.createWindow(
		"main window",
		spk::Window::Configuration{
			.rect = spk::Rect2D(100, 100, 600, 500),
			.title = "Widget Playground"});

	

	return application.run();
}
