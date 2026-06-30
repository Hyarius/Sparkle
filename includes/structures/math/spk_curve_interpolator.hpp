#pragma once

#include <cstddef>
#include <initializer_list>
#include <map>
#include <span>
#include <vector>

#include "structures/math/spk_vector2.hpp"

namespace spk
{
	class CurveInterpolator
	{
	public:
		using Point = spk::Vector2;
		using Points = std::map<float, float>;

		enum class Interpolation
		{
			Linear,
			Cubic,
			MonotoneCubicHermite,
			Bezier
		};

		enum class Extrapolation
		{
			Clamp,
			Linear,
			Throw
		};

	private:
		using ConstIterator = Points::const_iterator;

	private:
		Points _points;
		Interpolation _interpolation = Interpolation::MonotoneCubicHermite;
		Extrapolation _extrapolation = Extrapolation::Linear;

	private:
		[[nodiscard]] static bool _isFinite(float p_value) noexcept;
		static void _validateX(float p_x);
		static void _validatePoint(float p_x, float p_y);

		[[nodiscard]] static int _sign(float p_value) noexcept;
		[[nodiscard]] static float _lerp(float p_from, float p_to, float p_factor) noexcept;

		[[nodiscard]] static float _secant(ConstIterator p_left, ConstIterator p_right);
		[[nodiscard]] static float _factor(float p_x, ConstIterator p_left, ConstIterator p_right);

		[[nodiscard]] float _catmullRomTangentAt(ConstIterator p_point) const;

		[[nodiscard]] float _firstMonotoneTangent() const;
		[[nodiscard]] float _lastMonotoneTangent() const;
		[[nodiscard]] float _interiorMonotoneTangent(ConstIterator p_point) const;
		[[nodiscard]] float _monotoneTangentAt(ConstIterator p_point) const;

		[[nodiscard]] std::vector<float> _naturalCubicTangents() const;
		[[nodiscard]] float _naturalCubicTangentAt(ConstIterator p_point) const;

		[[nodiscard]] float _linearInterpolate(float p_x, ConstIterator p_left, ConstIterator p_right) const;

		[[nodiscard]] float _hermiteInterpolate(
			float p_x,
			ConstIterator p_left,
			ConstIterator p_right,
			float p_leftTangent,
			float p_rightTangent
		) const;

		[[nodiscard]] float _cubicInterpolate(float p_x, ConstIterator p_left, ConstIterator p_right) const;
		[[nodiscard]] float _monotoneCubicHermiteInterpolate(float p_x, ConstIterator p_left, ConstIterator p_right) const;
		[[nodiscard]] float _bezierInterpolate(float p_x, ConstIterator p_left, ConstIterator p_right) const;

		[[nodiscard]] float _interpolate(float p_x, ConstIterator p_left, ConstIterator p_right) const;

		[[nodiscard]] float _leftLinearExtrapolationSlope() const;
		[[nodiscard]] float _rightLinearExtrapolationSlope() const;

		[[nodiscard]] float _extrapolateLeft(float p_x) const;
		[[nodiscard]] float _extrapolateRight(float p_x) const;

	public:
		void insert(float p_x, float p_y);
		void insert(const Point &p_point);
		void insert(std::span<const Point> p_points);

		void assign(std::span<const Point> p_points);
		void assign(std::initializer_list<Point> p_points);

		bool remove(float p_x);
		void clear() noexcept;

		[[nodiscard]] std::size_t nbPoints() const noexcept;
		[[nodiscard]] bool empty() const noexcept;
		[[nodiscard]] const Points &points() const noexcept;

		void setInterpolation(Interpolation p_interpolation) noexcept;
		[[nodiscard]] Interpolation interpolation() const noexcept;

		void setExtrapolation(Extrapolation p_extrapolation) noexcept;
		[[nodiscard]] Extrapolation extrapolation() const noexcept;

		[[nodiscard]] float evaluate(float p_x) const;

		[[nodiscard]] float operator()(float p_x) const;
		[[nodiscard]] float operator[](float p_x) const;
	};
}