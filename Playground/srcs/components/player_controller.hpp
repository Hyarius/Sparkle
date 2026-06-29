#pragma once

#include "component2d.hpp"

namespace pg
{
	class PlayerController : public Component2D
	{
	private:
		float _speed = 4.0f;

	public:
		PlayerController() = default;

		explicit PlayerController(float p_speed) :
			_speed(p_speed)
		{
		}

		[[nodiscard]] float speed() const
		{
			return _speed;
		}

		void setSpeed(float p_speed)
		{
			_speed = p_speed;
		}
	};
}
