#include <gtest/gtest.h>

#include "structures/graphics/geometry/spk_color_mesh_2d.hpp"
#include "structures/graphics/geometry/spk_mesh_2d.hpp"
#include "structures/graphics/geometry/spk_texture_mesh_2d.hpp"

TEST(Vertex2DEqualityTest, ComparesPositionThenUv)
{
	const spk::Vertex2D base{spk::Vector2(1.0f, 2.0f), spk::Vector2(0.25f, 0.5f)};

	EXPECT_TRUE((base == spk::Vertex2D{spk::Vector2(1.0f, 2.0f), spk::Vector2(0.25f, 0.5f)}));
	EXPECT_FALSE((base == spk::Vertex2D{spk::Vector2(9.0f, 2.0f), spk::Vector2(0.25f, 0.5f)}));
	EXPECT_FALSE((base == spk::Vertex2D{spk::Vector2(1.0f, 2.0f), spk::Vector2(0.9f, 0.5f)}));
}

TEST(TextureVertex2DEqualityTest, ComparesPositionThenUv)
{
	const spk::TextureVertex2D base{spk::Vector3(1.0f, 2.0f, 3.0f), spk::Vector2(0.25f, 0.5f)};

	EXPECT_TRUE((base == spk::TextureVertex2D{spk::Vector3(1.0f, 2.0f, 3.0f), spk::Vector2(0.25f, 0.5f)}));
	EXPECT_FALSE((base == spk::TextureVertex2D{spk::Vector3(9.0f, 2.0f, 3.0f), spk::Vector2(0.25f, 0.5f)}));
	EXPECT_FALSE((base == spk::TextureVertex2D{spk::Vector3(1.0f, 2.0f, 3.0f), spk::Vector2(0.9f, 0.5f)}));
}

TEST(ColorVertex2DEqualityTest, ComparesPositionThenEachColorChannel)
{
	const spk::ColorVertex2D base{spk::Vector3(1.0f, 2.0f, 3.0f), spk::Color(0.1f, 0.2f, 0.3f, 0.4f)};

	EXPECT_TRUE((base == spk::ColorVertex2D{spk::Vector3(1.0f, 2.0f, 3.0f), spk::Color(0.1f, 0.2f, 0.3f, 0.4f)}));
	EXPECT_FALSE((base == spk::ColorVertex2D{spk::Vector3(9.0f, 2.0f, 3.0f), spk::Color(0.1f, 0.2f, 0.3f, 0.4f)}));
	EXPECT_FALSE((base == spk::ColorVertex2D{spk::Vector3(1.0f, 2.0f, 3.0f), spk::Color(0.9f, 0.2f, 0.3f, 0.4f)}));
	EXPECT_FALSE((base == spk::ColorVertex2D{spk::Vector3(1.0f, 2.0f, 3.0f), spk::Color(0.1f, 0.9f, 0.3f, 0.4f)}));
	EXPECT_FALSE((base == spk::ColorVertex2D{spk::Vector3(1.0f, 2.0f, 3.0f), spk::Color(0.1f, 0.2f, 0.9f, 0.4f)}));
	EXPECT_FALSE((base == spk::ColorVertex2D{spk::Vector3(1.0f, 2.0f, 3.0f), spk::Color(0.1f, 0.2f, 0.3f, 0.9f)}));
}
