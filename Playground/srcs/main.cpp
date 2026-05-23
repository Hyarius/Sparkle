#include <iostream>

#include <sparkle.hpp>
#include "rendering/render_command/spk_text_render_command.hpp"
#include "spk_generated_resources.hpp"

namespace
{
	const spk::Font::Size PlaygroundFontSize(48, 4);

	[[nodiscard]] spk::Font loadPlaygroundFont()
	{
		return spk::Font::fromRawData(SPARKLE_GET_RESOURCE("resources/fonts/arial.ttf"));
	}

	class PlaygroundWidget : public spk::Widget
	{
	private:
		spk::Font _font;

	protected:
		spk::RenderUnit _buildRenderUnit() const override
		{
			const spk::Rect2D widgetGeometry = geometry();
			const spk::Vector2Int center =
				widgetGeometry.anchor + static_cast<spk::Vector2Int>(widgetGeometry.size) / 2;

			spk::RenderUnitBuilder builder;
			builder.emplace<spk::TextRenderCommand>(
				_font,
				"Hello, world!",
				PlaygroundFontSize,
				spk::Color(0.0f, 1.0f, 0.0f, 1.0f),
				spk::Color(1.0f, 0.0f, 0.0f, 1.0f),
				0.0f,
				center,
				spk::HorizontalAlignment::Centered,
				spk::VerticalAlignment::Centered);
			return builder.build();
		}

	public:
		explicit PlaygroundWidget(spk::Widget* p_parent) :
			spk::Widget("PlaygroundWidget", p_parent),
			_font(loadPlaygroundFont())
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
				.title = "Sparkle Playground"});

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
