#include <iostream>
#include <sparkle.hpp>

int main()
{
	try
	{
		spk::Application application;

		spk::WindowHandle window = application.createWindow(
			"main window",
			spk::Window::Configuration{
				.rect = spk::Rect2D(100, 100, 600, 500),
				.title = "Widget Playground"});

		spk::WidgetStyle panelStyle = spk::WidgetStyle::makeDefault();

		spk::Panel panel("Panel", panelStyle, &window.centralWidget());
		panel.setGeometry(spk::Rect2D(20, 20, 260, 120));

		spk::WidgetStyle labelStyle = spk::WidgetStyle::makeDefault();
		labelStyle.setTextSize(spk::Font::Size(18, 2));
		labelStyle.setGlyphColor(spk::Color(1.0f, 1.0f, 1.0f, 1.0f));
		labelStyle.setOutlineColor(spk::Color(0.0f, 0.0f, 0.0f, 1.0f));

		spk::TextLabel label("Label", "Hello, Sparkle!", labelStyle, &window.centralWidget());
		label.setGeometry(spk::Rect2D(20, 160, 260, 60));

		spk::WidgetStyle buttonReleased = spk::WidgetStyle::makeDefault();
		buttonReleased.setTextSize(spk::Font::Size(16, 2));
		buttonReleased.setGlyphColor(spk::Color(0.0f, 0.0f, 0.0f, 1.0f));
		buttonReleased.setTextPadding(spk::Vector2Int(8, 4));

		spk::WidgetStyle buttonPressed = spk::WidgetStyle::makeDefaultPressed();
		buttonPressed.setTextSize(spk::Font::Size(16, 2));
		buttonPressed.setGlyphColor(spk::Color(1.0f, 1.0f, 1.0f, 1.0f));
		buttonPressed.setTextPadding(spk::Vector2Int(8, 4));

		spk::PushButton button("Button", "Click Me!", buttonReleased, buttonPressed, &window.centralWidget());
		button.setGeometry(spk::Rect2D(20, 240, 200, 60));

		auto clickContract = button.subscribeToClick([]()
		{
			std::cout << "Button clicked!" << std::endl;
		});

		std::cout << "Widget playground opened. Close the window to exit." << std::endl;
		return application.run();
	}
	catch (const std::exception& exception)
	{
		std::cerr << "Playground failed: " << exception.what() << std::endl;
		return 1;
	}
}
