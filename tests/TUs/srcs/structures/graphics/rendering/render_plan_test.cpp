#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "structures/graphics/opengl/spk_opengl_clear_command.hpp"
#include "structures/graphics/rendering/command/spk_use_framebuffer_render_command.hpp"
#include "structures/graphics/rendering/pipeline/spk_render_plan.hpp"
#include "structures/graphics/spk_framebuffer_object.hpp"

namespace
{
	class MarkerCommand : public spk::RenderCommand
	{
	public:
		int marker;
		explicit MarkerCommand(int p_marker) :
			marker(p_marker)
		{
		}
		void execute(spk::RenderContext &) override
		{
		}
	};

	spk::Viewport viewport()
	{
		return spk::Viewport(spk::Rect2D(0, 0, 16, 16));
	}

	spk::RenderPassDescription mainDescription(const spk::FrameBufferObject *p_target = nullptr)
	{
		return spk::RenderPassDescription{
			.id = {.kind = spk::RenderPassKind::MainScene, .instance = 0, .debugName = "MainScene"},
			.target = {.frameBuffer = p_target, .viewport = viewport()},
			.clear = {},
			.phases = {
				spk::RenderPhase::PassSetup,
				spk::RenderPhase::SceneOpaque,
				spk::RenderPhase::SceneTransparent,
				spk::RenderPhase::SceneOverlay}};
	}
}

TEST(RenderPlanTest, EmptyPlanIsInspectableAndCompilesToEmptyUnit)
{
	spk::RenderPlan plan;
	EXPECT_TRUE(plan.empty());
	EXPECT_EQ(plan.size(), 0u);
	EXPECT_TRUE(plan.compile().empty());
}

TEST(RenderPlanTest, ValidMainPassKeepsIdentityPhasesAndDiagnostics)
{
	spk::RenderPlan plan;
	spk::RenderPass pass(mainDescription());
	pass.emplace<MarkerCommand>(spk::RenderPhase::SceneOpaque, 7);
	pass.recordContributor(spk::RenderPhase::SceneOpaque);
	plan.addPass(std::move(pass));

	ASSERT_EQ(plan.size(), 1u);
	EXPECT_EQ(plan.passes()[0].description().id.debugName, "MainScene");
	EXPECT_EQ(plan.passes()[0].description().id.instance, 0u);
	ASSERT_EQ(plan.diagnostics().size(), 1u);
	EXPECT_EQ(plan.diagnostics()[0].phases[1].commandCount, 1u);
	EXPECT_EQ(plan.diagnostics()[0].phases[1].contributorCount, 1u);
}

TEST(RenderPlanTest, DuplicatePassIdentityIsRejectedWithoutParsingDebugName)
{
	spk::RenderPlan plan;
	plan.addPass(spk::RenderPass(mainDescription()));
	auto duplicate = mainDescription();
	duplicate.id.debugName = "Different label";
	EXPECT_THROW(plan.addPass(spk::RenderPass(std::move(duplicate))), std::invalid_argument);
}

TEST(RenderPlanTest, MovingPlanTransfersCompilationOwnershipExactlyOnce)
{
	spk::RenderPlan original;
	original.addPass(spk::RenderPass(mainDescription()));
	spk::RenderPlan moved(std::move(original));
	EXPECT_THROW((void)original.compile(), std::logic_error);
	EXPECT_NO_THROW((void)moved.compile());
	EXPECT_THROW((void)moved.compile(), std::logic_error);
}

TEST(RenderPassTest, RejectsDuplicateAndInvalidBuiltInPhaseOrder)
{
	auto duplicate = mainDescription();
	duplicate.phases = {spk::RenderPhase::SceneOpaque, spk::RenderPhase::SceneOpaque};
	EXPECT_THROW(spk::RenderPass(std::move(duplicate)), std::invalid_argument);

	auto setupLate = mainDescription();
	setupLate.phases = {spk::RenderPhase::SceneOpaque, spk::RenderPhase::PassSetup};
	EXPECT_THROW(spk::RenderPass(std::move(setupLate)), std::invalid_argument);

	auto transparentFirst = mainDescription();
	transparentFirst.phases = {spk::RenderPhase::SceneTransparent, spk::RenderPhase::SceneOpaque};
	EXPECT_THROW(spk::RenderPass(std::move(transparentFirst)), std::invalid_argument);
}

TEST(RenderPassTest, CommandsCanOnlyEnterDeclaredPhases)
{
	spk::RenderPass pass(mainDescription());
	EXPECT_NO_THROW(pass.emplace<MarkerCommand>(spk::RenderPhase::SceneOpaque, 1));
	EXPECT_EQ(pass.commandCount(spk::RenderPhase::SceneOpaque), 1u);
	EXPECT_THROW(pass.emplace<MarkerCommand>(spk::RenderPhase::ShadowCaster, 2), std::invalid_argument);
}

TEST(RenderPassTest, TypedClearValidatesAttachmentCapabilitiesAndDepthRange)
{
	spk::FrameBufferObject depthTarget(spk::FrameBufferObject::depthTextureTarget({16, 16}));
	auto depth = mainDescription(&depthTarget);
	depth.id.kind = spk::RenderPassKind::Shadow;
	depth.id.debugName = "Depth";
	depth.phases = {spk::RenderPhase::PassSetup, spk::RenderPhase::ShadowCaster};
	depth.clear.depth = 1.0f;
	EXPECT_NO_THROW({ spk::RenderPass pass{depth}; (void)pass; });

	depth.clear.color = spk::Color(0, 0, 0, 0);
	EXPECT_THROW({ spk::RenderPass pass{depth}; (void)pass; }, std::invalid_argument);
	depth.clear.color.reset();
	depth.clear.depth = 1.1f;
	EXPECT_THROW({ spk::RenderPass pass{depth}; (void)pass; }, std::invalid_argument);
}

TEST(RenderPlanTest, CompileEmitsBindThenClearAndPreservesPassAndPhaseOrder)
{
	spk::FrameBufferObject firstTarget(spk::FrameBufferObject::colorTarget({16, 16}));
	spk::FrameBufferObject secondTarget(spk::FrameBufferObject::depthTextureTarget({16, 16}));

	auto shadowDescription = spk::RenderPassDescription{
		.id = {.kind = spk::RenderPassKind::Shadow, .instance = 0, .debugName = "Shadow[0]"},
		.target = {.frameBuffer = &secondTarget, .viewport = secondTarget.viewport()},
		.clear = {.depth = 1.0f},
		.phases = {spk::RenderPhase::PassSetup, spk::RenderPhase::ShadowCaster}};
	spk::RenderPass shadow(shadowDescription);
	shadow.emplace<MarkerCommand>(spk::RenderPhase::PassSetup, 1);
	shadow.emplace<MarkerCommand>(spk::RenderPhase::ShadowCaster, 2);

	auto main = mainDescription(&firstTarget);
	main.clear = {.color = spk::Color(0, 0, 0, 0), .depth = 1.0f, .stencil = 0};
	spk::RenderPass mainPass(main);
	mainPass.emplace<MarkerCommand>(spk::RenderPhase::SceneOpaque, 3);
	mainPass.emplace<MarkerCommand>(spk::RenderPhase::SceneTransparent, 4);

	spk::RenderPlan plan;
	plan.addPass(std::move(shadow));
	plan.addPass(std::move(mainPass));
	spk::RenderUnit unit = plan.compile();
	ASSERT_EQ(unit.size(), 8u);
	const auto *shadowBind = dynamic_cast<const spk::UseFrameBufferRenderCommand *>(unit.commands()[0].get());
	ASSERT_NE(shadowBind, nullptr);
	EXPECT_EQ(shadowBind->target(), &secondTarget);
	const auto *shadowClear = dynamic_cast<const spk::ClearCommand *>(unit.commands()[1].get());
	ASSERT_NE(shadowClear, nullptr);
	EXPECT_EQ(shadowClear->mask(), GL_DEPTH_BUFFER_BIT);
	EXPECT_FLOAT_EQ(shadowClear->depth(), 1.0f);
	EXPECT_EQ(dynamic_cast<const MarkerCommand *>(unit.commands()[2].get())->marker, 1);
	EXPECT_EQ(dynamic_cast<const MarkerCommand *>(unit.commands()[3].get())->marker, 2);
	const auto *mainBind = dynamic_cast<const spk::UseFrameBufferRenderCommand *>(unit.commands()[4].get());
	ASSERT_NE(mainBind, nullptr);
	EXPECT_EQ(mainBind->target(), &firstTarget);
	const auto *mainClear = dynamic_cast<const spk::ClearCommand *>(unit.commands()[5].get());
	ASSERT_NE(mainClear, nullptr);
	EXPECT_EQ(mainClear->mask(), GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	EXPECT_EQ(mainClear->stencil(), 0u);
	EXPECT_EQ(dynamic_cast<const MarkerCommand *>(unit.commands()[6].get())->marker, 3);
	EXPECT_EQ(dynamic_cast<const MarkerCommand *>(unit.commands()[7].get())->marker, 4);
	EXPECT_THROW((void)plan.compile(), std::logic_error);
}
