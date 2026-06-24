#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include <stb_image_write.h>

#include "utils/image_comparison_test_utils.hpp"
#include "sparkle.hpp"
#include "utils/test_resource_path_utils.hpp"

#include <GL/glew.h>

namespace sparkle_test
{
	class OpenGLTestContext
	{
	private:
		static inline spk::PlatformRuntime* s_platformRuntime = nullptr;
		static inline spk::GPUPlatformRuntime* s_gpuRuntime = nullptr;
		static inline spk::IFrame* s_frame = nullptr;
		static inline spk::RenderContext* s_renderContext = nullptr;

	public:
		explicit OpenGLTestContext(const spk::Rect2D& p_rect = spk::Rect2D(0, 0, 32, 32))
		{
			if (s_platformRuntime == nullptr)
			{
				s_platformRuntime = new spk::PlatformRuntime();
				s_gpuRuntime = new spk::GPUPlatformRuntime();
				s_frame = s_platformRuntime->createFrame(spk::Rect2D(0, 0, 1024, 1024), "Sparkle OpenGL wrapper test").release();
				s_frame->hide();
				s_renderContext = s_gpuRuntime->createRenderContext(*s_frame).release();
			}
			s_renderContext->makeCurrent();
			s_renderContext->notifyResize(p_rect.atOrigin());
		}

		~OpenGLTestContext() = default;

		static void tearDownGlobal()
		{
			if (s_platformRuntime != nullptr)
			{
				s_renderContext->makeCurrent();
				s_gpuRuntime->waitUntilWorkDone();
				delete s_renderContext;
				s_renderContext = nullptr;

				s_frame->validateClosure();
				s_platformRuntime->pollEvents();

				delete s_gpuRuntime;
				delete s_frame;
				delete s_platformRuntime;
				s_gpuRuntime = nullptr;
				s_frame = nullptr;
				s_platformRuntime = nullptr;
			}
		}

		spk::RenderContext& renderContext()
		{
			s_renderContext->makeCurrent();
			return *s_renderContext;
		}

		spk::GPUPlatformRuntime& gpuRuntime()
		{
			return *s_gpuRuntime;
		}
	};

	// Offscreen render target for pixel-assertion tests. The hidden test window's
	// back buffer has undefined pixel ownership, so deterministic readbacks must go
	// through a dedicated framebuffer (same rationale as the widget visual helper).
	class OffscreenRenderTarget
	{
	private:
		GLuint _colorRenderbuffer = 0;
		GLuint _depthStencilRenderbuffer = 0;
		GLuint _framebuffer = 0;

	public:
		OffscreenRenderTarget(GLsizei p_width, GLsizei p_height)
		{
			glGenRenderbuffers(1, &_colorRenderbuffer);
			glBindRenderbuffer(GL_RENDERBUFFER, _colorRenderbuffer);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, p_width, p_height);

			glGenRenderbuffers(1, &_depthStencilRenderbuffer);
			glBindRenderbuffer(GL_RENDERBUFFER, _depthStencilRenderbuffer);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, p_width, p_height);

			glBindRenderbuffer(GL_RENDERBUFFER, 0);

			glGenFramebuffers(1, &_framebuffer);
			glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, _colorRenderbuffer);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, _depthStencilRenderbuffer);
		}

		~OffscreenRenderTarget()
		{
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glDeleteFramebuffers(1, &_framebuffer);
			glDeleteRenderbuffers(1, &_depthStencilRenderbuffer);
			glDeleteRenderbuffers(1, &_colorRenderbuffer);
		}

		OffscreenRenderTarget(const OffscreenRenderTarget&) = delete;
		OffscreenRenderTarget& operator=(const OffscreenRenderTarget&) = delete;

		[[nodiscard]] bool isComplete() const
		{
			return glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
		}
	};

	[[nodiscard]] inline std::array<std::uint8_t, 4> readPixel(int p_x, int p_y)
	{
		std::array<std::uint8_t, 4> pixel{};
		glReadPixels(p_x, p_y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
		return pixel;
	}

	[[nodiscard]] inline std::vector<std::uint8_t> readPixels(int p_width, int p_height)
	{
		std::vector<std::uint8_t> pixels(static_cast<std::size_t>(p_width) * static_cast<std::size_t>(p_height) * 4, 0);
		glReadPixels(0, 0, p_width, p_height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
		return pixels;
	}

	struct TestVertex
	{
		std::array<float, 2> position;
		std::array<float, 3> color;
	};

	inline std::shared_ptr<spk::Program> makeColorProgram()
	{
		return std::make_shared<spk::Program>(
			"#version 330 core\n"
			"layout(location = 0) in vec2 inPosition;\n"
			"layout(location = 1) in vec3 inColor;\n"
			"out vec3 vertexColor;\n"
			"void main()\n"
			"{\n"
			"	vertexColor = inColor;\n"
			"	gl_Position = vec4(inPosition, 0.0, 1.0);\n"
			"}\n",
			"#version 330 core\n"
			"in vec3 vertexColor;\n"
			"out vec4 outColor;\n"
			"void main()\n"
			"{\n"
			"	outColor = vec4(vertexColor, 1.0);\n"
			"}\n");
	}

	inline std::shared_ptr<spk::Program> makeSolidProgram(float p_red, float p_green, float p_blue)
	{
		return std::make_shared<spk::Program>(
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
			"	outColor = vec4(" + std::to_string(p_red) + ", " + std::to_string(p_green) + ", " + std::to_string(p_blue) + ", 1.0);\n"
			"}\n");
	}

	inline std::shared_ptr<spk::VertexBufferObject> makeTriangleVBO(const std::array<TestVertex, 3>& p_vertices)
	{
		auto vertexBuffer = std::make_shared<spk::VertexBufferObject>(
			spk::BufferObject::Usage::StaticDraw,
			sizeof(TestVertex) * p_vertices.size());

		spk::BinaryField vertices = vertexBuffer->field().addArray("vertices", 0, p_vertices.size(), sizeof(TestVertex));
		for (std::size_t i = 0; i < p_vertices.size(); ++i)
		{
			vertices[i] = p_vertices[i];
		}

		return vertexBuffer;
	}

	inline std::shared_ptr<spk::VertexArrayObject> makeTriangleVAO(const std::array<TestVertex, 3>& p_vertices)
	{
		auto vertexBuffer = makeTriangleVBO(p_vertices);
		auto vertexArray = std::make_shared<spk::VertexArrayObject>();
		vertexArray->addVertexBuffer(
			vertexBuffer,
			spk::VertexArrayObject::Attribute{
				.index = 0,
				.componentCount = 2,
				.componentType = GL_FLOAT,
				.normalized = false,
				.stride = sizeof(TestVertex),
				.offset = offsetof(TestVertex, position)
			});
		vertexArray->addVertexBuffer(
			vertexBuffer,
			spk::VertexArrayObject::Attribute{
				.index = 1,
				.componentCount = 3,
				.componentType = GL_FLOAT,
				.normalized = false,
				.stride = sizeof(TestVertex),
				.offset = offsetof(TestVertex, color)
			});

		return vertexArray;
	}

	inline std::array<TestVertex, 3> fullScreenTriangle(std::array<float, 3> p_color)
	{
		return {
			TestVertex{{-1.0f, -1.0f}, p_color},
			TestVertex{{3.0f, -1.0f}, p_color},
			TestVertex{{-1.0f, 3.0f}, p_color}
		};
	}

	inline std::filesystem::path expectedImagePath(const std::string& p_name)
	{
		return spk::test::expectedDirectory() / "OpenGLWrappers" / (p_name + ".png");
	}

	inline std::filesystem::path resultImagePath(const std::string& p_name)
	{
		return spk::test::resultDirectory() / "OpenGLWrappers" / (p_name + ".png");
	}

	inline void writeSolidExpectedImage(const std::filesystem::path& p_path, int p_width, int p_height, std::array<unsigned char, 4> p_color)
	{
		std::vector<unsigned char> pixels(static_cast<std::size_t>(p_width) * static_cast<std::size_t>(p_height) * 4, 0);
		for (std::size_t index = 0; index < pixels.size(); index += 4)
		{
			pixels[index + 0] = p_color[0];
			pixels[index + 1] = p_color[1];
			pixels[index + 2] = p_color[2];
			pixels[index + 3] = p_color[3];
		}
		ASSERT_NE(stbi_write_png(p_path.string().c_str(), p_width, p_height, 4, pixels.data(), p_width * 4), 0);
	}

	inline void writeScissorExpectedImage(const std::filesystem::path& p_path, int p_width, int p_height)
	{
		std::vector<unsigned char> pixels(static_cast<std::size_t>(p_width) * static_cast<std::size_t>(p_height) * 4, 0);
		for (int y = 0; y < p_height; ++y)
		{
			for (int x = 0; x < p_width; ++x)
			{
				const bool inside = x >= 4 && x < 12 && y >= 4 && y < 12;
				const std::size_t index = (static_cast<std::size_t>(p_height - 1 - y) * static_cast<std::size_t>(p_width) + static_cast<std::size_t>(x)) * 4;
				pixels[index + 0] = inside == true ? 255 : 0;
				pixels[index + 1] = 0;
				pixels[index + 2] = inside == true ? 0 : 255;
				pixels[index + 3] = 255;
			}
		}
		ASSERT_NE(stbi_write_png(p_path.string().c_str(), p_width, p_height, 4, pixels.data(), p_width * 4), 0);
	}

	// Saves a screenshot of p_rect from p_context, then compares it against p_expectedPath.
	// If the expected image does not exist yet, the test fails with instructions to copy
	// p_actualPath to p_expectedPath after visual validation.
	inline void validateScreenshot(
		OpenGLTestContext& p_context,
		const spk::Rect2D& p_rect,
		const std::filesystem::path& p_actualPath,
		const std::filesystem::path& p_expectedPath,
		const std::filesystem::path& p_diffPath)
	{
		p_context.gpuRuntime().saveScreenshot(p_actualPath, p_rect);

		if (std::filesystem::exists(p_expectedPath) == false)
		{
			ADD_FAILURE()
				<< "Expected image not found. Visually validate the actual image and copy it to approve.\n"
				<< "  actual:   " << p_actualPath.string() << "\n"
				<< "  expected: " << p_expectedPath.string();
			return;
		}

		const ImageComparisonResult result = compareImages(p_actualPath, p_expectedPath, p_diffPath);
		EXPECT_TRUE(result.matches)
			<< "actual=[" << p_actualPath.string() << "] "
			<< "expected=[" << p_expectedPath.string() << "] "
			<< "diff=[" << p_diffPath.string() << "]";
	}
}

