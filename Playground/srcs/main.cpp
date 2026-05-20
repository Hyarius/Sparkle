#include <array>
#include <cstddef>
#include <iostream>
#include <memory>
#include <string>

#include <sparkle.hpp>

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

#include <GL/glew.h>

namespace
{
	struct TriangleVertex
	{
		std::array<float, 2> position;
	};

	class PlaygroundWidget : public spk::Widget
	{
	private:
		std::shared_ptr<spk::OpenGL::VertexBufferObject> _vertexBuffer =
			std::make_shared<spk::OpenGL::VertexBufferObject>(
				spk::OpenGL::BufferObject::Usage::StaticDraw,
				sizeof(TriangleVertex) * 3);
		std::shared_ptr<spk::OpenGL::VertexArrayObject> _vertexArray =
			std::make_shared<spk::OpenGL::VertexArrayObject>();
		std::shared_ptr<spk::Program> _program = std::make_shared<spk::Program>(
			"#version 330 core\n"
			"layout(location = 0) in vec2 inPosition;\n"
			"void main()\n"
			"{\n"
			"	gl_Position = vec4(inPosition, 0.0, 1.0);\n"
			"}\n",
			"#version 330 core\n"
			"out vec4 outColor;\n"
			"void main()\n"
			"{\n"
			"	outColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
			"}\n");

	protected:
		spk::RenderUnit _buildRenderUnit() const override
		{
			spk::RenderUnitBuilder builder;
			builder.emplace<spk::OpenGL::UseProgramRenderCommand>(_program);
			builder.emplace<spk::OpenGL::BindVertexArrayCommand>(_vertexArray);
			builder.emplace<spk::OpenGL::DrawArraysCommand>(spk::OpenGL::Primitive::Triangles, 0, 3);
			builder.emplace<spk::OpenGL::BindVertexArrayCommand>();
			builder.emplace<spk::OpenGL::UseProgramRenderCommand>();
			return builder.build();
		}

	public:
		explicit PlaygroundWidget(spk::Widget* p_parent) :
			spk::Widget("PlaygroundWidget", p_parent)
		{
			spk::BinaryField vertices = _vertexBuffer->field().addArray("vertices", 0, 3, sizeof(TriangleVertex));
			vertices[0] = TriangleVertex{{-0.65f, -0.55f}};
			vertices[1] = TriangleVertex{{0.65f, -0.55f}};
			vertices[2] = TriangleVertex{{0.0f, 0.65f}};

			_vertexArray->addVertexBuffer(
				_vertexBuffer,
				spk::OpenGL::VertexArrayObject::Attribute{
					.index = 0,
					.componentCount = 2,
					.componentType = GL_FLOAT,
					.normalized = false,
					.stride = sizeof(TriangleVertex),
					.offset = offsetof(TriangleVertex, position)
				});

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
