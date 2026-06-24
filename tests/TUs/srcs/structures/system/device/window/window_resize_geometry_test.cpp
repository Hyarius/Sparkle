#include <gtest/gtest.h>

#include "structures/system/device/window/window_test_utils.hpp"
#include "structures/widget/spk_layout.hpp"
#include "structures/widget/spk_linear_layout.hpp"

namespace
{
	class InsetLayoutWidget : public spk::Widget
	{
	private:
		spk::Widget* _content = nullptr;
		spk::Vector2Int _inset;

	protected:
		void _onWindowResizedEvent(spk::WindowResizedEvent& p_event) override
		{
			setGeometry(p_event->rect.atOrigin());
		}

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

namespace
{
	// Mirrors the UIShowcase MainWidget: fills the window and lays out a Minimum-height
	// header above an Extend body, both via a padded vertical layout in _onGeometryChange.
	class VLayoutHost : public spk::Widget
	{
	private:
		spk::VerticalLayout _layout;

		void _onGeometryChange() override
		{
			_layout.setGeometry(geometry().atOrigin().shrink(spk::Vector2Int(5, 5)));
		}

	public:
		spk::Widget header;
		spk::Widget body;

		VLayoutHost(const std::string &p_name, spk::Widget *p_parent) :
			spk::Widget(p_name, p_parent),
			header(p_name + "/Header", this),
			body(p_name + "/Body", this)
		{
			header.setMinimalSize({0, 40});
			header.activate();
			body.activate();

			_layout.setElementPadding({5, 5});
			_layout.addWidget(&header, spk::Layout::SizePolicy::Minimum);
			_layout.addWidget(&body, spk::Layout::SizePolicy::Extend);
		}
	};
}

TEST(WindowResizeGeometryTest, ResizedWindowReflowsNestedLayoutHost)
{
	auto bundle = sparkle_test::createWindowBundle(spk::Rect2D(0, 0, 1280, 800), "ReflowResize");

	spk::Widget &rootWidget = sparkle_test::WindowAccess::rootWidget(*bundle.window);

	VLayoutHost host("Host", &rootWidget);
	host.setGeometry(rootWidget.geometry());
	host.activate();

	// Inner area after shrink(5,5) is 1270x790; header takes its 40px minimum, body the rest.
	EXPECT_EQ(host.geometry(), spk::Rect2D(0, 0, 1280, 800));
	EXPECT_EQ(host.header.geometry(), spk::Rect2D(5, 5, 1270, 40));
	EXPECT_EQ(host.body.geometry(), spk::Rect2D(5, 50, 1270, 745));

	bundle.platformRuntime->queueFrameEvent(spk::WindowResizedRecord{
		.rect = spk::Rect2D(0, 0, 686, 900)});
	bundle.platformRuntime->pollEvents();
	bundle.window->update();

	// Inner area 676x890; header 40px, body fills the rest. Nothing should exceed 686x900.
	EXPECT_EQ(host.geometry(), spk::Rect2D(0, 0, 686, 900));
	EXPECT_EQ(host.header.geometry(), spk::Rect2D(5, 5, 676, 40));
	EXPECT_EQ(host.body.geometry(), spk::Rect2D(5, 50, 676, 845));
}

TEST(WindowResizeGeometryTest, ResizedWindowProportionallyRescalesPlainChildWithoutHandlers)
{
	auto bundle = sparkle_test::createWindowBundle(spk::Rect2D(0, 0, 400, 400), "ProportionalResize");

	spk::Widget &rootWidget = sparkle_test::WindowAccess::rootWidget(*bundle.window);

	// A plain widget: no _onWindowResizedEvent, no layout, no _onGeometryChange override.
	spk::Widget child("Child", &rootWidget);
	child.setGeometry(spk::Rect2D(100, 100, 200, 200));
	child.activate();

	bundle.platformRuntime->queueFrameEvent(spk::WindowResizedRecord{
		.rect = spk::Rect2D(0, 0, 800, 800)});

	bundle.platformRuntime->pollEvents();
	bundle.window->update();

	EXPECT_EQ(rootWidget.geometry(), spk::Rect2D(0, 0, 800, 800));
	// Window doubled in both axes, so the child's anchor (1/4) and size (1/2) ratios double.
	EXPECT_EQ(child.geometry(), spk::Rect2D(200, 200, 400, 400));
	EXPECT_EQ(child.absoluteGeometry(), spk::Rect2D(200, 200, 400, 400));
}
