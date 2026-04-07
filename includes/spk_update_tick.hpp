#pragma once

#include "spk_timestamp.hpp"
#include "spk_duration.hpp"

#include "spk_mouse.hpp"
#include "spk_keyboard.hpp"

struct UpdateTick
{
	spk::Timestamp timestamp;
	spk::Duration deltaTime;

	spk::Mouse* mouse = nullptr;
	spk::Keyboard* keyboard = nullptr;
};