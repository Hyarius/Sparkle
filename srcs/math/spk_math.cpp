#include "math/spk_math.hpp"

namespace spk
{
	namespace
	{
		constexpr float Pi = 3.14159265358979323846f;
	}

	float degreeToRadian(float p_degrees)
	{
		return p_degrees * Pi / 180.0f;
	}

	float radianToDegree(float p_radians)
	{
		return p_radians * 180.0f / Pi;
	}
}
