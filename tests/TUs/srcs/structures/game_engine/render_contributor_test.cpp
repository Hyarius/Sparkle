#include <gtest/gtest.h>

#include <vector>

#include "structures/game_engine/spk_component_logic_registry.hpp"

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

	template <int Id>
	class TagComponent : public spk::Component
	{
	};

	std::vector<int> g_updates;

	template <int Id, spk::RenderPhaseMask Phases>
	class PhaseLogic : public spk::ComponentLogic<TagComponent<Id>>
	{
	public:
		int renderCalls = 0;
		[[nodiscard]] spk::RenderPhaseMask renderPhases() const noexcept override
		{
			return Phases;
		}

	protected:
		void _onUpdateStarted(const spk::UpdateContext &) override
		{
			g_updates.push_back(Id);
		}
		void _executeRender(const spk::RenderPhaseContext &p_context, spk::RenderPass &p_pass) override
		{
			++renderCalls;
			p_pass.emplace<MarkerCommand>(p_context.phase, Id);
		}
	};

	std::vector<int> dispatch(spk::ComponentLogicRegistry &p_logics, spk::RenderPhase p_phase)
	{
		spk::ComponentRegistry components;
		const spk::Viewport viewport(spk::Rect2D(0, 0, 8, 8));
		const spk::RenderFrameRequest request{
			.mainTarget = {.frameBuffer = nullptr, .viewport = viewport}, .mainClear = {}};
		spk::RenderFrameContext frame{.request = request, .components = components};
		spk::RenderPass pass(spk::RenderPassDescription{.id = {.kind = spk::RenderPassKind::Auxiliary, .instance = 0, .debugName = "Probe"}, .target = request.mainTarget, .clear = {}, .phases = {p_phase}});
		const spk::RenderPhaseContext context{
			.frame = frame, .pass = pass.description(), .phase = p_phase, .passInstance = 0};
		p_logics.renderPhase(context, pass, components);

		std::vector<int> result;
		for (auto &command : pass.takeCommands(p_phase))
		{
			result.push_back(dynamic_cast<MarkerCommand &>(*command).marker);
		}
		return result;
	}
}

TEST(RenderContributorTest, LogicParticipatesOnlyInDeclaredPhases)
{
	constexpr auto opaque = spk::renderPhaseBit(spk::RenderPhase::SceneOpaque);
	spk::ComponentLogicRegistry logics;
	auto &logic = logics.add<PhaseLogic<1, opaque>>();
	EXPECT_EQ(dispatch(logics, spk::RenderPhase::SceneOpaque), (std::vector<int>{1}));
	EXPECT_TRUE(dispatch(logics, spk::RenderPhase::SceneTransparent).empty());
	EXPECT_EQ(logic.renderCalls, 1);
}

TEST(RenderContributorTest, OneLogicCanParticipateInMultiplePhases)
{
	constexpr auto phases = spk::renderPhaseBit(spk::RenderPhase::SceneOpaque) |
							spk::renderPhaseBit(spk::RenderPhase::SceneTransparent);
	spk::ComponentLogicRegistry logics;
	auto &logic = logics.add<PhaseLogic<1, phases>>();
	EXPECT_EQ(dispatch(logics, spk::RenderPhase::SceneOpaque), (std::vector<int>{1}));
	EXPECT_EQ(dispatch(logics, spk::RenderPhase::SceneTransparent), (std::vector<int>{1}));
	EXPECT_EQ(logic.renderCalls, 2);
}

TEST(RenderContributorTest, PhasePriorityIsLocalAndEqualPriorityKeepsRegistrationOrder)
{
	constexpr auto phases = spk::renderPhaseBit(spk::RenderPhase::SceneOpaque) |
							spk::renderPhaseBit(spk::RenderPhase::SceneTransparent);
	spk::ComponentLogicRegistry logics;
	auto &first = logics.add<PhaseLogic<1, phases>>();
	auto &second = logics.add<PhaseLogic<2, phases>>();
	EXPECT_EQ(dispatch(logics, spk::RenderPhase::SceneOpaque), (std::vector<int>{1, 2}));

	second.setRenderPriority(spk::RenderPhase::SceneOpaque, 10);
	EXPECT_EQ(dispatch(logics, spk::RenderPhase::SceneOpaque), (std::vector<int>{2, 1}));
	EXPECT_EQ(dispatch(logics, spk::RenderPhase::SceneTransparent), (std::vector<int>{1, 2}));

	first.setRenderPriority(spk::RenderPhase::SceneOpaque, 10);
	EXPECT_EQ(dispatch(logics, spk::RenderPhase::SceneOpaque), (std::vector<int>{1, 2}));
}

TEST(RenderContributorTest, RenderPriorityDoesNotChangeUpdateOrder)
{
	constexpr auto opaque = spk::renderPhaseBit(spk::RenderPhase::SceneOpaque);
	spk::ComponentLogicRegistry logics;
	logics.add<PhaseLogic<1, opaque>>();
	auto &second = logics.add<PhaseLogic<2, opaque>>();
	second.setRenderPriority(spk::RenderPhase::SceneOpaque, 100);

	g_updates.clear();
	spk::ComponentRegistry components;
	logics.update(spk::UpdateContext{}, components);
	EXPECT_EQ(g_updates, (std::vector<int>{1, 2}));
	EXPECT_EQ(dispatch(logics, spk::RenderPhase::SceneOpaque), (std::vector<int>{2, 1}));
}

TEST(RenderContributorTest, InactiveClearDuplicateAndRepeatedDispatchAreDeterministic)
{
	constexpr auto opaque = spk::renderPhaseBit(spk::RenderPhase::SceneOpaque);
	spk::ComponentLogicRegistry logics;
	auto &first = logics.add<PhaseLogic<1, opaque>>();
	EXPECT_EQ(&first, (&logics.add<PhaseLogic<1, opaque>>()));
	logics.add<PhaseLogic<2, opaque>>().deactivate();
	EXPECT_EQ(dispatch(logics, spk::RenderPhase::SceneOpaque), (std::vector<int>{1}));
	EXPECT_EQ(dispatch(logics, spk::RenderPhase::SceneOpaque), (std::vector<int>{1}));
	logics.clear();
	EXPECT_EQ((logics.get<PhaseLogic<1, opaque>>()), nullptr);
	EXPECT_TRUE(dispatch(logics, spk::RenderPhase::SceneOpaque).empty());
}
