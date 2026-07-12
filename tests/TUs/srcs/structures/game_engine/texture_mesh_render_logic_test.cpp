#include <gtest/gtest.h>

#include <memory>

#include "structures/game_engine/spk_camera_3d.hpp"
#include "structures/game_engine/spk_entity_3d.hpp"
#include "structures/game_engine/spk_game_engine.hpp"
#include "structures/game_engine/spk_texture_mesh_render_logic.hpp"
#include "structures/game_engine/spk_texture_mesh_renderer_3d.hpp"
#include "structures/graphics/geometry/spk_primitive_object.hpp"
#include "structures/graphics/rendering/unit/spk_render_unit_builder.hpp"
#include "structures/graphics/spk_framebuffer_object.hpp"

namespace
{
	class TestTextureMeshRenderLogic : public spk::TextureMeshRenderLogic
	{
	public:
		using spk::TextureMeshRenderLogic::_executeRender;
		using spk::TextureMeshRenderLogic::_onRenderStarted;
		using spk::TextureMeshRenderLogic::_parseComponentForRender;
		using spk::TextureMeshRenderLogic::TextureMeshRenderLogic;
	};
}

TEST(TextureMeshRenderLogic, SkipsRenderersWithoutAnActiveCamera)
{
	spk::Entity3D entity;
	spk::TextureMeshRenderer3D &renderer = entity.addComponent<spk::TextureMeshRenderer3D>();
	TestTextureMeshRenderLogic logic;
	logic._onRenderStarted(1);
	logic._parseComponentForRender(renderer);
	spk::RenderUnitBuilder builder;
	logic._executeRender(builder);

	EXPECT_TRUE(builder.empty());
	EXPECT_EQ(spk::TextureMeshRenderLogic::lastMeshCount(), 0u);
}

TEST(TextureMeshRenderLogic, EmitsDrawAndTracksMeshStatistics)
{
	spk::Camera3D camera;
	camera.makeMain();
	spk::Entity3D entity;
	spk::Texture texture;
	auto mesh = std::make_shared<spk::TextureMesh3D>(spk::PrimitiveObject::CreateCube());
	spk::TextureMeshRenderer3D &renderer = entity.addComponent<spk::TextureMeshRenderer3D>();
	renderer.setMesh(mesh);
	renderer.setTexture(&texture);

	TestTextureMeshRenderLogic logic;
	logic._onRenderStarted(1);
	logic._parseComponentForRender(renderer);
	spk::RenderUnitBuilder builder;
	logic._executeRender(builder);

	EXPECT_EQ(builder.size(), 1u);
	EXPECT_EQ(spk::TextureMeshRenderLogic::lastMeshCount(), 1u);
	EXPECT_EQ(spk::TextureMeshRenderLogic::lastTriangleCount(), 12u);
}

TEST(TextureMeshRenderLogic, GeometryLogicDoesNotEmitPassWideFrameState)
{
	spk::Camera3D camera;
	camera.makeMain();
	spk::Entity3D entity;
	spk::Texture texture;
	auto mesh = std::make_shared<spk::TextureMesh3D>(spk::PrimitiveObject::CreateCube());
	spk::TextureMeshRenderer3D &renderer = entity.addComponent<spk::TextureMeshRenderer3D>();
	renderer.setMesh(mesh);
	renderer.setTexture(&texture);

	TestTextureMeshRenderLogic logic;
	logic._onRenderStarted(1);
	logic._parseComponentForRender(renderer);
	spk::RenderUnitBuilder builder;
	logic._executeRender(builder);

	EXPECT_EQ(builder.size(), 1u);
}

TEST(TextureMeshRenderLogic, ExplicitPipelineSeparatesOpaqueAndTranslucentQueues)
{
	spk::Camera3D camera;
	camera.makeMain();
	spk::Texture texture;
	auto mesh = std::make_shared<spk::TextureMesh3D>(spk::PrimitiveObject::CreateCube());
	spk::GameEngine engine;
	engine.add<spk::TextureMeshRenderLogic>();

	spk::Entity3D opaqueEntity;
	auto &opaque = opaqueEntity.addComponent<spk::TextureMeshRenderer3D>();
	opaque.setMesh(mesh);
	opaque.setTexture(&texture);
	engine.addEntity(&opaqueEntity);

	spk::Entity3D transparentEntity;
	auto &transparent = transparentEntity.addComponent<spk::TextureMeshRenderer3D>();
	transparent.setMesh(mesh);
	transparent.setTexture(&texture);
	transparent.setTranslucent(true);
	engine.addEntity(&transparentEntity);

	spk::FrameBufferObject target(spk::FrameBufferObject::colorTarget({16, 16}));
	spk::RenderPlan plan = engine.buildRenderPlan(spk::RenderFrameRequest{.mainTarget = {.frameBuffer = &target, .viewport = target.viewport()}, .mainClear = {.color = spk::Color(0, 0, 0, 0), .depth = 1.0f, .stencil = 0}});
	ASSERT_EQ(plan.size(), 1u);
	EXPECT_EQ(plan.passes()[0].commandCount(spk::RenderPhase::PassSetup), 2u);
	EXPECT_EQ(plan.passes()[0].commandCount(spk::RenderPhase::SceneOpaque), 1u);
	EXPECT_EQ(plan.passes()[0].commandCount(spk::RenderPhase::SceneTransparent), 1u);
}
