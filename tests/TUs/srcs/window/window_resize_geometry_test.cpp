#include <gtest/gtest.h>

#include "window_test_utils.hpp"

namespace
{
	class InsetLayoutWidget : public spk::Widget
	{
	private:
		spk::Widget* _content = nullptr;
		spk::Vector2Int _inset;

	protected:
		void _onGeometryChange() override
		{
			++geometryChangeCount;
			geometrySeenDuringChange = geometry();

			if (_content != nullptr)
			{
				_content->setGeometry(geometry().shrink(_inset));
			}
		}

	public:
		int geometryChangeCount = 0;
		spk::Rect2D geometrySeenDuringChange;

		InsetLayoutWidget(
			const std::string& p_name,
			spk::Widget* p_parent,
			spk::Widget* p_content,
			const spk::Vector2Int& p_inset) :
			spk::Widget(p_name, p_parent),
			_content(p_content),
			_inset(p_inset)
		{
		}
	};
}

TEST(WindowResizeGeometryTest, ResizedWindowUpdatesMainWidgetAndInsetChildGeometry)
{
	auto bundle = sparkle_test::createWindowBundle(spk::Rect2D(100, 100, 800, 600), "ResizeGeometry");

	spk::Widget contentWidget("Content", nullptr);
	InsetLayoutWidget mainWidget(
		"Main",
		&sparkle_test::WindowAccess::rootWidget(*bundle.window),
		&contentWidget,
		spk::Vector2Int(50, 50));
	contentWidget.setParent(&mainWidget);

	mainWidget.activate();
	contentWidget.activate();
	mainWidget.setGeometry(sparkle_test::WindowAccess::rootWidget(*bundle.window).geometry());

	EXPECT_EQ(mainWidget.geometry(), spk::Rect2D(0, 0, 800, 600));
	EXPECT_EQ(contentWidget.geometry(), spk::Rect2D(50, 50, 700, 500));
	EXPECT_EQ(mainWidget.geometryChangeCount, 1);
	EXPECT_EQ(mainWidget.geometrySeenDuringChange, spk::Rect2D(0, 0, 800, 600));

	const spk::Rect2D resizedRect(200, 150, 1800, 1280);
	bundle.platformRuntime->queueFrameEvent(spk::WindowResizedRecord{
		.rect = resizedRect});

	bundle.platformRuntime->pollEvents();
	bundle.window->update();

	EXPECT_EQ(sparkle_test::WindowAccess::rootWidget(*bundle.window).geometry(), resizedRect.atOrigin());
	EXPECT_EQ(mainWidget.geometry(), resizedRect.atOrigin());
	EXPECT_EQ(contentWidget.geometry(), resizedRect.atOrigin().shrink(spk::Vector2Int(50, 50)));
	EXPECT_EQ(mainWidget.geometryChangeCount, 2);
	EXPECT_EQ(mainWidget.geometrySeenDuringChange, resizedRect.atOrigin());
}
