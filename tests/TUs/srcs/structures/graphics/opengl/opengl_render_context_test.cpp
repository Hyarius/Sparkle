#include <gtest/gtest.h>


#include <memory>
#include <stdexcept>

#include <Windows.h>
#include <GL/gl.h>

#include "structures/graphics/opengl/spk_opengl_render_context.hpp"
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

TEST(OpenGLRenderContextTest, ContextLifecycleMakeCurrentResizePresentAndInvalidate)
{
	spk::PlatformRuntime platformRuntime;
	std::unique_ptr<spk::IFrame> baseFrame = platformRuntime.createFrame(spk::Rect2D(340, 340, 64, 48), "RenderContextLifecycle");
	ASSERT_NE(baseFrame, nullptr);
	auto& frame = dynamic_cast<spk::Frame&>(*baseFrame);

	spk::RenderContext context(frame);
	ASSERT_TRUE(context.isValid());
	EXPECT_NO_THROW(context.makeCurrent());

	context.notifyResize(spk::Rect2D(0, 0, 13, 17));
	GLint viewport[4] = {0, 0, 0, 0};
	glGetIntegerv(GL_VIEWPORT, viewport);
	EXPECT_EQ(viewport[0], 0);
	EXPECT_EQ(viewport[1], 0);
	EXPECT_EQ(viewport[2], 13);
	EXPECT_EQ(viewport[3], 17);

	EXPECT_NO_THROW(context.setVSync(true));
	EXPECT_NO_THROW(context.setVSync(false));
	EXPECT_NO_THROW(context.present());

	context.invalidate();
	EXPECT_FALSE(context.isValid());
	EXPECT_THROW(context.makeCurrent(), std::runtime_error);
	EXPECT_NO_THROW(context.notifyResize(spk::Rect2D(0, 0, 1, 1)));
	EXPECT_NO_THROW(context.setVSync(true));
	EXPECT_NO_THROW(context.present());

	baseFrame->validateClosure();
	pumpWinApiMessages();
}

TEST(OpenGLRenderContextTest, CreatingSecondContextOnSameFrameReusesConfiguredPixelFormat)
{
	spk::PlatformRuntime platformRuntime;
	std::unique_ptr<spk::IFrame> baseFrame = platformRuntime.createFrame(spk::Rect2D(360, 360, 32, 32), "RenderContextPixelFormatReuse");
	ASSERT_NE(baseFrame, nullptr);
	auto& frame = dynamic_cast<spk::Frame&>(*baseFrame);

	spk::RenderContext firstContext(frame);
	ASSERT_TRUE(firstContext.isValid());
	firstContext.makeCurrent();

	spk::RenderContext secondContext(frame);
	ASSERT_TRUE(secondContext.isValid());
	EXPECT_NO_THROW(secondContext.makeCurrent());
	EXPECT_NO_THROW(secondContext.notifyResize(spk::Rect2D(0, 0, 7, 9)));

	GLint viewport[4] = {0, 0, 0, 0};
	glGetIntegerv(GL_VIEWPORT, viewport);
	EXPECT_EQ(viewport[2], 7);
	EXPECT_EQ(viewport[3], 9);

	baseFrame->validateClosure();
	pumpWinApiMessages();
}

