#pragma once

#include <algorithm>
#include <cmath>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "utils/spk_math.hpp"
#include "structures/math/spk_vector3.hpp"

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

		[[nodiscard]] static Quaternion fromAxisAngle(const Vector3& p_axis, const float p_angle)
		{
			const Vector3 axis = p_axis.normalized();
			const float halfAngle = degreeToRadian(p_angle) * 0.5f;
			const float sine = std::sin(halfAngle);
			return {axis.x * sine, axis.y * sine, axis.z * sine, std::cos(halfAngle)};
		}

		[[nodiscard]] static Quaternion fromEuler(const Vector3& p_angles)
		{
			const float halfX = degreeToRadian(p_angles.x) * 0.5f;
			const float halfY = degreeToRadian(p_angles.y) * 0.5f;
			const float halfZ = degreeToRadian(p_angles.z) * 0.5f;
			const float cosX = std::cos(halfX);
			const float sinX = std::sin(halfX);
			const float cosY = std::cos(halfY);
			const float sinY = std::sin(halfY);
			const float cosZ = std::cos(halfZ);
			const float sinZ = std::sin(halfZ);

			return {
				sinX * cosY * cosZ - cosX * sinY * sinZ,
				cosX * sinY * cosZ + sinX * cosY * sinZ,
				cosX * cosY * sinZ - sinX * sinY * cosZ,
				cosX * cosY * cosZ + sinX * sinY * sinZ};
		}

		[[nodiscard]] static Quaternion lookAt(
			const Vector3& p_from,
			const Vector3& p_to,
			const Vector3& p_up = Vector3(0.0f, 1.0f, 0.0f))
		{
			const Vector3 forward = (p_to - p_from).normalized();
			Vector3 right = forward.cross(p_up);
			if (right.isZero())
			{
				const Vector3 fallbackUp = (std::fabs(forward.y) < 0.999f)
					? Vector3(0.0f, 1.0f, 0.0f)
					: Vector3(1.0f, 0.0f, 0.0f);
				right = forward.cross(fallbackUp);
			}
			right = right.normalized();
			const Vector3 up = right.cross(forward);

			const float m00 = right.x;
			const float m01 = up.x;
			const float m02 = -forward.x;
			const float m10 = right.y;
			const float m11 = up.y;
			const float m12 = -forward.y;
			const float m20 = right.z;
			const float m21 = up.z;
			const float m22 = -forward.z;
			const float trace = m00 + m11 + m22;

			Quaternion result;
			if (trace > 0.0f)
			{
				const float scale = std::sqrt(trace + 1.0f) * 2.0f;
				result = {(m21 - m12) / scale, (m02 - m20) / scale, (m10 - m01) / scale, 0.25f * scale};
			}
			else if (m00 > m11 && m00 > m22)
			{
				const float scale = std::sqrt(1.0f + m00 - m11 - m22) * 2.0f;
				result = {0.25f * scale, (m01 + m10) / scale, (m02 + m20) / scale, (m21 - m12) / scale};
			}
			else if (m11 > m22)
			{
				const float scale = std::sqrt(1.0f + m11 - m00 - m22) * 2.0f;
				result = {(m01 + m10) / scale, 0.25f * scale, (m12 + m21) / scale, (m02 - m20) / scale};
			}
			else
			{
				const float scale = std::sqrt(1.0f + m22 - m00 - m11) * 2.0f;
				result = {(m02 + m20) / scale, (m12 + m21) / scale, 0.25f * scale, (m10 - m01) / scale};
			}
			return result.normalized();
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

		[[nodiscard]] float dot(const Quaternion& p_other) const noexcept
		{
			return x * p_other.x + y * p_other.y + z * p_other.z + w * p_other.w;
		}

		[[nodiscard]] Vector3 toEuler() const
		{
			const Quaternion quaternion = normalized();
			const float sinXCosY = 2.0f * (quaternion.w * quaternion.x + quaternion.y * quaternion.z);
			const float cosXCosY = 1.0f - 2.0f * (quaternion.x * quaternion.x + quaternion.y * quaternion.y);
			const float sinY = 2.0f * (quaternion.w * quaternion.y - quaternion.z * quaternion.x);
			const float sinZCosY = 2.0f * (quaternion.w * quaternion.z + quaternion.x * quaternion.y);
			const float cosZCosY = 1.0f - 2.0f * (quaternion.y * quaternion.y + quaternion.z * quaternion.z);

			return {
				radianToDegree(std::atan2(sinXCosY, cosXCosY)),
				radianToDegree(std::asin(std::clamp(sinY, -1.0f, 1.0f))),
				radianToDegree(std::atan2(sinZCosY, cosZCosY))};
		}

		[[nodiscard]] static Quaternion slerp(const Quaternion& p_from, const Quaternion& p_to, const float p_alpha)
		{
			const Quaternion from = p_from.normalized();
			Quaternion to = p_to.normalized();
			float cosine = from.dot(to);

			if (cosine < 0.0f)
			{
				to = {-to.x, -to.y, -to.z, -to.w};
				cosine = -cosine;
			}

			if (cosine > 0.9995f)
			{
				return Quaternion(
					from.x + p_alpha * (to.x - from.x),
					from.y + p_alpha * (to.y - from.y),
					from.z + p_alpha * (to.z - from.z),
					from.w + p_alpha * (to.w - from.w)).normalized();
			}

			const float angle = std::acos(std::clamp(cosine, -1.0f, 1.0f));
			const float sine = std::sin(angle);
			const float fromScale = std::sin((1.0f - p_alpha) * angle) / sine;
			const float toScale = std::sin(p_alpha * angle) / sine;
			return {
				from.x * fromScale + to.x * toScale,
				from.y * fromScale + to.y * toScale,
				from.z * fromScale + to.z * toScale,
				from.w * fromScale + to.w * toScale};
		}
	};
}
