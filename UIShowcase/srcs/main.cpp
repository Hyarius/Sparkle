#include <iostream>

#include <sparkle.hpp>

class MainWidget : public spk::Widget
{
private:

public:
	MainWidget(const std::string& p_name, spk::Widget* p_parent) :
		spk::Widget(p_name, p_parent)
	{

	}
};

int main()
{
	try
	{
		spk::Application application;

		spk::WindowHandle window = application.createWindow(
			"UIShowcaseID",
			spk::Window::Configuration{
				.rect = spk::Rect2D(80, 80, 1280, 800),
				.title = "Sparkle UI Showcase"});

		MainWidget mainWidget = MainWidget("./MainWidget", window.centralWidget());
		mainWidget.setGeometry(window.rect());
		mainWidget.activate();

		std::cout << "Sparkle UI Showcase opened. Close the window to exit." << std::endl;
		return application.run();
	}
	catch (const std::exception &exception)
	{
		std::cerr << "Sparkle UI Showcase failed: " << exception.what() << std::endl;
		return 1;
	}
}
