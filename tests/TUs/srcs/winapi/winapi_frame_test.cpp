#include <gtest/gtest.h>


#include <memory>
#include <vector>
#include <variant>

#include <Windows.h>
#include <windowsx.h>

#include "input/spk_events.hpp"
#include "input/spk_keyboard.hpp"
#include "winapi/spk_winapi_frame.hpp"
#include "winapi/spk_winapi_platform_runtime.hpp"

namespace
{
	[[nodiscard]] LPARAM mouseLParam(int p_x, int p_y)
	{
		return MAKELPARAM(static_cast<SHORT>(p_x), static_cast<SHORT>(p_y));
	}

	template <typename TRecord>
	[[nodiscard]] const TRecord& onlyRecord(const std::vector<spk::FrameEventRecord>& p_events)
	{
		EXPECT_EQ(p_events.size(), 1u);
		return std::get<TRecord>(p_events.front());
	}

	template <typename TRecord>
	[[nodiscard]] const TRecord& onlyRecord(const std::vector<spk::MouseEventRecord>& p_events)
	{
		EXPECT_EQ(p_events.size(), 1u);
		return std::get<TRecord>(p_events.front());
	}

	template <typename TRecord>
	[[nodiscard]] const TRecord& onlyRecord(const std::vector<spk::KeyboardEventRecord>& p_events)
	{
		EXPECT_EQ(p_events.size(), 1u);
		return std::get<TRecord>(p_events.front());
	}

	void pumpWinApiMessages()
	{
		MSG message{};
		while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE) == TRUE)
		{
			TranslateMessage(&message);
			DispatchMessageW(&message);
		}
	}
}

TEST(WinAPIFrameTest, TranslatesFrameMessages)
{
	spk::PlatformRuntime runtime;
	std::unique_ptr<spk::IFrame> baseFrame = runtime.createFrame(spk::Rect2D(100, 100, 128, 96), "FrameMessages");
	auto& frame = dynamic_cast<spk::Frame&>(*baseFrame);
	std::vector<spk::FrameEventRecord> events;
	auto contract = frame.subscribeToFrameEvents([&](const spk::FrameEventRecord& p_event)
	{
		events.push_back(p_event);
	});

	SendMessageW(frame.handle(), WM_CLOSE, 0, 0);
	static_cast<void>(onlyRecord<spk::WindowCloseRequestedRecord>(events));
	events.clear();

	SendMessageW(frame.handle(), WM_SETFOCUS, 0, 0);
	EXPECT_TRUE(spk::holds<spk::WindowFocusGainedRecord>(events.front()));
	events.clear();

	SendMessageW(frame.handle(), WM_KILLFOCUS, 0, 0);
	EXPECT_TRUE(spk::holds<spk::WindowFocusLostRecord>(events.front()));
	events.clear();

	SendMessageW(frame.handle(), WM_SHOWWINDOW, TRUE, 0);
	EXPECT_TRUE(spk::holds<spk::WindowShownRecord>(events.front()));
	events.clear();

	SendMessageW(frame.handle(), WM_SHOWWINDOW, FALSE, 0);
	EXPECT_TRUE(spk::holds<spk::WindowHiddenRecord>(events.front()));
	events.clear();

	frame.resize(spk::Rect2D(120, 130, 222, 111));
	events.clear();
	SendMessageW(frame.handle(), WM_MOVE, 0, 0);
	EXPECT_EQ(onlyRecord<spk::WindowMovedRecord>(events).position, frame.rect().anchor);
	events.clear();

	SendMessageW(frame.handle(), WM_SIZE, 0, 0);
	EXPECT_EQ(onlyRecord<spk::WindowResizedRecord>(events).rect, frame.rect());

	frame.validateClosure();
	pumpWinApiMessages();
}

TEST(WinAPIFrameTest, TranslatesMouseMessagesAndSignedCoordinates)
{
	spk::PlatformRuntime runtime;
	std::unique_ptr<spk::IFrame> baseFrame = runtime.createFrame(spk::Rect2D(150, 150, 128, 96), "MouseMessages");
	auto& frame = dynamic_cast<spk::Frame&>(*baseFrame);
	std::vector<spk::MouseEventRecord> events;
	auto contract = frame.subscribeToMouseEvents([&](const spk::MouseEventRecord& p_event)
	{
		events.push_back(p_event);
	});

	SendMessageW(frame.handle(), WM_MOUSEMOVE, 0, mouseLParam(-3, 7));
	ASSERT_GE(events.size(), 1u);
	const auto& moved = std::get<spk::MouseMovedRecord>(events.back());
	EXPECT_EQ(moved.position, spk::Vector2Int(-3, 7));
	EXPECT_EQ(moved.delta, spk::Vector2Int(-3, 7));
	events.clear();

	SendMessageW(frame.handle(), WM_MOUSEMOVE, 0, mouseLParam(5, -2));
	const auto& secondMove = std::get<spk::MouseMovedRecord>(events.back());
	EXPECT_EQ(secondMove.position, spk::Vector2Int(5, -2));
	EXPECT_EQ(secondMove.delta, spk::Vector2Int(8, -9));
	events.clear();

	SendMessageW(frame.handle(), WM_MOUSELEAVE, 0, 0);
	EXPECT_EQ(onlyRecord<spk::MouseLeftRecord>(events).position, spk::Vector2Int(5, -2));
	events.clear();

	SendMessageW(frame.handle(), WM_MOUSEWHEEL, MAKEWPARAM(0, WHEEL_DELTA * -2), 0);
	EXPECT_EQ(onlyRecord<spk::MouseWheelScrolledRecord>(events).delta, spk::Vector2(0.0f, -2.0f));
	events.clear();

	SendMessageW(frame.handle(), WM_RBUTTONDOWN, 0, 0);
	EXPECT_EQ(onlyRecord<spk::MouseButtonPressedRecord>(events).button, spk::Mouse::Right);
	events.clear();

	SendMessageW(frame.handle(), WM_MBUTTONDOWN, 0, 0);
	EXPECT_EQ(onlyRecord<spk::MouseButtonPressedRecord>(events).button, spk::Mouse::Middle);
	events.clear();

	SendMessageW(frame.handle(), WM_LBUTTONDOWN, 0, 0);
	EXPECT_EQ(onlyRecord<spk::MouseButtonPressedRecord>(events).button, spk::Mouse::Left);
	events.clear();

	SendMessageW(frame.handle(), WM_RBUTTONUP, 0, 0);
	EXPECT_EQ(onlyRecord<spk::MouseButtonReleasedRecord>(events).button, spk::Mouse::Right);
	events.clear();

	SendMessageW(frame.handle(), WM_MBUTTONUP, 0, 0);
	EXPECT_EQ(onlyRecord<spk::MouseButtonReleasedRecord>(events).button, spk::Mouse::Middle);
	events.clear();

	SendMessageW(frame.handle(), WM_LBUTTONUP, 0, 0);
	EXPECT_EQ(onlyRecord<spk::MouseButtonReleasedRecord>(events).button, spk::Mouse::Left);
	events.clear();

	SendMessageW(frame.handle(), WM_LBUTTONDBLCLK, 0, 0);
	EXPECT_EQ(onlyRecord<spk::MouseButtonDoubleClickedRecord>(events).button, spk::Mouse::Left);

	frame.validateClosure();
	pumpWinApiMessages();
}

TEST(WinAPIFrameTest, TranslatesKeyboardMessagesIncludingRepeatAndUnknownKeys)
{
	spk::PlatformRuntime runtime;
	std::unique_ptr<spk::IFrame> baseFrame = runtime.createFrame(spk::Rect2D(200, 200, 128, 96), "KeyboardMessages");
	auto& frame = dynamic_cast<spk::Frame&>(*baseFrame);
	std::vector<spk::KeyboardEventRecord> events;
	auto contract = frame.subscribeToKeyboardEvents([&](const spk::KeyboardEventRecord& p_event)
	{
		events.push_back(p_event);
	});

	SendMessageW(frame.handle(), WM_KEYDOWN, static_cast<WPARAM>(spk::Keyboard::A), 0);
	const auto& pressed = onlyRecord<spk::KeyPressedRecord>(events);
	EXPECT_EQ(pressed.key, spk::Keyboard::A);
	EXPECT_FALSE(pressed.isRepeated);
	events.clear();

	SendMessageW(frame.handle(), WM_SYSKEYDOWN, static_cast<WPARAM>(spk::Keyboard::B), 1 << 30);
	const auto& repeated = onlyRecord<spk::KeyPressedRecord>(events);
	EXPECT_EQ(repeated.key, spk::Keyboard::B);
	EXPECT_TRUE(repeated.isRepeated);
	events.clear();

	SendMessageW(frame.handle(), WM_KEYUP, static_cast<WPARAM>(spk::Keyboard::C), 0);
	EXPECT_EQ(onlyRecord<spk::KeyReleasedRecord>(events).key, spk::Keyboard::C);
	events.clear();

	SendMessageW(frame.handle(), WM_KEYDOWN, static_cast<WPARAM>(spk::Keyboard::NbKey) + 100, 0);
	EXPECT_EQ(onlyRecord<spk::KeyPressedRecord>(events).key, spk::Keyboard::Unknown);
	events.clear();

	SendMessageW(frame.handle(), WM_CHAR, static_cast<WPARAM>('Z'), 0);
	EXPECT_EQ(onlyRecord<spk::TextInputRecord>(events).glyph, U'Z');

	frame.validateClosure();
	pumpWinApiMessages();
}

TEST(WinAPIFrameTest, TitleAndDeviceContextAccessorsReflectNativeWindow)
{
	spk::PlatformRuntime runtime;
	std::unique_ptr<spk::IFrame> baseFrame = runtime.createFrame(spk::Rect2D(220, 220, 128, 96), "InitialFrameTitle");
	auto& frame = dynamic_cast<spk::Frame&>(*baseFrame);

	EXPECT_EQ(frame.title(), "InitialFrameTitle");

	frame.setTitle("UpdatedFrameTitle");
	EXPECT_EQ(frame.title(), "UpdatedFrameTitle");

	HDC deviceContext = frame.deviceContext();
	ASSERT_NE(deviceContext, nullptr);
	EXPECT_EQ(WindowFromDC(deviceContext), frame.handle());
	ReleaseDC(frame.handle(), deviceContext);

	frame.validateClosure();
	pumpWinApiMessages();
}

