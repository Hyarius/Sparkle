#include <gtest/gtest.h>

#include <memory>

#include <GL/glew.h>

#include <Windows.h>

#include "structures/graphics/opengl/spk_opengl_render_context.hpp"
#include "structures/graphics/opengl/spk_opengl_vertex_array.hpp"
#include "structures/system/win32/spk_winapi_frame.hpp"
#include "structures/system/win32/spk_winapi_platform_runtime.hpp"

namespace
{
	void pumpWinApiMessages()
	{
		MSG message{};
		while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE) == TRUE)
		{
			TranslateMessage(&message);
			DispatchMessageW(&message);
		}
	}
}

TEST(OpenGLVertexArrayTest, GeneratesNonZeroIdentifier)
{
	spk::PlatformRuntime platformRuntime;
	std::unique_ptr<spk::IFrame> baseFrame = platformRuntime.createFrame(spk::Rect2D(360, 360, 32, 32), "VertexArrayId");
	auto& frame = dynamic_cast<spk::Frame&>(*baseFrame);

	spk::RenderContext context(frame);
	context.makeCurrent();

	spk::OpenGL::VertexArray vertexArray;
	EXPECT_NE(vertexArray.id(), 0u);

	baseFrame->validateClosure();
	pumpWinApiMessages();
}

TEST(OpenGLVertexArrayTest, SkipsDeletionWhenOwningContextIsNotCurrent)
{
	spk::PlatformRuntime platformRuntime;
	std::unique_ptr<spk::IFrame> baseFrame = platformRuntime.createFrame(spk::Rect2D(380, 380, 32, 32), "VertexArrayForeignContext");
	auto& frame = dynamic_cast<spk::Frame&>(*baseFrame);

	spk::RenderContext owningContext(frame);
	spk::RenderContext foreignContext(frame);

	owningContext.makeCurrent();
	auto vertexArray = std::make_unique<spk::OpenGL::VertexArray>();
	EXPECT_NE(vertexArray->id(), 0u);

	// Make a different context current, then destroy the VAO: _ownsCurrentContext()
	// is false, so the destructor must skip glDeleteVertexArrays without crashing.
	foreignContext.makeCurrent();
	EXPECT_NO_THROW(vertexArray.reset());

	baseFrame->validateClosure();
	pumpWinApiMessages();
}
