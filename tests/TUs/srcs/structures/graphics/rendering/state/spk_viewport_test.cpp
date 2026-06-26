#include <gtest/gtest.h>
#include <stdexcept>

#include "structures/graphics/rendering/state/spk_viewport.hpp"

namespace
{
	using ConcreteViewport = spk::Viewport;
}

TEST(ViewportTest, TwoArgConstructorSetsGeometryAndScissorIndependently)
{
	const spk::Rect2D geometry(10, 20, 100, 80);
	const spk::Rect2D scissor(15, 25, 60, 50);

	ConcreteViewport viewport(geometry, scissor);

	EXPECT_EQ(viewport.geometry(), geometry);
	EXPECT_EQ(viewport.scissor(), scissor);
}

TEST(ViewportTest, OneArgConstructorSetsScissorEqualToGeometry)
{
	const spk::Rect2D geometry(5, 5, 200, 150);

	ConcreteViewport viewport(geometry);

	EXPECT_EQ(viewport.geometry(), geometry);
	EXPECT_EQ(viewport.scissor(), geometry);
}

TEST(ViewportTest, SetMaxLayerAndMaxLayerRoundTrip)
{
	const float savedMaxLayer = spk::Viewport::maxLayer();

	spk::Viewport::setMaxLayer(500.0f);
	EXPECT_FLOAT_EQ(spk::Viewport::maxLayer(), 500.0f);

	spk::Viewport::setMaxLayer(savedMaxLayer);
}

TEST(ViewportTest, ConvertLayerToOpenGLMapsLayerRangeToNDCDepth)
{
	const float savedMaxLayer = spk::Viewport::maxLayer();

	spk::Viewport::setMaxLayer(200.0f);
	EXPECT_FLOAT_EQ(spk::Viewport::convertLayerToOpenGL(0.0f), 1.0f);
	EXPECT_FLOAT_EQ(spk::Viewport::convertLayerToOpenGL(100.0f), 0.0f);
	EXPECT_FLOAT_EQ(spk::Viewport::convertLayerToOpenGL(200.0f), -1.0f);

	spk::Viewport::setMaxLayer(savedMaxLayer);
}

TEST(ViewportTest, ActivateThrowsWhenGeometryWidthIsZero)
{
	ConcreteViewport viewport(spk::Rect2D(0, 0, 0, 50), spk::Rect2D(0, 0, 10, 10));

	EXPECT_THROW(viewport.activate(), std::runtime_error);
}

TEST(ViewportTest, ActivateThrowsWhenGeometryHeightIsZero)
{
	ConcreteViewport viewport(spk::Rect2D(0, 0, 50, 0), spk::Rect2D(0, 0, 10, 10));

	EXPECT_THROW(viewport.activate(), std::runtime_error);
}

TEST(ViewportTest, ActivateThrowsWhenScissorWidthIsZero)
{
	ConcreteViewport viewport(spk::Rect2D(0, 0, 50, 50), spk::Rect2D(0, 0, 0, 10));

	EXPECT_THROW(viewport.activate(), std::runtime_error);
}

TEST(ViewportTest, ActivateThrowsWhenScissorHeightIsZero)
{
	ConcreteViewport viewport(spk::Rect2D(0, 0, 50, 50), spk::Rect2D(0, 0, 10, 0));

	EXPECT_THROW(viewport.activate(), std::runtime_error);
}

TEST(ViewportTest, ActivateSetsActiveViewport)
{
	ConcreteViewport viewport(spk::Rect2D(0, 0, 100, 100));

	viewport.activate();

	EXPECT_EQ(spk::Viewport::activeViewport(), &viewport);
}
