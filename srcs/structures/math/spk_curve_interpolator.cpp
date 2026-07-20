#include "structures/math/spk_curve_interpolator.hpp"

#include <cmath>
#include <iterator>
#include <stdexcept>
#include <utility>

namespace spk
{
	bool CurveInterpolator::_isFinite(float p_value) noexcept
	{
		return std::isfinite(p_value);
	}

	void CurveInterpolator::_validateX(float p_x)
	{
		if (_isFinite(p_x) == false)
		{
			throw std::invalid_argument("spk::CurveInterpolator x must be finite");
		}
	}

	void CurveInterpolator::_validatePoint(float p_x, float p_y)
	{
		if (_isFinite(p_x) == false || _isFinite(p_y) == false)
		{
			throw std::invalid_argument("spk::CurveInterpolator point must be finite");
		}
	}

	int CurveInterpolator::_sign(float p_value) noexcept
	{
		if (p_value > 0.0f)
		{
			return 1;
		}

		if (p_value < 0.0f)
		{
			return -1;
		}

		return 0;
	}

	float CurveInterpolator::_lerp(float p_from, float p_to, float p_factor) noexcept
	{
		return p_from + (p_to - p_from) * p_factor;
	}

	float CurveInterpolator::_secant(ConstIterator p_left, ConstIterator p_right)
	{
		return (p_right->second - p_left->second) / (p_right->first - p_left->first);
	}

	float CurveInterpolator::_factor(float p_x, ConstIterator p_left, ConstIterator p_right)
	{
		return (p_x - p_left->first) / (p_right->first - p_left->first);
	}

	float CurveInterpolator::_catmullRomTangentAt(ConstIterator p_point) const
	{
		if (p_point == _points.begin())
		{
			return _secant(p_point, std::next(p_point));
		}

		if (std::next(p_point) == _points.end())
		{
			return _secant(std::prev(p_point), p_point);
		}

		const ConstIterator previous = std::prev(p_point);
		const ConstIterator next = std::next(p_point);

		return (next->second - previous->second) / (next->first - previous->first);
	}

	float CurveInterpolator::_firstMonotoneTangent() const
	{
		const ConstIterator first = _points.begin();
		const ConstIterator second = std::next(first);

		if (_points.size() == 2)
		{
			return _secant(first, second);
		}

		const ConstIterator third = std::next(second);

		const float h0 = second->first - first->first;
		const float h1 = third->first - second->first;

		const float d0 = _secant(first, second);
		const float d1 = _secant(second, third);

		const float tangent = ((2.0f * h0 + h1) * d0 - h0 * d1) / (h0 + h1);

		if (_sign(tangent) != _sign(d0))
		{
			return 0.0f;
		}

		if (_sign(d0) != _sign(d1) && std::fabs(tangent) > std::fabs(3.0f * d0))
		{
			return 3.0f * d0;
		}

		return tangent;
	}

	float CurveInterpolator::_lastMonotoneTangent() const
	{
		const ConstIterator last = std::prev(_points.end());
		const ConstIterator previous = std::prev(last);

		if (_points.size() == 2)
		{
			return _secant(previous, last);
		}

		const ConstIterator beforePrevious = std::prev(previous);

		const float h0 = previous->first - beforePrevious->first;
		const float h1 = last->first - previous->first;

		const float d0 = _secant(beforePrevious, previous);
		const float d1 = _secant(previous, last);

		const float tangent = ((2.0f * h1 + h0) * d1 - h1 * d0) / (h0 + h1);

		if (_sign(tangent) != _sign(d1))
		{
			return 0.0f;
		}

		if (_sign(d0) != _sign(d1) && std::fabs(tangent) > std::fabs(3.0f * d1))
		{
			return 3.0f * d1;
		}

		return tangent;
	}

	float CurveInterpolator::_interiorMonotoneTangent(ConstIterator p_point) const
	{
		const ConstIterator previous = std::prev(p_point);
		const ConstIterator next = std::next(p_point);

		const float h0 = p_point->first - previous->first;
		const float h1 = next->first - p_point->first;

		const float d0 = _secant(previous, p_point);
		const float d1 = _secant(p_point, next);

		if (d0 == 0.0f || d1 == 0.0f || _sign(d0) != _sign(d1))
		{
			return 0.0f;
		}

		const float w0 = 2.0f * h1 + h0;
		const float w1 = h1 + 2.0f * h0;

		return (w0 + w1) / ((w0 / d0) + (w1 / d1));
	}

	float CurveInterpolator::_monotoneTangentAt(ConstIterator p_point) const
	{
		if (p_point == _points.begin())
		{
			return _firstMonotoneTangent();
		}

		if (std::next(p_point) == _points.end())
		{
			return _lastMonotoneTangent();
		}

		return _interiorMonotoneTangent(p_point);
	}

	std::vector<float> CurveInterpolator::_naturalCubicTangents() const
	{
		const std::size_t count = _points.size();

		std::vector<float> tangents(count, 0.0f);

		if (count == 2)
		{
			const float slope = _secant(_points.begin(), std::next(_points.begin()));

			tangents[0] = slope;
			tangents[1] = slope;

			return tangents;
		}

		std::vector<float> x;
		std::vector<float> y;

		x.reserve(count);
		y.reserve(count);

		for (const auto &[pointX, pointY] : _points)
		{
			x.push_back(pointX);
			y.push_back(pointY);
		}

		std::vector<float> lower(count, 0.0f);
		std::vector<float> diagonal(count, 0.0f);
		std::vector<float> upper(count, 0.0f);
		std::vector<float> rhs(count, 0.0f);

		const auto delta = [&](std::size_t p_index) -> float {
			return (y[p_index + 1] - y[p_index]) / (x[p_index + 1] - x[p_index]);
		};

		diagonal[0] = 2.0f;
		upper[0] = 1.0f;
		rhs[0] = 3.0f * delta(0);

		for (std::size_t i = 1; i + 1 < count; ++i)
		{
			const float hPrevious = x[i] - x[i - 1];
			const float hNext = x[i + 1] - x[i];

			lower[i] = hNext;
			diagonal[i] = 2.0f * (hPrevious + hNext);
			upper[i] = hPrevious;
			rhs[i] = 3.0f * (hNext * delta(i - 1) + hPrevious * delta(i));
		}

		lower[count - 1] = 1.0f;
		diagonal[count - 1] = 2.0f;
		rhs[count - 1] = 3.0f * delta(count - 2);

		for (std::size_t i = 1; i < count; ++i)
		{
			const float factor = lower[i] / diagonal[i - 1];

			diagonal[i] -= factor * upper[i - 1];
			rhs[i] -= factor * rhs[i - 1];
		}

		tangents[count - 1] = rhs[count - 1] / diagonal[count - 1];

		for (std::size_t i = count - 1; i-- > 0;)
		{
			tangents[i] = (rhs[i] - upper[i] * tangents[i + 1]) / diagonal[i];
		}

		return tangents;
	}

	float CurveInterpolator::_naturalCubicTangentAt(ConstIterator p_point) const
	{
		const std::vector<float> tangents = _naturalCubicTangents();
		const std::size_t index = static_cast<std::size_t>(std::distance(_points.begin(), p_point));

		return tangents[index];
	}

	float CurveInterpolator::_linearInterpolate(
		float p_x,
		ConstIterator p_left,
		ConstIterator p_right) const
	{
		const float t = _factor(p_x, p_left, p_right);

		return _lerp(p_left->second, p_right->second, t);
	}

	float CurveInterpolator::_hermiteInterpolate(
		float p_x,
		ConstIterator p_left,
		ConstIterator p_right,
		float p_leftTangent,
		float p_rightTangent) const
	{
		const float x0 = p_left->first;
		const float y0 = p_left->second;

		const float x1 = p_right->first;
		const float y1 = p_right->second;

		const float width = x1 - x0;
		const float t = (p_x - x0) / width;

		const float t2 = t * t;
		const float t3 = t2 * t;

		const float h00 = 2.0f * t3 - 3.0f * t2 + 1.0f;
		const float h10 = t3 - 2.0f * t2 + t;
		const float h01 = -2.0f * t3 + 3.0f * t2;
		const float h11 = t3 - t2;

		return h00 * y0 + h10 * width * p_leftTangent + h01 * y1 + h11 * width * p_rightTangent;
	}

	float CurveInterpolator::_cubicInterpolate(
		float p_x,
		ConstIterator p_left,
		ConstIterator p_right) const
	{
		return _hermiteInterpolate(
			p_x,
			p_left,
			p_right,
			_naturalCubicTangentAt(p_left),
			_naturalCubicTangentAt(p_right));
	}

	float CurveInterpolator::_monotoneCubicHermiteInterpolate(
		float p_x,
		ConstIterator p_left,
		ConstIterator p_right) const
	{
		return _hermiteInterpolate(
			p_x,
			p_left,
			p_right,
			_monotoneTangentAt(p_left),
			_monotoneTangentAt(p_right));
	}

	float CurveInterpolator::_bezierInterpolate(
		float p_x,
		ConstIterator p_left,
		ConstIterator p_right) const
	{
		const float x0 = p_left->first;
		const float y0 = p_left->second;

		const float x1 = p_right->first;
		const float y1 = p_right->second;

		const float width = x1 - x0;
		const float t = (p_x - x0) / width;
		const float u = 1.0f - t;

		const float leftTangent = _catmullRomTangentAt(p_left);
		const float rightTangent = _catmullRomTangentAt(p_right);

		const float b0 = y0;
		const float b1 = y0 + width * leftTangent / 3.0f;
		const float b2 = y1 - width * rightTangent / 3.0f;
		const float b3 = y1;

		return u * u * u * b0 +
			   3.0f * u * u * t * b1 +
			   3.0f * u * t * t * b2 +
			   t * t * t * b3;
	}

	float CurveInterpolator::_interpolate(
		float p_x,
		ConstIterator p_left,
		ConstIterator p_right) const
	{
		switch (_interpolation)
		{
		case Interpolation::Linear:
			return _linearInterpolate(p_x, p_left, p_right);

		case Interpolation::Cubic:
			return _cubicInterpolate(p_x, p_left, p_right);

		case Interpolation::Bezier:
			return _bezierInterpolate(p_x, p_left, p_right);

		default:
			return _monotoneCubicHermiteInterpolate(p_x, p_left, p_right);
		}
	}

	float CurveInterpolator::_leftLinearExtrapolationSlope() const
	{
		const ConstIterator first = _points.begin();

		switch (_interpolation)
		{
		case Interpolation::Cubic:
			return _naturalCubicTangentAt(first);

		case Interpolation::MonotoneCubicHermite:
			return _monotoneTangentAt(first);

		case Interpolation::Bezier:
			return _catmullRomTangentAt(first);

		default:
			return _secant(first, std::next(first));
		}
	}

	float CurveInterpolator::_rightLinearExtrapolationSlope() const
	{
		const ConstIterator last = std::prev(_points.end());

		switch (_interpolation)
		{
		case Interpolation::Cubic:
			return _naturalCubicTangentAt(last);

		case Interpolation::MonotoneCubicHermite:
			return _monotoneTangentAt(last);

		case Interpolation::Bezier:
			return _catmullRomTangentAt(last);

		default:
			return _secant(std::prev(last), last);
		}
	}

	float CurveInterpolator::_extrapolateLeft(float p_x) const
	{
		const ConstIterator first = _points.begin();

		switch (_extrapolation)
		{
		case Extrapolation::Linear:
			return first->second + (p_x - first->first) * _leftLinearExtrapolationSlope();

		case Extrapolation::Throw:
			throw std::out_of_range("spk::CurveInterpolator x is before first point");

		default:
			return first->second;
		}
	}

	float CurveInterpolator::_extrapolateRight(float p_x) const
	{
		const ConstIterator last = std::prev(_points.end());

		switch (_extrapolation)
		{
		case Extrapolation::Linear:
			return last->second + (p_x - last->first) * _rightLinearExtrapolationSlope();

		case Extrapolation::Throw:
			throw std::out_of_range("spk::CurveInterpolator x is after last point");

		default:
			return last->second;
		}
	}

	void CurveInterpolator::insert(float p_x, float p_y)
	{
		_validatePoint(p_x, p_y);

		_points[p_x] = p_y;
	}

	void CurveInterpolator::insert(const Point &p_point)
	{
		insert(p_point.x, p_point.y);
	}

	void CurveInterpolator::insert(std::span<const Point> p_points)
	{
		for (const Point &point : p_points)
		{
			insert(point);
		}
	}

	void CurveInterpolator::assign(std::span<const Point> p_points)
	{
		Points newPoints;

		for (const Point &point : p_points)
		{
			_validatePoint(point.x, point.y);

			newPoints[point.x] = point.y;
		}

		_points = std::move(newPoints);
	}

	void CurveInterpolator::assign(std::initializer_list<Point> p_points)
	{
		assign(std::span<const Point>(p_points.begin(), p_points.size()));
	}

	bool CurveInterpolator::remove(float p_x)
	{
		_validateX(p_x);

		return _points.erase(p_x) != 0;
	}

	void CurveInterpolator::clear() noexcept
	{
		_points.clear();
	}

	std::size_t CurveInterpolator::nbPoints() const noexcept
	{
		return _points.size();
	}

	bool CurveInterpolator::empty() const noexcept
	{
		return _points.empty();
	}

	const CurveInterpolator::Points &CurveInterpolator::points() const noexcept
	{
		return _points;
	}

	void CurveInterpolator::setInterpolation(Interpolation p_interpolation) noexcept
	{
		_interpolation = p_interpolation;
	}

	CurveInterpolator::Interpolation CurveInterpolator::interpolation() const noexcept
	{
		return _interpolation;
	}

	void CurveInterpolator::setExtrapolation(Extrapolation p_extrapolation) noexcept
	{
		_extrapolation = p_extrapolation;
	}

	CurveInterpolator::Extrapolation CurveInterpolator::extrapolation() const noexcept
	{
		return _extrapolation;
	}

	float CurveInterpolator::evaluate(float p_x) const
	{
		_validateX(p_x);

		if (_points.empty())
		{
			return 0.0f;
		}

		if (_points.size() == 1)
		{
			return _points.begin()->second;
		}

		const ConstIterator right = _points.lower_bound(p_x);

		if (right != _points.end() && right->first == p_x)
		{
			return right->second;
		}

		if (right == _points.begin())
		{
			return _extrapolateLeft(p_x);
		}

		if (right == _points.end())
		{
			return _extrapolateRight(p_x);
		}

		return _interpolate(p_x, std::prev(right), right);
	}

	float CurveInterpolator::operator()(float p_x) const
	{
		return evaluate(p_x);
	}

	float CurveInterpolator::operator[](float p_x) const
	{
		return evaluate(p_x);
	}
}