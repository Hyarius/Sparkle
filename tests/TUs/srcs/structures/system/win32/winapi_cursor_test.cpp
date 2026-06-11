#include <gtest/gtest.h>


#include <atomic>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <Windows.h>

#include "structures/system/win32/spk_winapi_class.hpp"
#include "structures/system/win32/spk_winapi_cursor.hpp"
#include "structures/system/win32/spk_winapi_window.hpp"

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

	[[nodiscard]] HCURSOR createOwnedTestCursor()
	{
		std::vector<BYTE> andMask(1, 0xFF);
		std::vector<BYTE> xorMask(1, 0x00);
		return CreateCursor(GetModuleHandleW(nullptr), 0, 0, 1, 1, andMask.data(), xorMask.data());
	}
}

TEST(WinAPICursorTest, DefaultAndNullCursorsAreRejectedForActivation)
{
	spk::Cursor cursor;

	EXPECT_FALSE(cursor.isValid());
	EXPECT_EQ(cursor.handle(), nullptr);
	EXPECT_THROW(cursor.activate(), std::runtime_error);
	EXPECT_THROW(spk::Cursor(nullptr), std::invalid_argument);
}

TEST(WinAPICursorTest, StandardCursorsAreValidAndMovable)
{
	spk::Cursor arrow = spk::Cursor::arrow();
	ASSERT_TRUE(arrow.isValid());
	EXPECT_NE(arrow.handle(), nullptr);
	EXPECT_NO_THROW(arrow.activate());

	spk::Cursor moved(std::move(arrow));
	EXPECT_FALSE(arrow.isValid());
	EXPECT_TRUE(moved.isValid());

	spk::Cursor assigned;
	assigned = std::move(moved);
	EXPECT_FALSE(moved.isValid());
	EXPECT_TRUE(assigned.isValid());

	EXPECT_TRUE(spk::Cursor::ibeam().isValid());
	EXPECT_TRUE(spk::Cursor::hand().isValid());
	EXPECT_TRUE(spk::Cursor::cross().isValid());
	EXPECT_TRUE(spk::Cursor::wait().isValid());
}

TEST(WinAPICursorTest, OwnedCursorIsDestroyedOnReplacementAndDestruction)
{
	HCURSOR firstHandle = createOwnedTestCursor();
	HCURSOR secondHandle = createOwnedTestCursor();
	ASSERT_NE(firstHandle, nullptr);
	ASSERT_NE(secondHandle, nullptr);

	spk::Cursor cursor(firstHandle, true);
	ASSERT_TRUE(cursor.isValid());
	EXPECT_EQ(cursor.handle(), firstHandle);

	cursor = spk::Cursor(secondHandle, true);
	EXPECT_TRUE(cursor.isValid());
	EXPECT_EQ(cursor.handle(), secondHandle);
}

TEST(WinAPICursorTest, SelfMoveAssignmentPreservesCursor)
{
	spk::Cursor cursor = spk::Cursor::arrow();
	const HCURSOR originalHandle = cursor.handle();

	cursor = std::move(cursor);

	EXPECT_TRUE(cursor.isValid());
	EXPECT_EQ(cursor.handle(), originalHandle);
}

TEST(WinAPICursorTest, ActivateOnWindowSetsWindowCursorAndActivatesIt)
{
	auto windowClass = std::make_shared<spk::WindowClass>(uniqueClassName("Sparkle.Test.Cursor.Window"), &testWindowProcedure);
	spk::WindowRuntime window(windowClass, spk::Rect2D(20, 20, 64, 64), "CursorActivation");
	spk::Cursor cursor = spk::Cursor::hand();

	ASSERT_TRUE(window.isValid());
	ASSERT_TRUE(cursor.isValid());

	EXPECT_NO_THROW(cursor.activate(window));
	EXPECT_EQ(reinterpret_cast<HCURSOR>(GetClassLongPtrW(window.handle(), GCLP_HCURSOR)), cursor.handle());

	window.destroy();
}

TEST(WinAPICursorTest, ResizeCursorFactoriesAreValid)
{
	EXPECT_TRUE(spk::Cursor::resizeNS().isValid());
	EXPECT_TRUE(spk::Cursor::resizeWE().isValid());
	EXPECT_TRUE(spk::Cursor::resizeNWSE().isValid());
	EXPECT_TRUE(spk::Cursor::resizeNESW().isValid());
	EXPECT_TRUE(spk::Cursor::resizeAll().isValid());
}

TEST(WinAPICursorTest, NonOwningCursorDoesNotDestroyOnDestruction)
{
	HCURSOR handle = createOwnedTestCursor();
	ASSERT_NE(handle, nullptr);

	{
		spk::Cursor nonOwning(handle, false);
		EXPECT_TRUE(nonOwning.isValid());
		EXPECT_EQ(nonOwning.handle(), handle);
	}

	EXPECT_NE(handle, nullptr);
	DestroyCursor(handle);
}

TEST(WinAPICursorTest, RegisterAndGetCursorRoundTrip)
{
	spk::Cursor arrow = spk::Cursor::arrow();
	const std::string name = "TestCursor_" + std::to_string(GetCurrentProcessId());

	spk::Cursor::registerCursor(name, arrow);

	spk::Cursor retrieved = spk::Cursor::get(name);
	EXPECT_TRUE(retrieved.isValid());
	EXPECT_NE(retrieved.handle(), nullptr);
}

TEST(WinAPICursorTest, RegisterCursorOverwriteWorks)
{
	const std::string name = "OverwriteCursor_" + std::to_string(GetCurrentProcessId());
	spk::Cursor::registerCursor(name, spk::Cursor::arrow());
	HCURSOR firstHandle = spk::Cursor::get(name).handle();

	spk::Cursor::registerCursor(name, spk::Cursor::hand());
	HCURSOR secondHandle = spk::Cursor::get(name).handle();

	EXPECT_NE(firstHandle, nullptr);
	EXPECT_NE(secondHandle, nullptr);
}

TEST(WinAPICursorTest, GetUnknownCursorNameThrows)
{
	EXPECT_THROW(static_cast<void>(spk::Cursor::get("__nonexistent_cursor_xyz__")), std::invalid_argument);
}

