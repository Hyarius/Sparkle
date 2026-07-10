#include "structures/graphics/opengl/opengl_wrapper_test_utils.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <memory>
#include <ostream>
#include <string>

#include "utils/image_comparison_test_utils.hpp"
#include "utils/test_resource_path_utils.hpp"

namespace
{
	// Atlas cells mirror the historical Playground voxel definitions so the scene reads
	// naturally with tests/TUs/resources/textures/voxels.png (8x8 atlas).
	struct SceneRegistry
	{
		spk::VoxelRegistry registry;
		std::int32_t grass = 0;
		std::int32_t stone = 0;
		std::int32_t wall = 0;
		std::int32_t slope = 0;
		std::int32_t stair = 0;
		std::int32_t slab = 0;
		std::int32_t bush = 0;

		SceneRegistry()
		{
			grass = registry.registerShape(std::make_unique<spk::CubeVoxelShape>(spk::VoxelShape::TextureSlots{{"top", {0, 0}}, {"bottom", {2, 0}}, {"posX", {1, 0}}, {"negX", {1, 0}}, {"posZ", {1, 0}}, {"negZ", {1, 0}}}));
			stone = registry.registerShape(std::make_unique<spk::CubeVoxelShape>(spk::AtlasCell{3, 0}));
			wall = registry.registerShape(std::make_unique<spk::CubeVoxelShape>(spk::VoxelShape::TextureSlots{{"top", {4, 0}}, {"bottom", {3, 0}}, {"posX", {5, 0}}, {"negX", {5, 0}}, {"posZ", {6, 0}}, {"negZ", {6, 0}}}));
			slope = registry.registerShape(std::make_unique<spk::SlopeVoxelShape>(spk::VoxelShape::TextureSlots{{"slope", {0, 0}}, {"back", {1, 0}}, {"bottom", {2, 0}}, {"sideLeft", {0, 1}}, {"sideRight", {1, 1}}}));
			stair = registry.registerShape(std::make_unique<spk::StairVoxelShape>(spk::VoxelShape::TextureSlots{{"top", {3, 0}}, {"riser", {4, 1}}, {"back", {3, 0}}, {"bottom", {3, 0}}, {"sideLeft", {5, 1}}, {"sideRight", {5, 1}}}, 2));
			slab = registry.registerShape(std::make_unique<spk::SlabVoxelShape>(spk::VoxelShape::TextureSlots{{"top", {3, 0}}, {"bottom", {3, 0}}, {"posX", {4, 1}}, {"negX", {4, 1}}, {"posZ", {4, 1}}, {"negZ", {4, 1}}}, 0.5f));
			bush = registry.registerShape(std::make_unique<spk::DiagonalCrossVoxelShape>(spk::AtlasCell{0, 2}));
		}
	};

	// Predetermined showcase of every shape shipped with the library: a solid grass
	// ground, a stone wall, a stair flight, one slope per orientation (plus a flipped
	// one), a few slabs, a stone platform and some bushes.
	void buildScene(const SceneRegistry &p_scene, spk::VoxelChunk &p_chunk)
	{
		for (int z = 0; z < spk::VoxelChunk::Size.z; ++z)
		{
			for (int x = 0; x < spk::VoxelChunk::Size.x; ++x)
			{
				p_chunk.setCell({x, 0, z}, {p_scene.grass});
			}
		}

		for (int z = 4; z <= 11; ++z)
		{
			for (int y = 1; y <= 3; ++y)
			{
				p_chunk.setCell({3, y, z}, {p_scene.wall});
			}
		}

		for (int x = 6; x <= 7; ++x)
		{
			for (int z = 2; z <= 3; ++z)
			{
				p_chunk.setCell({x, 1, z}, {p_scene.stone});
			}
		}

		p_chunk.setCell({8, 1, 6}, {p_scene.stair});
		p_chunk.setCell({9, 1, 6}, {p_scene.stair});
		p_chunk.setCell({11, 1, 6}, {p_scene.stair, spk::VoxelOrientation::PositiveX});

		p_chunk.setCell({6, 1, 12}, {p_scene.slope});
		p_chunk.setCell({7, 1, 12}, {p_scene.slope, spk::VoxelOrientation::PositiveX});
		p_chunk.setCell({8, 1, 12}, {p_scene.slope, spk::VoxelOrientation::NegativeZ});
		p_chunk.setCell({9, 1, 12}, {p_scene.slope, spk::VoxelOrientation::NegativeX});
		p_chunk.setCell({11, 1, 12}, {p_scene.slope, spk::VoxelOrientation::PositiveZ, spk::VoxelFlip::NegativeY});

		p_chunk.setCell({12, 1, 4}, {p_scene.slab});
		p_chunk.setCell({13, 1, 4}, {p_scene.slab});
		p_chunk.setCell({12, 1, 5}, {p_scene.slab});

		p_chunk.setCell({2, 1, 13}, {p_scene.bush});
		p_chunk.setCell({13, 1, 2}, {p_scene.bush});
		p_chunk.setCell({5, 1, 9}, {p_scene.bush});
	}

	struct CameraView
	{
		std::string name;
		spk::Vector3 position;
		spk::Vector3 target;
		float fieldOfView = 60.0f;
	};

	void PrintTo(const CameraView &p_view, std::ostream *p_stream)
	{
		*p_stream << p_view.name;
	}
}

class VoxelSceneVisualTest : public ::testing::TestWithParam<CameraView>
{
};

TEST_P(VoxelSceneVisualTest, SceneMatchesTheExpectedImage)
{
	const spk::Rect2D captureRect(0, 0, 2000, 2000);
	sparkle_test::OpenGLTestContext context(captureRect);
	const CameraView &view = GetParam();

	const std::filesystem::path texturePath = spk::test::testResourcesRoot() / "textures" / "voxels.png";
	ASSERT_TRUE(std::filesystem::exists(texturePath)) << "Missing voxel texture: " << texturePath.string();
	spk::Image voxelTexture(texturePath);
	voxelTexture.setProperties(
		spk::Texture::Filtering::Linear,
		spk::Texture::Wrap::ClampToEdge,
		spk::Texture::Mipmap::Disable);

	SceneRegistry scene;
	spk::GameEngine engine;
	engine.add<spk::VoxelChunkRenderLogic>(voxelTexture);
	engine.add<spk::VoxelChunkTransparentRenderLogic>(voxelTexture);
	spk::VoxelMap map(
		scene.registry,
		[&scene](spk::VoxelChunk &p_chunk) {
			buildScene(scene, p_chunk);
		},
		&engine);
	(void)map.chunk({0, 0, 0});

	spk::Camera3D camera;
	camera.makeMain();
	camera.setPosition(view.position);
	camera.setTarget(view.target);
	camera.setPerspective(view.fieldOfView, 0.1f, 1000.0f);
	camera.setAspectRatio(1.0f);

	// Update pass: bakes the chunk mesh through the render logic.
	engine.logicRegistry().update(spk::UpdateContext{}, engine.componentRegistry());

	const std::string group = "VoxelSceneVisual";
	const std::string &variant = view.name;
	const std::filesystem::path expectedPath = spk::test::expectedImagePath(group, variant);
	const std::filesystem::path resultDir = spk::test::resultDirectory() / group;
	const std::filesystem::path actualPath = resultDir / (variant + "_actual.png");
	const std::filesystem::path diffPath = resultDir / (variant + "_diff.png");

	spk::RenderContext &renderContext = context.renderContext();
	{
		sparkle_test::OffscreenRenderTarget target(
			static_cast<GLsizei>(captureRect.width()),
			static_cast<GLsizei>(captureRect.height()));
		ASSERT_TRUE(target.isComplete()) << "Offscreen capture framebuffer is incomplete";

		glViewport(0, 0, static_cast<GLsizei>(captureRect.width()), static_cast<GLsizei>(captureRect.height()));
		glScissor(0, 0, static_cast<GLsizei>(captureRect.width()), static_cast<GLsizei>(captureRect.height()));
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClearDepth(1.0);
		glClearStencil(0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		spk::RenderUnitBuilder builder;
		engine.logicRegistry().render(builder, engine.componentRegistry());
		spk::RenderUnit renderUnit = builder.build();
		renderUnit.execute(renderContext);
		context.gpuRuntime().waitUntilWorkDone();

		std::filesystem::create_directories(resultDir);
		context.gpuRuntime().saveScreenshot(actualPath, captureRect.atOrigin());
	}

	if (std::filesystem::exists(expectedPath) == false)
	{
		ADD_FAILURE() << "No expected image for " << group << " (" << variant << ")."
					  << " Visually validate the actual image, then copy it to approve.\n"
					  << "  actual:   " << actualPath.string() << "\n"
					  << "  expected: " << expectedPath.string();
		return;
	}

	const sparkle_test::ImageComparisonResult result = sparkle_test::compareImages(actualPath, expectedPath, diffPath);
	EXPECT_TRUE(result.matches) << "Visual mismatch for " << group << " (" << variant << ")"
								<< ". Diff: " << diffPath.string()
								<< ", actual: " << actualPath.string();
}

INSTANTIATE_TEST_SUITE_P(
	CameraViews,
	VoxelSceneVisualTest,
	::testing::Values(
		// Whole-scene coverage.
		CameraView{"topDown", {8.0f, 18.0f, -5.0f}, {8.0f, 0.5f, 8.0f}, 60.0f},
		CameraView{"southSide", {8.0f, 8.0f, -10.0f}, {8.0f, 1.0f, 8.0f}, 55.0f},
		CameraView{"northSide", {8.0f, 8.0f, 26.0f}, {8.0f, 1.0f, 8.0f}, 55.0f},
		CameraView{"westSide", {-10.0f, 8.0f, 8.0f}, {8.0f, 1.0f, 8.0f}, 55.0f},
		CameraView{"eastSide", {26.0f, 8.0f, 8.0f}, {8.0f, 1.0f, 8.0f}, 55.0f},

		// Close views exercise the individual shape silhouettes and texture slots.
		CameraView{"wallCloseUp", {8.0f, 4.5f, 7.5f}, {3.0f, 2.0f, 7.5f}, 50.0f},
		CameraView{"stairCloseUp", {9.0f, 3.5f, 2.0f}, {9.0f, 1.3f, 6.0f}, 45.0f},
		CameraView{"slopeCloseUp", {8.5f, 3.5f, 7.5f}, {8.5f, 1.3f, 12.0f}, 45.0f},
		CameraView{"slabCloseUp", {17.0f, 3.5f, 1.0f}, {12.5f, 1.2f, 4.5f}, 40.0f},
		CameraView{"bushCloseUp", {5.0f, 3.2f, 4.5f}, {5.0f, 1.5f, 9.0f}, 40.0f}),
	[](const ::testing::TestParamInfo<CameraView> &p_info) {
		return p_info.param.name;
	});
