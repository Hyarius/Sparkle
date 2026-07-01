#include <gtest/gtest.h>


#include <memory>
#include <stdexcept>

#include <GL/glew.h>

#include <Windows.h>

#include "structures/graphics/opengl/spk_opengl_render_context.hpp"
#include "structures/system/win32/spk_winapi_frame.hpp"
#include "structures/system/win32/spk_winapi_platform_runtime.hpp"

namespace
{
	class StubRenderContext : public spk::RenderContext
	{
	public:
		explicit StubRenderContext(std::shared_ptr<spk::SurfaceState> p_surfaceState) :
			spk::RenderContext(std::move(p_surfaceState))
		{
		}
	};

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

TEST(OpenGLRenderContextTest, StubContextCoversRegistrySurfaceAndInvalidOperations)
{
	const std::uint64_t generationBefore = spk::RenderContext::deathGeneration();
	const auto surfaceState = std::make_shared<spk::SurfaceState>();
	std::uint64_t contextId = 0;

	{
		StubRenderContext context(surfaceState);
		contextId = context.id();

		EXPECT_EQ(spk::RenderContext::fromId(contextId), &context);
		EXPECT_EQ(context.surfaceState(), surfaceState);
		EXPECT_FALSE(context.isValid());
		EXPECT_FALSE(context.supportsOpenGLCommands());
		EXPECT_THROW(context.makeCurrent(), std::runtime_error);
		EXPECT_NO_THROW(context.scheduleRelease(nullptr));
		EXPECT_NO_THROW(context.flushReleaseQueue());
		EXPECT_NO_THROW(context.present());
		EXPECT_NO_THROW(context.setVSync(true));

		context.notifyResize(spk::Rect2D(0, 0, 12u, 34u));
		EXPECT_EQ(surfaceState->rect(), spk::Rect2D(0, 0, 12u, 34u));

		context.invalidate();
		EXPECT_FALSE(surfaceState->isValid());
	}

	EXPECT_EQ(spk::RenderContext::fromId(contextId), nullptr);
	EXPECT_GT(spk::RenderContext::deathGeneration(), generationBefore);
}

TEST(OpenGLRenderContextTest, BindingCacheTracksAndResetsAllSupportedKinds)
{
	StubRenderContext context(std::make_shared<spk::SurfaceState>());
	const auto *program = reinterpret_cast<const spk::OpenGL::Program *>(1);
	const auto *vertexArray = reinterpret_cast<const spk::OpenGL::VertexArray *>(1);
	const auto *buffer = reinterpret_cast<const spk::OpenGL::Buffer *>(1);

	EXPECT_FALSE(context.isProgramActive(program));
	context.setActiveProgram(program);
	EXPECT_TRUE(context.isProgramActive(program));

	EXPECT_FALSE(context.isVertexArrayActive(vertexArray));
	context.setActiveVertexArray(vertexArray);
	EXPECT_TRUE(context.isVertexArrayActive(vertexArray));

	EXPECT_FALSE(context.isUniformBufferBaseActive(0, buffer));
	context.setActiveUniformBufferBase(0, buffer);
	EXPECT_TRUE(context.isUniformBufferBaseActive(0, buffer));
	context.setActiveUniformBufferBase(8, buffer);
	EXPECT_FALSE(context.isUniformBufferBaseActive(8, buffer));

	context.resetBindingCache();
	EXPECT_FALSE(context.isProgramActive(program));
	EXPECT_FALSE(context.isVertexArrayActive(vertexArray));
	EXPECT_FALSE(context.isUniformBufferBaseActive(0, buffer));
}

