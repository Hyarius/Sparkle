#include <gtest/gtest.h>

#include <vector>

#include "structures/game_engine/rendering/spk_scene_render_pipeline.hpp"
#include "structures/game_engine/rendering/spk_scene_render_passes.hpp"
#include "structures/game_engine/spk_component_logic_registry.hpp"
#include "structures/graphics/rendering/pass/spk_render_pass_bucket_pack.hpp"

namespace
{
	class MarkerCommand : public spk::RenderCommand
	{
	public:
		int marker;
		explicit MarkerCommand(int p_marker) : marker(p_marker) {}
		void execute(spk::RenderContext &) override {}
	};

	template <int Id>
	class TagComponent : public spk::Component {};

	std::vector<int> updates;

	template <int Id, bool Transparent = false>
	class PassLogic : public spk::ComponentLogic<TagComponent<Id>>
	{
	public:
		int renderCalls = 0;

	protected:
		void _onUpdateStarted(const spk::UpdateContext &) override
		{
			updates.push_back(Id);
		}

		void _executeRender(const spk::SceneRenderBuildContext &p_context) override
		{
			++renderCalls;
			auto &opaque = p_context.frame.passes.require({.type = spk::SceneRenderPasses::MainOpaque, .scope = p_context.sceneScope});
			opaque.contribute(this->renderPriority(spk::SceneRenderPasses::MainOpaque), p_context.contributorRegistrationOrder).template emplace<MarkerCommand>(Id);
			if constexpr (Transparent)
			{
				auto &transparent = p_context.frame.passes.require({.type = spk::SceneRenderPasses::MainTransparent, .scope = p_context.sceneScope});
				transparent.contribute(this->renderPriority(spk::SceneRenderPasses::MainTransparent), p_context.contributorRegistrationOrder).template emplace<MarkerCommand>(Id * 10);
			}
		}
	};

	std::vector<int> renderMarkers(spk::ComponentLogicRegistry &p_logics)
	{
		spk::ComponentRegistry components;
		spk::RenderPassBucketPack passes;
		spk::RenderFrameBuildContext frame{.passes = passes};
		spk::SceneRenderPipeline pipeline;
		const spk::Viewport viewport(spk::Rect2D(0, 0, 8, 8));
		const spk::SceneRenderFrameRequest request{.mainTarget = {.frameBuffer = nullptr, .viewport = viewport}, .mainClear = {}};
		pipeline.buildPasses(frame, {3}, request, p_logics, components);
		spk::RenderUnit unit = passes.build().compile();
		std::vector<int> result;
		for (const auto &command : unit.commands())
		{
			if (const auto *marker = dynamic_cast<const MarkerCommand *>(command.get()); marker != nullptr)
			{
				result.push_back(marker->marker);
			}
		}
		return result;
	}
}

TEST(RenderContributorTest, LogicIsCollectedOnceAndMayContributeToSeveralPasses)
{
	spk::ComponentLogicRegistry logics;
	auto &logic = logics.add<PassLogic<1, true>>();
	EXPECT_EQ(renderMarkers(logics), (std::vector<int>{1, 10}));
	EXPECT_EQ(logic.renderCalls, 1);
}

TEST(RenderContributorTest, PassSpecificPriorityIsAscendingAndKeepsRegistrationOrderOnTies)
{
	spk::ComponentLogicRegistry logics;
	auto &first = logics.add<PassLogic<1>>();
	auto &second = logics.add<PassLogic<2>>();
	EXPECT_EQ(renderMarkers(logics), (std::vector<int>{1, 2}));
	second.setRenderPriority(spk::SceneRenderPasses::MainOpaque, -10);
	EXPECT_EQ(renderMarkers(logics), (std::vector<int>{2, 1}));
	first.setRenderPriority(spk::SceneRenderPasses::MainOpaque, -10);
	EXPECT_EQ(renderMarkers(logics), (std::vector<int>{1, 2}));
}

TEST(RenderContributorTest, RenderPriorityDoesNotChangeUpdateOrderingAndInactiveLogicDoesNothing)
{
	spk::ComponentLogicRegistry logics;
	logics.add<PassLogic<1>>();
	auto &second = logics.add<PassLogic<2>>();
	second.setRenderPriority(spk::SceneRenderPasses::MainOpaque, -100);
	updates.clear();
	spk::ComponentRegistry components;
	logics.update({}, components);
	EXPECT_EQ(updates, (std::vector<int>{1, 2}));
	second.deactivate();
	EXPECT_EQ(renderMarkers(logics), (std::vector<int>{1}));
}
