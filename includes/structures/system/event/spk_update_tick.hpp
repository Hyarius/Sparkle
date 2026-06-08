#pragma once

#include "structures/system/time/spk_timestamp.hpp"
#include "structures/system/time/spk_duration.hpp"

#include "structures/system/device/input/spk_mouse.hpp"
#include "structures/system/device/input/spk_keyboard.hpp"

namespace spk
{
	struct UpdateTick
	{
		spk::Timestamp timestamp;
		spk::Duration deltaTime;

		spk::Mouse *mouse = nullptr;
		spk::Keyboard *keyboard = nullptr;
	};
}