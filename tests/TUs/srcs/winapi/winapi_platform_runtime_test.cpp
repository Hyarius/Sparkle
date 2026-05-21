#include <gtest/gtest.h>

#ifdef _WIN32

#include <memory>

#include <Windows.h>

#include "input/spk_events.hpp"
#include "winapi/spk_winapi_frame.hpp"
#include "winapi/spk_winapi_platform_runtime.hpp"

TEST(WinAPIPlatformRuntimeTest, PollEventsDispatchesPostedCloseRequest)
{
	spk::WinAPI::PlatformRuntime runtime;
	std::unique_ptr<spk::IFrame> baseFrame = runtime.createFrame(spk::Rect2D(250, 250, 128, 96), "RuntimeMessages");
	auto& frame = dynamic_cast<spk::WinAPI::Frame&>(*baseFrame);
	int closeRequestCount = 0;
	auto contract = frame.subscribeToFrameEvents([&](const spk::FrameEventRecord& p_event)
	{
		if (spk::holds<spk::WindowCloseRequestedRecord>(p_event) == true)
		{
			++closeRequestCount;
		}
	});

	frame.requestClosure();
	runtime.pollEvents();

	EXPECT_EQ(closeRequestCount, 1);

	frame.validateClosure();
	runtime.pollEvents();
}

#endif
