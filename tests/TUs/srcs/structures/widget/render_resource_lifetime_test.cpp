#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <vector>

#include <GL/glew.h>

#include "spk_generated_resources.hpp"
#include "structures/graphics/opengl/opengl_wrapper_test_utils.hpp"
#include "structures/graphics/rendering/command/render_command_test_utils.hpp"
#include "structures/graphics/rendering/snapshot/spk_render_snapshot_builder.hpp"
#include "structures/widget/spk_animation_label.hpp"
#include "structures/widget/spk_image_label.hpp"
#include "structures/widget/spk_panel.hpp"
#include "structures/widget/spk_text_label.hpp"

namespace
{
	[[nodiscard]] std::shared_ptr<spk::Font> makeFont()
	{
		return std::make_shared<spk::Font>(sparkle_test::testFont());
	}

	[[nodiscard]] std::shared_ptr<spk::SpriteSheet> makeTwoSpriteSheet()
	{
		auto sheet = std::make_shared<spk::SpriteSheet>();
		sheet->loadFromData(sparkle_test::makeTwoSpritePngBytes(), {2, 1});
		return sheet;
	}

	[[nodiscard]] std::shared_ptr<spk::SpriteSheet> makeNineSliceSpriteSheet()
	{
		const auto& bytes = SPARKLE_GET_RESOURCE("resources/textures/default_nine_slice.png");
		return std::make_shared<spk::SpriteSheet>(
			spk::SpriteSheet::fromRawData(
				std::vector<std::uint8_t>(bytes.begin(), bytes.end()),
				{3, 3},
				spk::Texture::Filtering::Nearest,
				spk::Texture::Wrap::ClampToEdge,
				spk::Texture::Mipmap::Disable));
	}

	[[nodiscard]] spk::RenderSnapshot buildSnapshot(const spk::Widget& p_widget)
	{
		spk::RenderSnapshotBuilder builder;
		p_widget.appendRenderUnits(builder);
		return builder.build();
	}

	void clearRenderTarget(int p_width, int p_height)
	{
		glViewport(0, 0, p_width, p_height);
		glScissor(0, 0, p_width, p_height);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClearDepth(1.0);
		glClearStencil(0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	}
}

TEST(RenderResourceLifetimeTest, OldImageLabelSnapshotKeepsTextureAliveAfterReplacement)
{
	constexpr int width = 32;
	constexpr int height = 32;
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, width, height));
	sparkle_test::OffscreenRenderTarget target(width, height);
	ASSERT_TRUE(target.isComplete());

	std::shared_ptr<spk::Texture> texture = sparkle_test::makeSolidTexture({1, 1}, 0, 0, 255);
	spk::ImageLabel label("Image", texture);
	label.setGeometry(spk::Rect2D(0, 0, width, height));

	spk::RenderSnapshot oldSnapshot = buildSnapshot(label);
	ASSERT_FALSE(oldSnapshot.empty());

	label.setTexture(sparkle_test::makeSolidTexture({1, 1}, 255, 0, 0));
	texture.reset();
	spk::RenderSnapshot currentSnapshot = buildSnapshot(label);
	ASSERT_FALSE(currentSnapshot.empty());

	clearRenderTarget(width, height);
	EXPECT_NO_THROW(oldSnapshot.execute(context.renderContext()));
	context.gpuRuntime().waitUntilWorkDone();
}

TEST(RenderResourceLifetimeTest, OldTextLabelSnapshotKeepsFontAliveAfterReplacement)
{
	constexpr int width = 96;
	constexpr int height = 48;
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, width, height));
	sparkle_test::OffscreenRenderTarget target(width, height);
	ASSERT_TRUE(target.isComplete());

	std::shared_ptr<spk::Font> font = makeFont();
	spk::TextLabel label("Label", "Old");
	label.setFont(font);
	label.setGeometry(spk::Rect2D(0, 0, width, height));

	spk::RenderSnapshot oldSnapshot = buildSnapshot(label);
	ASSERT_FALSE(oldSnapshot.empty());

	label.setFont(makeFont());
	font.reset();
	spk::RenderSnapshot currentSnapshot = buildSnapshot(label);
	ASSERT_FALSE(currentSnapshot.empty());

	clearRenderTarget(width, height);
	EXPECT_NO_THROW(oldSnapshot.execute(context.renderContext()));
	context.gpuRuntime().waitUntilWorkDone();
}

TEST(RenderResourceLifetimeTest, OldAnimationLabelSnapshotKeepsSpriteSheetAliveAfterReplacement)
{
	constexpr int width = 32;
	constexpr int height = 32;
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, width, height));
	sparkle_test::OffscreenRenderTarget target(width, height);
	ASSERT_TRUE(target.isComplete());

	std::shared_ptr<spk::SpriteSheet> spriteSheet = makeTwoSpriteSheet();
	spk::AnimationLabel label("Animation", spriteSheet);
	label.setGeometry(spk::Rect2D(0, 0, width, height));

	spk::RenderSnapshot oldSnapshot = buildSnapshot(label);
	ASSERT_FALSE(oldSnapshot.empty());

	label.setSpriteSheet(makeTwoSpriteSheet());
	spriteSheet.reset();
	spk::RenderSnapshot currentSnapshot = buildSnapshot(label);
	ASSERT_FALSE(currentSnapshot.empty());

	clearRenderTarget(width, height);
	EXPECT_NO_THROW(oldSnapshot.execute(context.renderContext()));
	context.gpuRuntime().waitUntilWorkDone();
}

TEST(RenderResourceLifetimeTest, OldPanelSnapshotKeepsNineSliceSpriteSheetAliveAfterReplacement)
{
	constexpr int width = 64;
	constexpr int height = 48;
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, width, height));
	sparkle_test::OffscreenRenderTarget target(width, height);
	ASSERT_TRUE(target.isComplete());

	std::shared_ptr<spk::SpriteSheet> spriteSheet = makeNineSliceSpriteSheet();
	spk::Panel panel("Panel", spriteSheet);
	panel.setGeometry(spk::Rect2D(0, 0, width, height));

	spk::RenderSnapshot oldSnapshot = buildSnapshot(panel);
	ASSERT_FALSE(oldSnapshot.empty());

	panel.setSpriteSheet(makeNineSliceSpriteSheet());
	spriteSheet.reset();
	spk::RenderSnapshot currentSnapshot = buildSnapshot(panel);
	ASSERT_FALSE(currentSnapshot.empty());

	clearRenderTarget(width, height);
	EXPECT_NO_THROW(oldSnapshot.execute(context.renderContext()));
	context.gpuRuntime().waitUntilWorkDone();
}
