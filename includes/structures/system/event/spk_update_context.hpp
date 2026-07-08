#pragma once

#include "structures/system/time/spk_duration.hpp"
#include "structures/system/time/spk_timestamp.hpp"

#include "structures/system/device/input/spk_keyboard.hpp"
#include "structures/system/device/input/spk_mouse.hpp"

namespace spk
{
	class Profiler;

	// Per-tick context handed down the widget tree (and into every ComponentLogic)
	// during the update pass. Carries the frame timing plus the shared input devices
	// and a non-owning handle to the application-wide profiler, so any update-side
	// code can record timings without reaching for a global.
	struct UpdateContext
	{
		spk::Timestamp timestamp;
		spk::Duration deltaTime;

		spk::Mouse *mouse = nullptr;
		spk::Keyboard *keyboard = nullptr;

		spk::Profiler *profiler = nullptr;
	};
}