#include <gtest/gtest.h>

#ifdef _WIN32

#include <atomic>
#include <memory>
#include <stdexcept>
#include <string>

#include <Windows.h>

#include "winapi/spk_winapi_class.hpp"
#include "winapi/spk_winapi_cursor.hpp"
#include "winapi/spk_winapi_window.hpp"

namespace
{
	std::atomic_uint g_uniqueId = 0;

	[[nodiscard]] std::string uniqueClassName(const std::string& p_prefix)
	{
		return p_prefix + "." + std::to_string(GetCurrentProcessId()) + "." + std::to_string(++g_uniqueId);
	}

	LRESULT CALLBACK testWindowProcedure(HWND p_handle, UINT p_message, WPARAM p_wParam, LPARAM p_lParam)
	{
		return DefWindowProcW(p_handle, p_message, p_wParam, p_lParam);
	}
}

TEST(WinAPIWindowTest, ConstructorRejectsMissingClass)
{
	EXPECT_THROW(
		spk::WinAPI::Window(nullptr, spk::Rect2D(10, 20, 100, 80), "MissingClass"),
		std::invalid_argument);
}

TEST(WinAPIWindowTest, WindowTracksTitleGeometryMoveAndDestroyedState)
{
	auto windowClass = std::make_shared<spk::WinAPI::Class>(uniqueClassName("Sparkle.Test.Window"), &testWindowProcedure);
	spk::WinAPI::Window window(
		windowClass,
		spk::Rect2D(40, 50, 160, 120),
		"Initial");

	ASSERT_TRUE(window.isValid());
	EXPECT_NE(window.handle(), nullptr);
	EXPECT_EQ(window.title(), "Initial");
	EXPECT_EQ(window.clientRect().width(), 160u);
	EXPECT_EQ(window.clientRect().height(), 120u);

	HDC deviceContext = window.deviceContext();
	ASSERT_NE(deviceContext, nullptr);
	EXPECT_EQ(WindowFromDC(deviceContext), window.handle());
	ReleaseDC(window.handle(), deviceContext);

	const spk::Rect2D windowRect = window.rect();
	EXPECT_GT(windowRect.width(), 0u);
	EXPECT_GT(windowRect.height(), 0u);

	window.setTitle("Updated");
	EXPECT_EQ(window.title(), "Updated");

	window.resize(spk::Rect2D(80, 90, 200, 140));
	EXPECT_EQ(window.clientRect().width(), 200u);
	EXPECT_EQ(window.clientRect().height(), 140u);

	const HWND originalHandle = window.handle();
	spk::WinAPI::Window movedWindow(std::move(window));
	EXPECT_EQ(window.handle(), nullptr);
	EXPECT_EQ(movedWindow.handle(), originalHandle);

	movedWindow.hide();
	EXPECT_FALSE(IsWindowVisible(movedWindow.handle()));
	EXPECT_NO_THROW(movedWindow.setCursor(spk::WinAPI::Cursor{}));
	EXPECT_NO_THROW(movedWindow.setCursor(spk::WinAPI::Cursor::arrow()));

	movedWindow.destroy();
	EXPECT_FALSE(movedWindow.isValid());
	EXPECT_EQ(movedWindow.handle(), nullptr);
	EXPECT_EQ(movedWindow.title(), "");
	EXPECT_EQ(movedWindow.rect(), spk::Rect2D());
	EXPECT_EQ(movedWindow.clientRect(), spk::Rect2D());
	EXPECT_EQ(movedWindow.deviceContext(), nullptr);

	EXPECT_NO_THROW(movedWindow.hide());
	EXPECT_NO_THROW(movedWindow.show());
	EXPECT_NO_THROW(movedWindow.resize(spk::Rect2D(0, 0, 1, 1)));
	EXPECT_NO_THROW(movedWindow.setTitle("Ignored"));
	EXPECT_NO_THROW(movedWindow.destroy());
}

TEST(WinAPIWindowTest, MoveAssignmentTransfersNativeHandleAndSelfMoveIsNoOp)
{
	auto windowClass = std::make_shared<spk::WinAPI::Class>(uniqueClassName("Sparkle.Test.Window.MoveAssignment"), &testWindowProcedure);
	spk::WinAPI::Window sourceWindow(windowClass, spk::Rect2D(30, 30, 80, 60), "Source");
	spk::WinAPI::Window assignedWindow(windowClass, spk::Rect2D(40, 40, 90, 70), "Assigned");

	const HWND sourceHandle = sourceWindow.handle();
	ASSERT_NE(sourceHandle, nullptr);
	ASSERT_TRUE(sourceWindow.isValid());
	ASSERT_TRUE(assignedWindow.isValid());

	assignedWindow = std::move(sourceWindow);

	EXPECT_EQ(sourceWindow.handle(), nullptr);
	EXPECT_EQ(assignedWindow.handle(), sourceHandle);
	EXPECT_TRUE(assignedWindow.isValid());
	EXPECT_EQ(assignedWindow.title(), "Source");

	assignedWindow = std::move(assignedWindow);

	EXPECT_EQ(assignedWindow.handle(), sourceHandle);
	EXPECT_TRUE(assignedWindow.isValid());

	assignedWindow.destroy();
}

#endif
