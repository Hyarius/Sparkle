#include <gtest/gtest.h>

#include <stdexcept>

#include "structures/widget/spk_image_label.hpp"
#include "structures/widget/spk_widget_style.hpp"

namespace
{
	[[nodiscard]] std::shared_ptr<spk::Texture> makeTexture()
	{
		return spk::WidgetStyle::makeDefault().nineSliceSpriteSheet();
	}
}

TEST(ImageLabelTest, NameOnlyConstructorActivatesWithoutTexture)
{
	spk::ImageLabel label("Image");
	label.setGeometry(spk::Rect2D(0, 0, 64, 64));

	auto renderUnit = label.renderUnit();

	ASSERT_NE(renderUnit, nullptr);
	EXPECT_EQ(renderUnit->size(), 0u);
	EXPECT_TRUE(label.isActivated());
	EXPECT_EQ(label.texture(), nullptr);
}

TEST(ImageLabelTest, BuildsImageRenderCommandWhenTextured)
{
	auto texture = makeTexture();
	spk::ImageLabel label("Image", texture);
	label.setGeometry(spk::Rect2D(0, 0, 64, 64));

	auto renderUnit = label.renderUnit();

	ASSERT_NE(renderUnit, nullptr);
	EXPECT_EQ(renderUnit->size(), 1u);
	EXPECT_EQ(label.texture(), texture);
}

TEST(ImageLabelTest, SetTextureRejectsNull)
{
	spk::ImageLabel label("Image");

	EXPECT_THROW(label.setTexture(nullptr), std::invalid_argument);
}

TEST(ImageLabelTest, DefaultSectionCoversWholeTexture)
{
	spk::ImageLabel label("Image");

	EXPECT_EQ(label.section().anchor, spk::Vector2(0.0f, 0.0f));
	EXPECT_EQ(label.section().size, spk::Vector2(1.0f, 1.0f));
}

TEST(ImageLabelTest, SetSectionUpdatesValue)
{
	spk::ImageLabel label("Image");
	const spk::Texture::Section section({0.25f, 0.25f}, {0.5f, 0.5f});

	label.setSection(section);

	EXPECT_EQ(label.section(), section);
}

TEST(ImageLabelTest, SetDepthUpdatesValue)
{
	spk::ImageLabel label("Image");

	label.setDepth(3.0f);

	EXPECT_FLOAT_EQ(label.depth(), 3.0f);
}
