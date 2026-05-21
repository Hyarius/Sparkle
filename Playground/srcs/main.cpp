#include <iostream>
#include <string>

#include <sparkle.hpp>

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

namespace
{
	class PlaygroundWidget : public spk::Widget
	{
	private:
		spk::Mesh2D _mesh;

	protected:
		spk::RenderUnit _buildRenderUnit() const override
		{
			spk::RenderUnitBuilder builder;
			builder.emplace<spk::DrawColorMeshRenderCommand>(_mesh, spk::Color(1.0f, 0.0f, 0.0f, 1.0f));
			return builder.build();
		}

	public:
		explicit PlaygroundWidget(spk::Widget* p_parent) :
			spk::Widget("PlaygroundWidget", p_parent)
		{
			_mesh.addShape(
				spk::Vertex2D{.position = {-0.65f, -0.55f}},
				spk::Vertex2D{.position = {0.65f, -0.55f}},
				spk::Vertex2D{.position = {0.0f, 0.65f}});
			activate();
		}
	};
}

#else

namespace
{
	class PlaygroundWidget : public spk::Widget
	{
	protected:
		spk::RenderUnit _buildRenderUnit() const override
		{
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

#endif

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
