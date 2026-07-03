#include <gtest/gtest.h>

#include <memory>

#include "structures/game_engine/spk_camera_3d.hpp"
#include "structures/game_engine/spk_entity_3d.hpp"
#include "structures/game_engine/spk_texture_mesh_render_logic.hpp"
#include "structures/game_engine/spk_texture_mesh_renderer_3d.hpp"
#include "structures/graphics/geometry/spk_primitive_object.hpp"
#include "structures/graphics/rendering/unit/spk_render_unit_builder.hpp"

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
	TestTextureMeshRenderLogic logic(false);
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

	TestTextureMeshRenderLogic logic(false);
	logic._onRenderStarted(1);
	logic._parseComponentForRender(renderer);
	spk::RenderUnitBuilder builder;
	logic._executeRender(builder);

	EXPECT_EQ(builder.size(), 1u);
	EXPECT_EQ(spk::TextureMeshRenderLogic::lastMeshCount(), 1u);
	EXPECT_EQ(spk::TextureMeshRenderLogic::lastTriangleCount(), 12u);
}

TEST(TextureMeshRenderLogic, EmitsCameraAndDirectionalLightWhenFrameStateIsEnabled)
{
	spk::Camera3D camera;
	camera.makeMain();
	spk::Entity3D entity;
	spk::Texture texture;
	auto mesh = std::make_shared<spk::TextureMesh3D>(spk::PrimitiveObject::CreateCube());
	spk::TextureMeshRenderer3D &renderer = entity.addComponent<spk::TextureMeshRenderer3D>();
	renderer.setMesh(mesh);
	renderer.setTexture(&texture);

	TestTextureMeshRenderLogic logic(true);
	logic._onRenderStarted(1);
	logic._parseComponentForRender(renderer);
	spk::RenderUnitBuilder builder;
	logic._executeRender(builder);

	EXPECT_EQ(builder.size(), 3u);
}
