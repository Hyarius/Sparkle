#pragma once

#include "time/spk_timestamp.hpp"
#include "time/spk_duration.hpp"

#include "input/spk_mouse.hpp"
#include "input/spk_keyboard.hpp"

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