#include <gtest/gtest.h>

#ifdef _WIN32

#include <atomic>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <Windows.h>

#include "spk_winapi_class.hpp"
#include "spk_winapi_cursor.hpp"
#include "spk_winapi_window.hpp"

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
	spk::WinAPI::Cursor cursor;

	EXPECT_FALSE(cursor.isValid());
	EXPECT_EQ(cursor.handle(), nullptr);
	EXPECT_THROW(cursor.activate(), std::runtime_error);
	EXPECT_THROW(spk::WinAPI::Cursor(nullptr), std::invalid_argument);
}

TEST(WinAPICursorTest, StandardCursorsAreValidAndMovable)
{
	spk::WinAPI::Cursor arrow = spk::WinAPI::Cursor::arrow();
	ASSERT_TRUE(arrow.isValid());
	EXPECT_NE(arrow.handle(), nullptr);
	EXPECT_NO_THROW(arrow.activate());

	spk::WinAPI::Cursor moved(std::move(arrow));
	EXPECT_FALSE(arrow.isValid());
	EXPECT_TRUE(moved.isValid());

	spk::WinAPI::Cursor assigned;
	assigned = std::move(moved);
	EXPECT_FALSE(moved.isValid());
	EXPECT_TRUE(assigned.isValid());

	EXPECT_TRUE(spk::WinAPI::Cursor::ibeam().isValid());
	EXPECT_TRUE(spk::WinAPI::Cursor::hand().isValid());
	EXPECT_TRUE(spk::WinAPI::Cursor::cross().isValid());
	EXPECT_TRUE(spk::WinAPI::Cursor::wait().isValid());
}

TEST(WinAPICursorTest, OwnedCursorIsDestroyedOnReplacementAndDestruction)
{
	HCURSOR firstHandle = createOwnedTestCursor();
	HCURSOR secondHandle = createOwnedTestCursor();
	ASSERT_NE(firstHandle, nullptr);
	ASSERT_NE(secondHandle, nullptr);

	spk::WinAPI::Cursor cursor(firstHandle, true);
	ASSERT_TRUE(cursor.isValid());
	EXPECT_EQ(cursor.handle(), firstHandle);

	cursor = spk::WinAPI::Cursor(secondHandle, true);
	EXPECT_TRUE(cursor.isValid());
	EXPECT_EQ(cursor.handle(), secondHandle);
}

TEST(WinAPICursorTest, SelfMoveAssignmentPreservesCursor)
{
	spk::WinAPI::Cursor cursor = spk::WinAPI::Cursor::arrow();
	const HCURSOR originalHandle = cursor.handle();

	cursor = std::move(cursor);

	EXPECT_TRUE(cursor.isValid());
	EXPECT_EQ(cursor.handle(), originalHandle);
}

TEST(WinAPICursorTest, ActivateOnWindowSetsWindowCursorAndActivatesIt)
{
	auto windowClass = std::make_shared<spk::WinAPI::Class>(uniqueClassName("Sparkle.Test.Cursor.Window"), &testWindowProcedure);
	spk::WinAPI::Window window(windowClass, spk::Rect2D(20, 20, 64, 64), "CursorActivation");
	spk::WinAPI::Cursor cursor = spk::WinAPI::Cursor::hand();

	ASSERT_TRUE(window.isValid());
	ASSERT_TRUE(cursor.isValid());

	EXPECT_NO_THROW(cursor.activate(window));
	EXPECT_EQ(reinterpret_cast<HCURSOR>(GetClassLongPtrW(window.handle(), GCLP_HCURSOR)), cursor.handle());

	window.destroy();
}

#endif
