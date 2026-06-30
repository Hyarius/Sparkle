#include "structures/math/spk_perlin.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>

namespace spk
{
	Perlin::Perlin() :
		Perlin(Parameters{})
	{

	}
	
	Perlin::Perlin(const Parameters &p_parameters)
	{
		setParameters(p_parameters);
	}

	bool Perlin::_isFinite(float p_value) noexcept
	{
		return std::isfinite(p_value);
	}

	void Perlin::_validateParameters(const Parameters &p_parameters)
	{
		if (p_parameters.octaves <= 0)
		{
			throw std::invalid_argument("spk::Perlin octaves must be greater than zero");
		}

		if (_isFinite(p_parameters.persistence) == false || p_parameters.persistence < 0.0f)
		{
			throw std::invalid_argument("spk::Perlin persistence must be finite and greater than or equal to zero");
		}

		if (_isFinite(p_parameters.lacunarity) == false || p_parameters.lacunarity <= 0.0f)
		{
			throw std::invalid_argument("spk::Perlin lacunarity must be finite and greater than zero");
		}

		if (_isFinite(p_parameters.frequency) == false || p_parameters.frequency <= 0.0f)
		{
			throw std::invalid_argument("spk::Perlin frequency must be finite and greater than zero");
		}
	}

	void Perlin::_validateRange(float p_min, float p_max)
	{
		if (_isFinite(p_min) == false || _isFinite(p_max) == false)
		{
			throw std::invalid_argument("spk::Perlin sample range must be finite");
		}
	}

	std::uint64_t Perlin::_splitMix64(std::uint64_t &p_state) noexcept
	{
		p_state += 0x9E3779B97F4A7C15ull;

		std::uint64_t result = p_state;

		result = (result ^ (result >> 30)) * 0xBF58476D1CE4E5B9ull;
		result = (result ^ (result >> 27)) * 0x94D049BB133111EBull;
		result = result ^ (result >> 31);

		return result;
	}

	Perlin::PermutationTable Perlin::_buildPermutationTable(std::uint32_t p_seed)
	{
		std::array<std::uint8_t, _permutationSize> base = {};

		for (std::size_t i = 0; i < _permutationSize; ++i)
		{
			base[i] = static_cast<std::uint8_t>(i);
		}

		std::uint64_t state = static_cast<std::uint64_t>(p_seed);

		for (std::size_t i = _permutationSize - 1; i > 0; --i)
		{
			const std::size_t j = static_cast<std::size_t>(_splitMix64(state) % (i + 1));

			std::swap(base[i], base[j]);
		}

		PermutationTable result = {};

		for (std::size_t i = 0; i < _permutationSize; ++i)
		{
			result[i] = base[i];
			result[i + _permutationSize] = base[i];
		}

		return result;
	}

	int Perlin::_fastFloor(float p_value) noexcept
	{
		const int truncated = static_cast<int>(p_value);

		return p_value < static_cast<float>(truncated) ? truncated - 1 : truncated;
	}

	float Perlin::_lerp(float p_from, float p_to, float p_factor) noexcept
	{
		return p_from + (p_to - p_from) * p_factor;
	}

	float Perlin::_smoothStep(float p_value) noexcept
	{
		return p_value * p_value * (3.0f - 2.0f * p_value);
	}

	float Perlin::_smootherStep(float p_value) noexcept
	{
		return p_value * p_value * p_value * (p_value * (p_value * 6.0f - 15.0f) + 10.0f);
	}

	float Perlin::_fade(float p_value) const noexcept
	{
		switch (_parameters.interpolation)
		{
		case Interpolation::Linear:
			return p_value;

		case Interpolation::SmoothStep:
			return _smoothStep(p_value);

		default:
			return _smootherStep(p_value);
		}
	}

	std::uint8_t Perlin::_hash1D(int p_x) const noexcept
	{
		return _permutation[static_cast<std::size_t>(p_x) & _permutationMask];
	}

	std::uint8_t Perlin::_hash2D(int p_x, int p_y) const noexcept
	{
		const std::size_t x = static_cast<std::size_t>(p_x) & _permutationMask;
		const std::size_t y = static_cast<std::size_t>(p_y) & _permutationMask;

		return _permutation[static_cast<std::size_t>(_permutation[x]) + y];
	}

	std::uint8_t Perlin::_hash3D(int p_x, int p_y, int p_z) const noexcept
	{
		const std::size_t x = static_cast<std::size_t>(p_x) & _permutationMask;
		const std::size_t y = static_cast<std::size_t>(p_y) & _permutationMask;
		const std::size_t z = static_cast<std::size_t>(p_z) & _permutationMask;

		return _permutation[static_cast<std::size_t>(_permutation[static_cast<std::size_t>(_permutation[x]) + y]) + z];
	}

	float Perlin::_gradient1D(std::uint8_t p_hash, float p_x) noexcept
	{
		return (p_hash & 1u) == 0u ? p_x : -p_x;
	}

	float Perlin::_gradient2D(std::uint8_t p_hash, float p_x, float p_y) noexcept
	{
		switch (p_hash & 7u)
		{
		case 0u:
			return p_x;
		case 1u:
			return -p_x;
		case 2u:
			return p_y;
		case 3u:
			return -p_y;
		case 4u:
			return 0.70710678118f * (p_x + p_y);
		case 5u:
			return 0.70710678118f * (-p_x + p_y);
		case 6u:
			return 0.70710678118f * (p_x - p_y);
		default:
			return 0.70710678118f * (-p_x - p_y);
		}
	}

	float Perlin::_gradient3D(std::uint8_t p_hash, float p_x, float p_y, float p_z) noexcept
	{
		const std::uint8_t hash = p_hash & 15u;

		const float u = hash < 8u ? p_x : p_y;
		const float v = hash < 4u ? p_y : (hash == 12u || hash == 14u ? p_x : p_z);

		const float first = (hash & 1u) == 0u ? u : -u;
		const float second = (hash & 2u) == 0u ? v : -v;

		return first + second;
	}

	float Perlin::_noise1D(float p_x) const noexcept
	{
		const int x0 = _fastFloor(p_x);
		const int x1 = x0 + 1;

		const float localX0 = p_x - static_cast<float>(x0);
		const float localX1 = localX0 - 1.0f;

		const float u = _fade(localX0);

		const float n0 = _gradient1D(_hash1D(x0), localX0);
		const float n1 = _gradient1D(_hash1D(x1), localX1);

		return 2.0f * _lerp(n0, n1, u);
	}

	float Perlin::_noise2D(float p_x, float p_y) const noexcept
	{
		const int x0 = _fastFloor(p_x);
		const int y0 = _fastFloor(p_y);

		const int x1 = x0 + 1;
		const int y1 = y0 + 1;

		const float localX0 = p_x - static_cast<float>(x0);
		const float localY0 = p_y - static_cast<float>(y0);

		const float localX1 = localX0 - 1.0f;
		const float localY1 = localY0 - 1.0f;

		const float u = _fade(localX0);
		const float v = _fade(localY0);

		const float n00 = _gradient2D(_hash2D(x0, y0), localX0, localY0);
		const float n10 = _gradient2D(_hash2D(x1, y0), localX1, localY0);
		const float n01 = _gradient2D(_hash2D(x0, y1), localX0, localY1);
		const float n11 = _gradient2D(_hash2D(x1, y1), localX1, localY1);

		const float nx0 = _lerp(n00, n10, u);
		const float nx1 = _lerp(n01, n11, u);

		return _lerp(nx0, nx1, v);
	}

	float Perlin::_noise3D(float p_x, float p_y, float p_z) const noexcept
	{
		const int x0 = _fastFloor(p_x);
		const int y0 = _fastFloor(p_y);
		const int z0 = _fastFloor(p_z);

		const int x1 = x0 + 1;
		const int y1 = y0 + 1;
		const int z1 = z0 + 1;

		const float localX0 = p_x - static_cast<float>(x0);
		const float localY0 = p_y - static_cast<float>(y0);
		const float localZ0 = p_z - static_cast<float>(z0);

		const float localX1 = localX0 - 1.0f;
		const float localY1 = localY0 - 1.0f;
		const float localZ1 = localZ0 - 1.0f;

		const float u = _fade(localX0);
		const float v = _fade(localY0);
		const float w = _fade(localZ0);

		const float n000 = _gradient3D(_hash3D(x0, y0, z0), localX0, localY0, localZ0);
		const float n100 = _gradient3D(_hash3D(x1, y0, z0), localX1, localY0, localZ0);
		const float n010 = _gradient3D(_hash3D(x0, y1, z0), localX0, localY1, localZ0);
		const float n110 = _gradient3D(_hash3D(x1, y1, z0), localX1, localY1, localZ0);

		const float n001 = _gradient3D(_hash3D(x0, y0, z1), localX0, localY0, localZ1);
		const float n101 = _gradient3D(_hash3D(x1, y0, z1), localX1, localY0, localZ1);
		const float n011 = _gradient3D(_hash3D(x0, y1, z1), localX0, localY1, localZ1);
		const float n111 = _gradient3D(_hash3D(x1, y1, z1), localX1, localY1, localZ1);

		const float nx00 = _lerp(n000, n100, u);
		const float nx10 = _lerp(n010, n110, u);
		const float nx01 = _lerp(n001, n101, u);
		const float nx11 = _lerp(n011, n111, u);

		const float nxy0 = _lerp(nx00, nx10, v);
		const float nxy1 = _lerp(nx01, nx11, v);

		return _lerp(nxy0, nxy1, w);
	}

	float Perlin::_mapRawToRange(float p_value, float p_min, float p_max) noexcept
	{
		const float clamped = std::clamp(p_value, -1.0f, 1.0f);
		const float normalized = (clamped + 1.0f) * 0.5f;

		return _lerp(p_min, p_max, normalized);
	}

	void Perlin::reseed(std::uint32_t p_seed)
	{
		_parameters.seed = p_seed;
		_permutation = _buildPermutationTable(p_seed);
	}

	void Perlin::setParameters(const Parameters &p_parameters)
	{
		_validateParameters(p_parameters);

		_parameters = p_parameters;
		_permutation = _buildPermutationTable(_parameters.seed);
	}

	const Perlin::Parameters &Perlin::parameters() const noexcept
	{
		return _parameters;
	}

	float Perlin::raw1D(float p_x) const
	{
		if (_isFinite(p_x) == false)
		{
			throw std::invalid_argument("spk::Perlin x must be finite");
		}

		float result = 0.0f;
		float amplitude = 1.0f;
		float amplitudeSum = 0.0f;
		float frequency = _parameters.frequency;

		for (int i = 0; i < _parameters.octaves; ++i)
		{
			result += _noise1D(p_x * frequency) * amplitude;
			amplitudeSum += amplitude;

			amplitude *= _parameters.persistence;
			frequency *= _parameters.lacunarity;
		}

		return result / amplitudeSum;
	}

	float Perlin::raw2D(float p_x, float p_y) const
	{
		if (_isFinite(p_x) == false || _isFinite(p_y) == false)
		{
			throw std::invalid_argument("spk::Perlin coordinates must be finite");
		}

		float result = 0.0f;
		float amplitude = 1.0f;
		float amplitudeSum = 0.0f;
		float frequency = _parameters.frequency;

		for (int i = 0; i < _parameters.octaves; ++i)
		{
			result += _noise2D(p_x * frequency, p_y * frequency) * amplitude;
			amplitudeSum += amplitude;

			amplitude *= _parameters.persistence;
			frequency *= _parameters.lacunarity;
		}

		return result / amplitudeSum;
	}

	float Perlin::raw3D(float p_x, float p_y, float p_z) const
	{
		if (_isFinite(p_x) == false || _isFinite(p_y) == false || _isFinite(p_z) == false)
		{
			throw std::invalid_argument("spk::Perlin coordinates must be finite");
		}

		float result = 0.0f;
		float amplitude = 1.0f;
		float amplitudeSum = 0.0f;
		float frequency = _parameters.frequency;

		for (int i = 0; i < _parameters.octaves; ++i)
		{
			result += _noise3D(p_x * frequency, p_y * frequency, p_z * frequency) * amplitude;
			amplitudeSum += amplitude;

			amplitude *= _parameters.persistence;
			frequency *= _parameters.lacunarity;
		}

		return result / amplitudeSum;
	}

	float Perlin::sample1D(float p_x, float p_min, float p_max) const
	{
		_validateRange(p_min, p_max);

		return _mapRawToRange(raw1D(p_x), p_min, p_max);
	}

	float Perlin::sample2D(float p_x, float p_y, float p_min, float p_max) const
	{
		_validateRange(p_min, p_max);

		return _mapRawToRange(raw2D(p_x, p_y), p_min, p_max);
	}

	float Perlin::sample3D(float p_x, float p_y, float p_z, float p_min, float p_max) const
	{
		_validateRange(p_min, p_max);

		return _mapRawToRange(raw3D(p_x, p_y, p_z), p_min, p_max);
	}
}