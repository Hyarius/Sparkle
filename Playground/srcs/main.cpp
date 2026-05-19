#include <iostream>

#include <sparkle.hpp>

namespace
{
	class PlaygroundWidget : public spk::Widget
	{
	protected:
		spk::RenderUnit _buildRenderUnit() const override
		{
			std::cout << "PlaygroundWidget: computing an empty render unit." << std::endl;
			return spk::RenderUnit();
		}

	public:
		explicit PlaygroundWidget(spk::Widget* p_parent) :
			spk::Widget("PlaygroundWidget", p_parent)
		{
			activate();
		}
	};
}

int main()
{
	try
	{
		spk::Application application;

		spk::WindowHandle window = application.createWindow(
			"main window",
			spk::Window::Configuration{
				.rect = spk::Rect2D(100, 100, 800, 600),
				.title = "Sparkle Playground"
			});

		PlaygroundWidget widget(&window.centralWidget());
		widget.setGeometry(window.rect().atOrigin());

		std::cout << "Sparkle playground window opened. Close the window to stop the application." << std::endl;
		return application.run();
	}
	catch (const std::exception& exception)
	{
		std::cerr << "Sparkle playground failed: " << exception.what() << std::endl;
		return 1;
	}
}
