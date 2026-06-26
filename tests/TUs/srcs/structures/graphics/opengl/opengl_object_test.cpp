#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <optional>

#include <GL/glew.h>

#include <Windows.h>

#include "structures/graphics/opengl/spk_opengl_buffer.hpp"
#include "structures/graphics/opengl/spk_opengl_object.hpp"
#include "structures/graphics/opengl/spk_opengl_program.hpp"
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

	struct TestObject : spk::OpenGL::Object
	{
		using spk::OpenGL::Object::_ownsCurrentContext;
	};

	std::unique_ptr<spk::IFrame> makeFrame(spk::PlatformRuntime &p_runtime, int p_x, int p_y, const std::string &p_title)
	{
		return p_runtime.createFrame(spk::Rect2D(p_x, p_y, 32, 32), p_title);
	}
}

TEST(OpenGLObjectTest, ConstructsWithoutContextAndDoesNotOwnIt)
{
	spk::PlatformRuntime platformRuntime;
	std::unique_ptr<spk::IFrame> baseFrame = makeFrame(platformRuntime, 300, 300, "ObjectNoContext");
	auto& frame = dynamic_cast<spk::Frame&>(*baseFrame);

	{
		spk::RenderContext context(frame);
		context.makeCurrent();
	}

	ASSERT_EQ(spk::RenderContext::current(), nullptr);

	TestObject object;
	EXPECT_EQ(object.contextId(), 0u);
	EXPECT_FALSE(object._ownsCurrentContext());

	baseFrame->validateClosure();
	pumpWinApiMessages();
}

TEST(OpenGLObjectTest, IsContextAliveReflectsRegistry)
{
	spk::PlatformRuntime platformRuntime;
	std::unique_ptr<spk::IFrame> baseFrame = makeFrame(platformRuntime, 320, 320, "ObjectContextAlive");
	auto& frame = dynamic_cast<spk::Frame&>(*baseFrame);

	std::uint64_t contextId = 0;
	{
		spk::RenderContext context(frame);
		contextId = context.id();
		EXPECT_TRUE(spk::OpenGL::isContextAlive(contextId));
	}

	EXPECT_FALSE(spk::OpenGL::isContextAlive(contextId));

	baseFrame->validateClosure();
	pumpWinApiMessages();
}

TEST(OpenGLObjectTest, ReleaseObjectIgnoresNull)
{
	EXPECT_NO_THROW(spk::OpenGL::releaseObject(nullptr));
}

TEST(OpenGLObjectTest, ReleaseObjectSchedulesOnAliveOwnerWhenNotCurrent)
{
	spk::PlatformRuntime platformRuntime;
	std::unique_ptr<spk::IFrame> baseFrame = makeFrame(platformRuntime, 340, 340, "ObjectReleaseScheduled");
	auto& frame = dynamic_cast<spk::Frame&>(*baseFrame);

	spk::RenderContext owningContext(frame);
	spk::RenderContext otherContext(frame);

	owningContext.makeCurrent();
	auto object = std::make_unique<TestObject>();
	EXPECT_EQ(object->contextId(), owningContext.id());

	otherContext.makeCurrent();
	EXPECT_NO_THROW(spk::OpenGL::releaseObject(std::move(object)));

	baseFrame->validateClosure();
	pumpWinApiMessages();
}

TEST(OpenGLObjectTest, ReleaseObjectDropsWrapperWhenOwnerContextIsDead)
{
	spk::PlatformRuntime platformRuntime;
	std::unique_ptr<spk::IFrame> baseFrame = makeFrame(platformRuntime, 360, 360, "ObjectReleaseDead");
	auto& frame = dynamic_cast<spk::Frame&>(*baseFrame);

	std::unique_ptr<TestObject> object;
	{
		spk::RenderContext context(frame);
		context.makeCurrent();
		object = std::make_unique<TestObject>();
	}

	ASSERT_EQ(spk::RenderContext::current(), nullptr);
	EXPECT_NO_THROW(spk::OpenGL::releaseObject(std::move(object)));

	baseFrame->validateClosure();
	pumpWinApiMessages();
}

TEST(OpenGLObjectTest, NotifyDeletionsAreNoOpWithoutCurrentContext)
{
	spk::PlatformRuntime platformRuntime;
	std::unique_ptr<spk::IFrame> baseFrame = makeFrame(platformRuntime, 380, 380, "ObjectNotifyNoContext");
	auto& frame = dynamic_cast<spk::Frame&>(*baseFrame);

	std::optional<spk::OpenGL::Buffer> buffer;
	std::optional<spk::OpenGL::Program> program;
	std::optional<spk::OpenGL::VertexArray> vertexArray;
	{
		spk::RenderContext context(frame);
		context.makeCurrent();
		buffer.emplace(GL_ARRAY_BUFFER, GL_STATIC_DRAW, nullptr, static_cast<std::size_t>(16));
		program.emplace(
			"#version 330 core\nvoid main() { gl_Position = vec4(0.0); }\n",
			"#version 330 core\nout vec4 outColor;\nvoid main() { outColor = vec4(1.0); }\n");
		vertexArray.emplace();
	}

	ASSERT_EQ(spk::RenderContext::current(), nullptr);
	EXPECT_NO_THROW(spk::OpenGL::notifyBufferDeleted(*buffer));
	EXPECT_NO_THROW(spk::OpenGL::notifyProgramDeleted(*program));
	EXPECT_NO_THROW(spk::OpenGL::notifyVertexArrayDeleted(*vertexArray));

	baseFrame->validateClosure();
	pumpWinApiMessages();
}
