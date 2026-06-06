#include <gtest/gtest.h>
#include <stdexcept>

#include "rendering/spk_viewport.hpp"
#include "window/spk_surface_state.hpp"

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

TEST(ViewportTest, ConvertLayerToOpenGLScalesByMaxLayer)
{
	const float savedMaxLayer = spk::Viewport::maxLayer();

	spk::Viewport::setMaxLayer(200.0f);
	EXPECT_FLOAT_EQ(spk::Viewport::convertLayerToOpenGL(100.0f), 0.5f);
	EXPECT_FLOAT_EQ(spk::Viewport::convertLayerToOpenGL(200.0f), 1.0f);
	EXPECT_FLOAT_EQ(spk::Viewport::convertLayerToOpenGL(0.0f), 0.0f);

	spk::Viewport::setMaxLayer(savedMaxLayer);
}

TEST(ViewportTest, ActivateThrowsWhenGeometryWidthIsZero)
{
	ConcreteViewport viewport(spk::Rect2D(0, 0, 0, 50), spk::Rect2D(0, 0, 10, 10));
	spk::SurfaceState surfaceState;

	EXPECT_THROW(viewport.activate(surfaceState), std::runtime_error);
}

TEST(ViewportTest, ActivateThrowsWhenGeometryHeightIsZero)
{
	ConcreteViewport viewport(spk::Rect2D(0, 0, 50, 0), spk::Rect2D(0, 0, 10, 10));
	spk::SurfaceState surfaceState;

	EXPECT_THROW(viewport.activate(surfaceState), std::runtime_error);
}

TEST(ViewportTest, ActivateThrowsWhenScissorWidthIsZero)
{
	ConcreteViewport viewport(spk::Rect2D(0, 0, 50, 50), spk::Rect2D(0, 0, 0, 10));
	spk::SurfaceState surfaceState;

	EXPECT_THROW(viewport.activate(surfaceState), std::runtime_error);
}

TEST(ViewportTest, ActivateThrowsWhenScissorHeightIsZero)
{
	ConcreteViewport viewport(spk::Rect2D(0, 0, 50, 50), spk::Rect2D(0, 0, 10, 0));
	spk::SurfaceState surfaceState;

	EXPECT_THROW(viewport.activate(surfaceState), std::runtime_error);
}

TEST(ViewportTest, ActivateSetsActiveViewport)
{
	ConcreteViewport viewport(spk::Rect2D(0, 0, 100, 100));
	spk::SurfaceState surfaceState;
	surfaceState.setRect(spk::Rect2D(0, 0, 100, 100));

	viewport.activate(surfaceState);

	EXPECT_EQ(spk::Viewport::activeViewport(), &viewport);
}
