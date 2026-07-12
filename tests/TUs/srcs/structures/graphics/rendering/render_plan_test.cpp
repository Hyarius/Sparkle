#include <gtest/gtest.h>

#include <memory>
#include <unordered_map>
#include <vector>

#include "structures/game_engine/rendering/spk_shadow_render_pass.hpp"
#include "structures/graphics/opengl/spk_opengl_clear_command.hpp"
#include "structures/graphics/rendering/command/spk_use_framebuffer_render_command.hpp"
#include "structures/graphics/rendering/pass/spk_render_pass_bucket_pack.hpp"
#include "structures/graphics/spk_framebuffer_object.hpp"

namespace
{
	inline constexpr auto TestPass = spk::makeRenderPassTypeId("spk.test.pass");
	inline constexpr auto OtherPass = spk::makeRenderPassTypeId("spk.test.other");

	class MarkerCommand : public spk::RenderCommand
	{
	public:
		int marker;
		explicit MarkerCommand(int p_marker) : marker(p_marker) {}
		void execute(spk::RenderContext &) override {}
	};

	class OtherDerivedPass : public spk::RenderPass
	{
	public:
		using spk::RenderPass::RenderPass;
	};

	spk::Viewport viewport()
	{
		return spk::Viewport(spk::Rect2D(0, 0, 16, 16));
	}

	spk::RenderPass::Description description(const std::string &p_name = "Test")
	{
		return {.debugName = p_name, .target = {.frameBuffer = nullptr, .viewport = viewport()}, .clear = {}};
	}
}

TEST(RenderPassIdentityTest, LiteralIdsAndKeysAreStableAndStrong)
{
	constexpr auto same = spk::makeRenderPassTypeId("spk.test.pass");
	static_assert(TestPass == same);
	static_assert(TestPass != OtherPass);
	EXPECT_EQ(TestPass.value, 18413676259322888550ull);

	const spk::RenderPass::Key base{.type = TestPass, .scope = {7}, .instance = 2};
	EXPECT_EQ(base, (spk::RenderPass::Key{.type = TestPass, .scope = {7}, .instance = 2}));
	EXPECT_NE(base, (spk::RenderPass::Key{.type = TestPass, .scope = {8}, .instance = 2}));
	EXPECT_NE(base, (spk::RenderPass::Key{.type = TestPass, .scope = {7}, .instance = 3}));
	std::unordered_map<spk::RenderPass::Key, int> index{{base, 42}};
	EXPECT_EQ(index.at(base), 42);
}

TEST(RenderPassBucketPackTest, LookupDuplicateTypedAccessAndFilteringAreDeterministic)
{
	spk::RenderPassBucketPack pack;
	EXPECT_TRUE(pack.empty());
	const spk::RenderPass::Key key{.type = TestPass, .scope = {7}};
	auto &pass = pack.emplacePass<spk::RenderPass>(key, 10, "First", description("ignored"));
	EXPECT_THROW(pass.setPriority(11), std::logic_error);
	EXPECT_EQ(&pack.require(key), &pass);
	EXPECT_EQ(pack.find(key), &pass);
	EXPECT_EQ(pack.find({.type = OtherPass}), nullptr);
	EXPECT_THROW((void)pack.require({.type = OtherPass}), std::invalid_argument);
	EXPECT_THROW((void)pack.emplacePass<OtherDerivedPass>(key, 20, "Duplicate", description()), std::invalid_argument);
	ASSERT_EQ(pack.passesOfType(TestPass).size(), 1u);
	ASSERT_EQ(pack.passesOfType(TestPass, spk::RenderPass::ScopeId{7}).size(), 1u);
	EXPECT_TRUE(pack.passesOfType(TestPass, spk::RenderPass::ScopeId{8}).empty());
	EXPECT_THROW((void)pack.require<OtherDerivedPass>(key), std::invalid_argument);
}

TEST(RenderPassBucketPackTest, CanonicalNameRegistrationRejectsHashCollisions)
{
	spk::RenderPassBucketPack pack;
	pack.registerPassType(TestPass, "spk.test.pass");
	EXPECT_NO_THROW(pack.registerPassType(TestPass, "spk.test.pass"));
	EXPECT_THROW(pack.registerPassType(TestPass, "different.canonical.name"), std::invalid_argument);
}

TEST(RenderPassBucketPackTest, PassAndContributorOrderingSurvivesPlanCompilation)
{
	spk::RenderPassBucketPack pack;
	const auto add = [&](spk::RenderPass::TypeId p_type, std::uint32_t p_instance, int p_priority, int p_marker) {
		auto &pass = pack.emplacePass<spk::RenderPass>(
			{.type = p_type, .scope = {4}, .instance = p_instance}, p_priority,
			"Pass", description());
		pass.contribute(0, 0).emplace<MarkerCommand>(p_marker);
		return &pass;
	};
	add(TestPass, 0, 1000, 3);
	add(OtherPass, 0, 100, 1);
	auto *equal = add(OtherPass, 1, 100, 2);
	auto late = equal->contribute(10, 0);
	late.emplace<MarkerCommand>(22);
	auto early = equal->contribute(-10, 99);
	early.emplace<MarkerCommand>(20);
	auto tied = equal->contribute(10, 0);
	tied.emplace<MarkerCommand>(23);

	const auto sorted = pack.diagnostics(true);
	ASSERT_EQ(sorted.size(), 3u);
	EXPECT_EQ(sorted[0].key.instance, 0u);
	EXPECT_EQ(sorted[1].key.instance, 1u);
	EXPECT_EQ(sorted[2].priority, 1000);

	spk::RenderPlan plan = pack.build();
	EXPECT_THROW((void)pack.build(), std::logic_error);
	EXPECT_THROW(late.emplace<MarkerCommand>(99), std::logic_error);
	EXPECT_THROW(equal->setPriority(0), std::logic_error);
	ASSERT_EQ(plan.passes().size(), 3u);
	EXPECT_EQ(plan.passes()[0]->priority(), 100);
	EXPECT_EQ(plan.passes()[1]->declarationOrder(), 2u);

	spk::RenderUnit unit = plan.compile();
	std::vector<int> markers;
	for (const auto &command : unit.commands())
	{
		if (const auto *marker = dynamic_cast<const MarkerCommand *>(command.get()); marker != nullptr)
		{
			markers.push_back(marker->marker);
		}
	}
	EXPECT_EQ(markers, (std::vector<int>{1, 20, 2, 22, 23, 3}));
	EXPECT_THROW((void)plan.compile(), std::logic_error);
}

TEST(RenderPassBucketPackTest, DerivedShadowPassRemainsTypedAndPolymorphic)
{
	spk::FrameBufferObject target(spk::FrameBufferObject::depthTextureTarget({16, 16}));
	spk::RenderPassBucketPack pack;
	const spk::RenderPass::Key key{.type = spk::LightingRenderPasses::DirectionalShadow, .scope = {5}, .instance = 2};
	auto &shadow = pack.emplacePass<spk::ShadowRenderPass>(
		key, 102, "DirectionalShadow[2]",
		{.debugName = "DirectionalShadow[2]", .target = {.frameBuffer = &target, .viewport = target.viewport()}, .clear = {.depth = 1.0f}},
		2u, spk::Matrix4x4::identity());
	EXPECT_EQ(shadow.cascadeIndex(), 2u);
	EXPECT_EQ(&pack.require(key), static_cast<spk::RenderPass *>(&shadow));
	EXPECT_EQ(&pack.require<spk::ShadowRenderPass>(key), &shadow);
	EXPECT_EQ(pack.passesOfType<spk::ShadowRenderPass>(spk::LightingRenderPasses::DirectionalShadow, spk::RenderPass::ScopeId{5}).size(), 1u);
	spk::RenderPlan plan = pack.build();
	ASSERT_NE(dynamic_cast<spk::ShadowRenderPass *>(plan.passes()[0].get()), nullptr);
	EXPECT_NO_THROW((void)plan.compile());
}
