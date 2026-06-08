#include <gtest/gtest.h>


#include <memory>

#include <Windows.h>

#include "structures/system/event/spk_events.hpp"
#include "structures/system/win32/spk_winapi_frame.hpp"
#include "structures/system/win32/spk_winapi_platform_runtime.hpp"

TEST(WinAPIPlatformRuntimeTest, PollEventsDispatchesPostedCloseRequest)
{
	spk::PlatformRuntime runtime;
	std::unique_ptr<spk::IFrame> baseFrame = runtime.createFrame(spk::Rect2D(250, 250, 128, 96), "RuntimeMessages");
	auto& frame = dynamic_cast<spk::Frame&>(*baseFrame);
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

