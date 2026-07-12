#include <gtest/gtest.h>

#include <memory>

#include "structures/game_engine/spk_camera_3d.hpp"
#include "structures/game_engine/spk_entity_3d.hpp"
#include "structures/game_engine/spk_game_engine.hpp"
#include "structures/game_engine/spk_texture_mesh_render_logic.hpp"
#include "structures/game_engine/spk_texture_mesh_renderer_3d.hpp"
#include "structures/graphics/geometry/spk_primitive_object.hpp"
#include "structures/graphics/rendering/command/spk_camera_update_render_command.hpp"
#include "structures/graphics/rendering/command/spk_directional_light_update_render_command.hpp"
#include "structures/graphics/rendering/command/spk_draw_texture_mesh_3d_render_command.hpp"
#include "structures/graphics/rendering/command/spk_draw_voxel_mesh_3d_render_command.hpp"
#include "structures/graphics/rendering/unit/spk_render_unit_builder.hpp"
#include "structures/graphics/spk_texture.hpp"
#include "structures/voxel/spk_cube_voxel_shape.hpp"
#include "structures/voxel/spk_voxel_chunk_render_logic.hpp"
#include "structures/voxel/spk_voxel_chunk_transparent_render_logic.hpp"
#include "structures/voxel/spk_voxel_map.hpp"

TEST(VoxelChunkRenderLogic, TransparentChunksRenderAfterOpaqueActors)
{
	spk::VoxelRegistry registry;
	const spk::VoxelRuntimeId stone = registry.registerShape(
		std::make_unique<spk::CubeVoxelShape>(spk::AtlasCell{0, 0}));
	auto waterShape = std::make_unique<spk::CubeVoxelShape>(spk::AtlasCell{1, 0});
	waterShape->setTransparency(0.5f);
	const spk::VoxelRuntimeId water = registry.registerShape(std::move(waterShape));

	spk::Texture texture;
	spk::GameEngine engine;
	spk::VoxelChunkRenderLogic &opaqueChunks = engine.add<spk::VoxelChunkRenderLogic>(texture);
	spk::TextureMeshRenderLogic &actors = engine.add<spk::TextureMeshRenderLogic>();
	spk::VoxelChunkTransparentRenderLogic &transparentChunks = engine.add<spk::VoxelChunkTransparentRenderLogic>(texture);

	EXPECT_EQ(opaqueChunks.priority(), actors.priority());
	EXPECT_EQ(actors.priority(), transparentChunks.priority());
	EXPECT_TRUE(spk::containsRenderPhase(opaqueChunks.renderPhases(), spk::RenderPhase::SceneOpaque));
	EXPECT_TRUE(spk::containsRenderPhase(actors.renderPhases(), spk::RenderPhase::SceneOpaque));
	EXPECT_TRUE(spk::containsRenderPhase(actors.renderPhases(), spk::RenderPhase::SceneTransparent));
	EXPECT_TRUE(spk::containsRenderPhase(transparentChunks.renderPhases(), spk::RenderPhase::SceneTransparent));
	EXPECT_FALSE(spk::containsRenderPhase(transparentChunks.renderPhases(), spk::RenderPhase::SceneOpaque));

	spk::VoxelMap map(
		registry,
		[stone, water](spk::VoxelChunk &p_chunk) {
			p_chunk.setCell({0, 0, 0}, {stone});
			p_chunk.setCell({1, 0, 0}, {water});
		},
		&engine);
	(void)map.chunk({0, 0, 0});

	spk::Entity3D player;
	spk::TextureMeshRenderer3D &playerRenderer = player.addComponent<spk::TextureMeshRenderer3D>();
	playerRenderer.setMesh(std::make_shared<spk::TextureMesh3D>(spk::PrimitiveObject::CreateCube()));
	playerRenderer.setTexture(&texture);
	engine.addEntity(&player);

	spk::Camera3D camera;
	camera.makeMain();
	engine.logicRegistry().update(spk::UpdateContext{}, engine.componentRegistry());

	spk::RenderUnitBuilder builder;
	engine.logicRegistry().render(builder, engine.componentRegistry());
	const spk::RenderUnit renderUnit = builder.build();
	const auto &commands = renderUnit.commands();
	ASSERT_EQ(commands.size(), 5u);
	EXPECT_NE(dynamic_cast<const spk::CameraUpdateRenderCommand *>(commands[0].get()), nullptr);
	EXPECT_NE(dynamic_cast<const spk::DirectionalLightUpdateRenderCommand *>(commands[1].get()), nullptr);

	const auto *opaqueVoxel = dynamic_cast<const spk::DrawVoxelMesh3DRenderCommand *>(commands[2].get());
	ASSERT_NE(opaqueVoxel, nullptr);
	EXPECT_FALSE(opaqueVoxel->translucent());
	EXPECT_NE(dynamic_cast<const spk::DrawTextureMesh3DRenderCommand *>(commands[3].get()), nullptr);

	const auto *transparentVoxel = dynamic_cast<const spk::DrawVoxelMesh3DRenderCommand *>(commands[4].get());
	ASSERT_NE(transparentVoxel, nullptr);
	EXPECT_TRUE(transparentVoxel->translucent());
}
