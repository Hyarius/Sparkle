#include <iostream>
#include <string>

#include <sparkle.hpp>
#include "rendering/render_command/spk_draw_texture_mesh_render_command.hpp"
#include "spk_generated_resources.hpp"

namespace
{
	const spk::Font::Size PlaygroundFontSize(32, 5);

	[[nodiscard]] spk::Font::Text playgroundGlyphSet()
	{
		spk::Font::Text result;
		for (spk::Font::Codepoint glyph = U'A'; glyph <= U'G'; ++glyph)
		{
			result.push_back(glyph);
		}
		return result;
	}

	[[nodiscard]] spk::Font loadPlaygroundFont()
	{
		return spk::Font::fromRawData(SPARKLE_GET_RESOURCE("resources/fonts/arial.ttf"));
	}

	[[nodiscard]] spk::TextureMesh2D makeAtlasMesh(
		const spk::Texture &p_atlas,
		const spk::Rect2D &p_screenRectangle)
	{
		constexpr float depth = 0.0f;

		const spk::Vector2UInt atlasSize = p_atlas.size();

		if (atlasSize.x == 0 || atlasSize.y == 0 || p_screenRectangle.size.x == 0 || p_screenRectangle.size.y == 0)
		{
			return {};
		}

		const float textureAspectRatio =
			static_cast<float>(atlasSize.x) / static_cast<float>(atlasSize.y);

		const float availableAspectRatio =
			static_cast<float>(p_screenRectangle.size.x) / static_cast<float>(p_screenRectangle.size.y);

		spk::Vector2Int renderedSize;

		if (availableAspectRatio > textureAspectRatio)
		{
			// Available space is wider than the texture.
			// Match height, reduce width.
			renderedSize.y = static_cast<int>(p_screenRectangle.size.y);
			renderedSize.x = static_cast<int>(
				static_cast<float>(renderedSize.y) * textureAspectRatio);
		}
		else
		{
			// Available space is taller than the texture.
			// Match width, reduce height.
			renderedSize.x = static_cast<int>(p_screenRectangle.size.x);
			renderedSize.y = static_cast<int>(
				static_cast<float>(renderedSize.x) / textureAspectRatio);
		}

		const spk::Vector2Int availableSize =
			static_cast<spk::Vector2Int>(p_screenRectangle.size);

		const spk::Vector2Int offset = (availableSize - renderedSize) / 2;

		const spk::Vector2Int topLeft = p_screenRectangle.anchor + offset;
		const spk::Vector2Int bottomRight = topLeft + renderedSize;

		spk::TextureMesh2D mesh;
		mesh.addShape(
			{spk::Vector3(static_cast<float>(topLeft.x), static_cast<float>(topLeft.y), depth), {0.0f, 0.0f}},
			{spk::Vector3(static_cast<float>(topLeft.x), static_cast<float>(bottomRight.y), depth), {0.0f, 1.0f}},
			{spk::Vector3(static_cast<float>(bottomRight.x), static_cast<float>(bottomRight.y), depth), {1.0f, 1.0f}},
			{spk::Vector3(static_cast<float>(bottomRight.x), static_cast<float>(topLeft.y), depth), {1.0f, 0.0f}});

		return mesh;
	}

	class PlaygroundWidget : public spk::Widget
	{
	private:
		spk::Font _font;
		spk::Font::Atlas *_atlas;

	protected:
		spk::RenderUnit _buildRenderUnit() const override
		{
			std::cout << "Building the render unit for geometry : " << geometry() << std::endl;
			spk::RenderUnitBuilder builder;
			builder.emplace<spk::DrawTextureMeshRenderCommand>(*_atlas, makeAtlasMesh(*_atlas, geometry().shrink({20, 20})));
			return builder.build();
		}

	public:
		explicit PlaygroundWidget(spk::Widget *p_parent) : spk::Widget("PlaygroundWidget", p_parent),
														   _font(loadPlaygroundFont()),
														   _atlas(&_font.atlas(PlaygroundFontSize))
		{
			_atlas->loadGlyphs(playgroundGlyphSet());

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
	catch (const std::exception &exception)
	{
		std::cerr << "Sparkle playground failed: " << exception.what() << std::endl;
		return 1;
	}
}
