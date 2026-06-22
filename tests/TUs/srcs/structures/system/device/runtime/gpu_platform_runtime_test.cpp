#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>

#include "structures/system/device/window/spk_frame.hpp"
#include "structures/system/device/runtime/spk_opengl_runtime.hpp"
#include "structures/graphics/rendering/context/spk_render_context.hpp"
#include "structures/system/device/window/spk_surface_state.hpp"

namespace
{
	class FrameA : public spk::IFrame
	{
	public:
		FrameA() : spk::IFrame(std::make_shared<spk::SurfaceState>()) {}
		void resize(const spk::Rect2D&) override {}
		void setTitle(const std::string&) override {}
		void hide() override {}
		void requestClosure() override {}
		void validateClosure() override {}
		[[nodiscard]] spk::Rect2D rect() const override { return {}; }
		[[nodiscard]] std::string title() const override { return {}; }
	};

	class FrameB : public spk::IFrame
	{
	public:
		FrameB() : spk::IFrame(std::make_shared<spk::SurfaceState>()) {}
		void resize(const spk::Rect2D&) override {}
		void setTitle(const std::string&) override {}
		void hide() override {}
		void requestClosure() override {}
		void validateClosure() override {}
		[[nodiscard]] spk::Rect2D rect() const override { return {}; }
		[[nodiscard]] std::string title() const override { return {}; }
	};

	class TestRenderContext : public spk::RenderContext
	{
	public:
		explicit TestRenderContext(std::shared_ptr<spk::SurfaceState> p_state)
			: spk::RenderContext(std::move(p_state)) {}
		void makeCurrent() override {}
		void present() override {}
		void setVSync(bool) override {}
		void notifyResize(const spk::Rect2D& p_rect) override { surfaceState()->setRect(p_rect); }
	};

	// A GPU runtime that expects FrameA specifically.
	class FrameARuntime : public spk::GPUPlatformRuntime
	{
	public:
		std::unique_ptr<spk::RenderContext> createRenderContext(spk::IFrame& p_frame) override
		{
			FrameA& frame = _requireFrame<FrameA>(p_frame);
			return std::make_unique<TestRenderContext>(frame.surfaceState());
		}

		void waitUntilWorkDone() override
		{
		}
	};
}

TEST(GPUPlatformRuntimeTest, RequireFrameDoesNotThrowOnMatchingType)
{
	FrameARuntime runtime;
	FrameA frame;
	EXPECT_NO_THROW(runtime.createRenderContext(frame));
}

TEST(GPUPlatformRuntimeTest, RequireFrameThrowsOnMismatchedType)
{
	FrameARuntime runtime;
	FrameB frame;
	EXPECT_THROW(runtime.createRenderContext(frame), std::runtime_error);
}
