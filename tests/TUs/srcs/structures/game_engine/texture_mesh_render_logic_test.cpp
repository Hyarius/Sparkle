#include <gtest/gtest.h>

#include <cstddef>
#include <memory>

#include "structures/game_engine/rendering/spk_scene_render_passes.hpp"
#include "structures/game_engine/spk_camera_3d.hpp"
#include "structures/game_engine/spk_entity_3d.hpp"
#include "structures/game_engine/spk_game_engine.hpp"
#include "structures/game_engine/spk_texture_mesh_render_logic.hpp"
#include "structures/game_engine/spk_texture_mesh_renderer_3d.hpp"
#include "structures/graphics/geometry/spk_primitive_object.hpp"
#include "structures/graphics/rendering/command/spk_draw_texture_mesh_3d_render_command.hpp"
#include "structures/graphics/rendering/pass/spk_render_pass.hpp"
#include "structures/graphics/spk_framebuffer_object.hpp"

namespace
{
	spk::SceneRenderFrameRequest requestFor(spk::FrameBufferObject &p_target)
	{
		return {.mainTarget = {.frameBuffer = &p_target, .viewport = p_target.viewport()},
			.mainClear = {.color = spk::Color(0, 0, 0, 0), .depth = 1.0f, .stencil = 0}};
	}

	// The pipeline seeds every main pass with camera and lighting setup commands, so only the
	// mesh draws reflect what TextureMeshRenderLogic contributed.
	std::size_t meshDrawCount(spk::RenderPass &p_pass)
	{
		std::size_t result = 0;
		for (const auto &command : p_pass.takeCommands())
		{
			if (dynamic_cast<const spk::DrawTextureMesh3DRenderCommand *>(command.get()) != nullptr)
			{
				++result;
			}
		}
		return result;
	}
}

TEST(TextureMeshRenderLogic, SkipsRenderersWithoutAnActiveCamera)
{
	spk::GameEngine engine;
	engine.add<spk::TextureMeshRenderLogic>();
	spk::Entity3D entity;
	entity.addComponent<spk::TextureMeshRenderer3D>();
	engine.addEntity(&entity);
	spk::FrameBufferObject target(spk::FrameBufferObject::colorTarget({16, 16}));
	spk::RenderPlan plan = engine.buildRenderPlan(requestFor(target));
	ASSERT_EQ(plan.size(), 3u);
	EXPECT_EQ(meshDrawCount(*plan.passes()[0]), 0u);
	EXPECT_EQ(spk::TextureMeshRenderLogic::lastMeshCount(), 0u);
}

TEST(TextureMeshRenderLogic, OneCollectionSeparatesOpaqueAndTranslucentQueues)
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
	spk::RenderPlan plan = engine.buildRenderPlan(requestFor(target));
	ASSERT_EQ(plan.size(), 3u);
	EXPECT_EQ(plan.passes()[0]->key().type, spk::SceneRenderPasses::MainOpaque);
	EXPECT_EQ(plan.passes()[1]->key().type, spk::SceneRenderPasses::MainTransparent);
	EXPECT_EQ(meshDrawCount(*plan.passes()[0]), 1u);
	EXPECT_EQ(meshDrawCount(*plan.passes()[1]), 1u);
	EXPECT_EQ(spk::TextureMeshRenderLogic::lastMeshCount(), 2u);
	EXPECT_EQ(spk::TextureMeshRenderLogic::lastTriangleCount(), 24u);
}
