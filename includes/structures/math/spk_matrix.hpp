#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

#include "structures/math/spk_approx_value.hpp"
#include "structures/math/spk_quaternion.hpp"
#include "structures/math/spk_vector2.hpp"
#include "structures/math/spk_vector3.hpp"
#include "structures/math/spk_vector4.hpp"
#include "type/spk_constants.hpp"
#include "utils/spk_math.hpp"

namespace spk
{
	template <std::size_t SizeX, std::size_t SizeY>
	class IMatrix
	{
	public:
		class Column
		{
		private:
			std::array<float, SizeY> _rows{};

		public:
			float &operator[](std::size_t p_index)
			{
				if (p_index >= SizeY)
				{
					throw std::invalid_argument("spk::IMatrix: row index out of bounds");
				}
				return _rows[p_index];
			}

			const float &operator[](std::size_t p_index) const
			{
				if (p_index >= SizeY)
				{
					throw std::invalid_argument("spk::IMatrix: row index out of bounds");
				}
				return _rows[p_index];
			}
		};

	private:
		std::array<Column, SizeX> _columns{};

	public:
		IMatrix()
		{
			for (std::size_t x = 0; x < SizeX; ++x)
			{
				for (std::size_t y = 0; y < SizeY; ++y)
				{
					(*this)[x][y] = (x == y ? 1.0f : 0.0f);
				}
			}
		}

		explicit IMatrix(const float *p_values)
		{
			for (std::size_t y = 0; y < SizeY; ++y)
			{
				for (std::size_t x = 0; x < SizeX; ++x)
				{
					(*this)[x][y] = p_values[y * SizeX + x];
				}
			}
		}

		IMatrix(std::initializer_list<float> p_values)
		{
			if (p_values.size() != SizeX * SizeY)
			{
				throw std::invalid_argument("spk::IMatrix: initializer list size does not match matrix dimensions");
			}

			auto iterator = p_values.begin();
			for (std::size_t y = 0; y < SizeY; ++y)
			{
				for (std::size_t x = 0; x < SizeX; ++x)
				{
					(*this)[x][y] = *iterator++;
				}
			}
		}

		[[nodiscard]] static IMatrix identity()
		{
			return {};
		}

		Column &operator[](std::size_t p_index)
		{
			if (p_index >= SizeX)
			{
				throw std::invalid_argument("spk::IMatrix: column index out of bounds");
			}
			return _columns[p_index];
		}

		const Column &operator[](std::size_t p_index) const
		{
			if (p_index >= SizeX)
			{
				throw std::invalid_argument("spk::IMatrix: column index out of bounds");
			}
			return _columns[p_index];
		}

		template <std::size_t X = SizeX, std::size_t Y = SizeY>
			requires(X == 3 && Y == 3)
		[[nodiscard]] spk::Vector2 operator*(const spk::Vector2 &p_vector) const
		{
			return {
				(*this)[0][0] * p_vector.x + (*this)[1][0] * p_vector.y + (*this)[2][0],
				(*this)[0][1] * p_vector.x + (*this)[1][1] * p_vector.y + (*this)[2][1]};
		}

		template <std::size_t X = SizeX, std::size_t Y = SizeY>
			requires(X == 3 && Y == 3)
		[[nodiscard]] spk::Vector3 operator*(const spk::Vector3 &p_vector) const
		{
			return {
				(*this)[0][0] * p_vector.x + (*this)[1][0] * p_vector.y + (*this)[2][0] * p_vector.z,
				(*this)[0][1] * p_vector.x + (*this)[1][1] * p_vector.y + (*this)[2][1] * p_vector.z,
				(*this)[0][2] * p_vector.x + (*this)[1][2] * p_vector.y + (*this)[2][2] * p_vector.z};
		}

		template <std::size_t X = SizeX, std::size_t Y = SizeY>
			requires(X == 4 && Y == 4)
		[[nodiscard]] spk::Vector4 operator*(const spk::Vector4 &p_vector) const
		{
			return {
				(*this)[0][0] * p_vector.x + (*this)[1][0] * p_vector.y + (*this)[2][0] * p_vector.z + (*this)[3][0] * p_vector.w,
				(*this)[0][1] * p_vector.x + (*this)[1][1] * p_vector.y + (*this)[2][1] * p_vector.z + (*this)[3][1] * p_vector.w,
				(*this)[0][2] * p_vector.x + (*this)[1][2] * p_vector.y + (*this)[2][2] * p_vector.z + (*this)[3][2] * p_vector.w,
				(*this)[0][3] * p_vector.x + (*this)[1][3] * p_vector.y + (*this)[2][3] * p_vector.z + (*this)[3][3] * p_vector.w};
		}

		template <std::size_t X = SizeX, std::size_t Y = SizeY>
			requires(X == 4 && Y == 4)
		[[nodiscard]] spk::Vector3 operator*(const spk::Vector3 &p_vector) const
		{
			return ((*this) * spk::Vector4(p_vector, 1.0f)).dehomogenized();
		}

		[[nodiscard]] IMatrix operator*(const IMatrix &p_other) const
		{
			IMatrix result;
			for (std::size_t x = 0; x < SizeX; ++x)
			{
				for (std::size_t y = 0; y < SizeY; ++y)
				{
					result[x][y] = 0.0f;
					for (std::size_t k = 0; k < SizeX; ++k)
					{
						result[x][y] += (*this)[k][y] * p_other[x][k];
					}
				}
			}
			return result;
		}

		[[nodiscard]] bool operator==(const IMatrix &p_other) const
		{
			for (std::size_t x = 0; x < SizeX; ++x)
			{
				for (std::size_t y = 0; y < SizeY; ++y)
				{
					if (spk::ApproxValue((*this)[x][y]) != p_other[x][y])
					{
						return false;
					}
				}
			}
			return true;
		}

		[[nodiscard]] bool operator!=(const IMatrix &p_other) const
		{
			return !(*this == p_other);
		}

		template <std::size_t X = SizeX, std::size_t Y = SizeY>
			requires(X == 4 && Y == 4)
		[[nodiscard]] static IMatrix rotation(float p_angleX, float p_angleY, float p_angleZ)
		{
			const float cosX = std::cos(spk::degreeToRadian(p_angleX));
			const float sinX = std::sin(spk::degreeToRadian(p_angleX));
			const float cosY = std::cos(spk::degreeToRadian(p_angleY));
			const float sinY = std::sin(spk::degreeToRadian(p_angleY));
			const float cosZ = std::cos(spk::degreeToRadian(p_angleZ));
			const float sinZ = std::sin(spk::degreeToRadian(p_angleZ));

			const IMatrix rotationX = {1, 0, 0, 0, 0, cosX, -sinX, 0, 0, sinX, cosX, 0, 0, 0, 0, 1};
			const IMatrix rotationY = {cosY, 0, sinY, 0, 0, 1, 0, 0, -sinY, 0, cosY, 0, 0, 0, 0, 1};
			const IMatrix rotationZ = {cosZ, -sinZ, 0, 0, sinZ, cosZ, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
			return rotationZ * rotationY * rotationX;
		}

		template <std::size_t X = SizeX, std::size_t Y = SizeY>
			requires(X == 4 && Y == 4)
		[[nodiscard]] static IMatrix rotation(const spk::Vector3 &p_angle)
		{
			return rotation(p_angle.x, p_angle.y, p_angle.z);
		}

		template <std::size_t X = SizeX, std::size_t Y = SizeY>
			requires(X == 4 && Y == 4)
		[[nodiscard]] static IMatrix rotation(const spk::Quaternion &p_quaternion)
		{
			const spk::Quaternion q = p_quaternion.normalized();

			IMatrix result;
			result[0][0] = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);
			result[0][1] = 2.0f * (q.x * q.y + q.w * q.z);
			result[0][2] = 2.0f * (q.x * q.z - q.w * q.y);
			result[1][0] = 2.0f * (q.x * q.y - q.w * q.z);
			result[1][1] = 1.0f - 2.0f * (q.x * q.x + q.z * q.z);
			result[1][2] = 2.0f * (q.y * q.z + q.w * q.x);
			result[2][0] = 2.0f * (q.x * q.z + q.w * q.y);
			result[2][1] = 2.0f * (q.y * q.z - q.w * q.x);
			result[2][2] = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
			return result;
		}

		template <std::size_t X = SizeX, std::size_t Y = SizeY>
			requires(X == 4 && Y == 4)
		[[nodiscard]] static IMatrix translation(float p_translateX, float p_translateY, float p_translateZ)
		{
			return {1, 0, 0, p_translateX, 0, 1, 0, p_translateY, 0, 0, 1, p_translateZ, 0, 0, 0, 1};
		}

		template <std::size_t X = SizeX, std::size_t Y = SizeY>
			requires(X == 4 && Y == 4)
		[[nodiscard]] static IMatrix translation(const spk::Vector3 &p_translation)
		{
			return translation(p_translation.x, p_translation.y, p_translation.z);
		}

		template <std::size_t X = SizeX, std::size_t Y = SizeY>
			requires(X == 4 && Y == 4)
		[[nodiscard]] static IMatrix scale(float p_scaleX, float p_scaleY, float p_scaleZ)
		{
			return {p_scaleX, 0, 0, 0, 0, p_scaleY, 0, 0, 0, 0, p_scaleZ, 0, 0, 0, 0, 1};
		}

		template <std::size_t X = SizeX, std::size_t Y = SizeY>
			requires(X == 4 && Y == 4)
		[[nodiscard]] static IMatrix scale(const spk::Vector3 &p_scale)
		{
			return scale(p_scale.x, p_scale.y, p_scale.z);
		}

		template <std::size_t X = SizeX, std::size_t Y = SizeY>
			requires(X == 4 && Y == 4)
		[[nodiscard]] static IMatrix lookAt(
			const spk::Vector3 &p_from,
			const spk::Vector3 &p_to,
			const spk::Vector3 &p_up)
		{
			const spk::Vector3 forward = (p_to - p_from).normalized();

			spk::Vector3 right = forward.cross(p_up);
			if (right.isZero())
			{
				const spk::Vector3 fallbackUp =
					(std::fabs(forward.y) < 0.999f)
						? spk::Vector3(0.0f, 1.0f, 0.0f)
						: spk::Vector3(1.0f, 0.0f, 0.0f);

				right = forward.cross(fallbackUp);
			}

			right = right.normalized();
			const spk::Vector3 up = right.cross(forward);

			IMatrix result;

			result[0][0] = right.x;
			result[1][0] = right.y;
			result[2][0] = right.z;

			result[0][1] = up.x;
			result[1][1] = up.y;
			result[2][1] = up.z;

			result[0][2] = -forward.x;
			result[1][2] = -forward.y;
			result[2][2] = -forward.z;

			result[3][0] = -right.dot(p_from);
			result[3][1] = -up.dot(p_from);
			result[3][2] = forward.dot(p_from);

			return result;
		}

		template <std::size_t X = SizeX, std::size_t Y = SizeY>
			requires(X == 4 && Y == 4)
		[[nodiscard]] static IMatrix rotateAroundAxis(const spk::Vector3 &p_axis, float p_rotationAngle)
		{
			const spk::Vector3 axis = p_axis.normalized();
			const float radians = spk::degreeToRadian(p_rotationAngle);
			const float c = std::cos(radians);
			const float s = std::sin(radians);

			IMatrix result;
			result[0][0] = c + axis.x * axis.x * (1 - c);
			result[0][1] = axis.x * axis.y * (1 - c) + axis.z * s;
			result[0][2] = axis.x * axis.z * (1 - c) - axis.y * s;
			result[1][0] = axis.y * axis.x * (1 - c) - axis.z * s;
			result[1][1] = c + axis.y * axis.y * (1 - c);
			result[1][2] = axis.y * axis.z * (1 - c) + axis.x * s;
			result[2][0] = axis.z * axis.x * (1 - c) + axis.y * s;
			result[2][1] = axis.z * axis.y * (1 - c) - axis.x * s;
			result[2][2] = c + axis.z * axis.z * (1 - c);
			return result;
		}

		template <std::size_t X = SizeX, std::size_t Y = SizeY>
			requires(X == 4 && Y == 4)
		[[nodiscard]] static IMatrix perspective(float p_fov, float p_aspectRatio, float p_nearPlane, float p_farPlane)
		{
			const float radians = spk::degreeToRadian(p_fov);
			const float tanHalfFov = std::tan(radians / 2.0f);
			IMatrix result;
			result[0][0] = 1.0f / (tanHalfFov * p_aspectRatio);
			result[1][1] = 1.0f / tanHalfFov;
			result[2][2] = -(p_farPlane + p_nearPlane) / (p_farPlane - p_nearPlane);
			result[2][3] = -1.0f;
			result[3][2] = -(2.0f * p_farPlane * p_nearPlane) / (p_farPlane - p_nearPlane);
			result[3][3] = 0.0f;
			return result;
		}

		template <std::size_t X = SizeX, std::size_t Y = SizeY>
			requires(X == 4 && Y == 4)
		[[nodiscard]] static IMatrix ortho(float p_left, float p_right, float p_bottom, float p_top, float p_nearPlane, float p_farPlane)
		{
			IMatrix result;
			result[0][0] = 2.0f / (p_right - p_left);
			result[1][1] = 2.0f / (p_top - p_bottom);
			result[2][2] = -2.0f / (p_farPlane - p_nearPlane);
			result[3][0] = -(p_right + p_left) / (p_right - p_left);
			result[3][1] = -(p_top + p_bottom) / (p_top - p_bottom);
			result[3][2] = -(p_farPlane + p_nearPlane) / (p_farPlane - p_nearPlane);
			return result;
		}

		template <std::size_t X = SizeX, std::size_t Y = SizeY>
			requires(X == Y)
		[[nodiscard]] float determinant() const
		{
			constexpr std::size_t N = SizeX;
			IMatrix matrix(*this);
			float determinantValue = 1.0f;
			int sign = 1;
			for (std::size_t i = 0; i < N; ++i)
			{
				std::size_t pivotRow = i;
				float maxElement = std::fabs(matrix[i][i]);
				for (std::size_t row = i + 1; row < N; ++row)
				{
					const float value = std::fabs(matrix[i][row]);
					if (value > maxElement)
					{
						maxElement = value;
						pivotRow = row;
					}
				}
				if (maxElement < 1e-9f)
				{
					return 0.0f;
				}
				if (pivotRow != i)
				{
					for (std::size_t column = 0; column < N; ++column)
					{
						std::swap(matrix[column][i], matrix[column][pivotRow]);
					}
					sign = -sign;
				}
				for (std::size_t row = i + 1; row < N; ++row)
				{
					const float factor = matrix[i][row] / matrix[i][i];
					for (std::size_t column = i; column < N; ++column)
					{
						matrix[column][row] -= factor * matrix[column][i];
					}
				}
				determinantValue *= matrix[i][i];
			}
			return determinantValue * static_cast<float>(sign);
		}

		template <std::size_t X = SizeX, std::size_t Y = SizeY>
			requires(X == Y)
		[[nodiscard]] bool isInvertible() const
		{
			return std::fabs(determinant()) > spk::Math::Constants::pointPrecision;
		}

		template <std::size_t X = SizeX, std::size_t Y = SizeY>
			requires(X == Y)
		[[nodiscard]] IMatrix inverse() const
		{
			constexpr std::size_t N = SizeX;
			IMatrix matrix(*this);
			IMatrix result = IMatrix::identity();
			for (std::size_t i = 0; i < N; ++i)
			{
				std::size_t pivotRow = i;
				float maxElement = std::fabs(matrix[i][i]);
				for (std::size_t row = i + 1; row < N; ++row)
				{
					const float value = std::fabs(matrix[i][row]);
					if (value > maxElement)
					{
						maxElement = value;
						pivotRow = row;
					}
				}
				if (pivotRow != i)
				{
					for (std::size_t column = 0; column < N; ++column)
					{
						std::swap(matrix[column][i], matrix[column][pivotRow]);
						std::swap(result[column][i], result[column][pivotRow]);
					}
				}
				const float pivotValue = matrix[i][i];
				if (std::fabs(pivotValue) < 1e-9f)
				{
					throw std::runtime_error("spk::IMatrix: matrix is not invertible");
				}
				for (std::size_t column = 0; column < N; ++column)
				{
					matrix[column][i] /= pivotValue;
					result[column][i] /= pivotValue;
				}
				for (std::size_t row = 0; row < N; ++row)
				{
					if (row == i)
					{
						continue;
					}
					const float factor = matrix[i][row];
					for (std::size_t column = 0; column < N; ++column)
					{
						matrix[column][row] -= factor * matrix[column][i];
						result[column][row] -= factor * result[column][i];
					}
				}
			}
			return result;
		}

		friend std::ostream &operator<<(std::ostream &p_outputStream, const IMatrix &p_matrix)
		{
			for (std::size_t y = 0; y < SizeY; ++y)
			{
				if (y != 0)
				{
					p_outputStream << "\n";
				}
				p_outputStream << "[";
				for (std::size_t x = 0; x < SizeX; ++x)
				{
					if (x != 0)
					{
						p_outputStream << ", ";
					}
					p_outputStream << p_matrix[x][y];
				}
				p_outputStream << "]";
			}
			return p_outputStream;
		}

		[[nodiscard]] std::string toString() const
		{
			std::ostringstream outputStream;
			outputStream << *this;
			return outputStream.str();
		}
	};

	using Matrix2x2 = IMatrix<2, 2>;
	using Matrix3x3 = IMatrix<3, 3>;
	using Matrix4x4 = IMatrix<4, 4>;
}
