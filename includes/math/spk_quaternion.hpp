#pragma once

#include <cmath>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "math/spk_vector3.hpp"

namespace spk
{
	class Quaternion
	{
	public:
		float x = 0.0f;
		float y = 0.0f;
		float z = 0.0f;
		float w = 1.0f;

		constexpr Quaternion() = default;
		constexpr Quaternion(float p_x, float p_y, float p_z, float p_w) :
			x(p_x),
			y(p_y),
			z(p_z),
			w(p_w)
		{
		}

		[[nodiscard]] static constexpr Quaternion identity()
		{
			return {};
		}

		friend std::ostream& operator<<(std::ostream& p_outputStream, const Quaternion& p_value)
		{
			p_outputStream << '(' << p_value.x << ", " << p_value.y << ", " << p_value.z << ", " << p_value.w << ')';
			return p_outputStream;
		}

		[[nodiscard]] std::string toString() const
		{
			std::ostringstream outputStream;
			outputStream << *this;
			return outputStream.str();
		}

		[[nodiscard]] Quaternion normalized() const
		{
			const float length = std::sqrt(x * x + y * y + z * z + w * w);
			if (length == 0.0f)
			{
				throw std::runtime_error("spk::Quaternion: cannot normalize a zero-length quaternion");
			}
			return {x / length, y / length, z / length, w / length};
		}
	};
}
