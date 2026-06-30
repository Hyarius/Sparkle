#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace spk
{
	class Perlin
	{
	public:
		enum class Interpolation
		{
			Linear,
			SmoothStep,
			SmootherStep
		};

		struct Parameters
		{
			std::uint32_t seed = 0;
			int octaves = 3;
			float persistence = 0.5f;
			float lacunarity = 2.0f;
			float frequency = 1.0f;
			Interpolation interpolation = Interpolation::SmootherStep;
		};

	private:
		static constexpr std::size_t _permutationSize = 256;
		static constexpr std::size_t _permutationMask = _permutationSize - 1;
		static constexpr std::size_t _permutationTableSize = _permutationSize * 2;

		using PermutationTable = std::array<std::uint8_t, _permutationTableSize>;

	private:
		Parameters _parameters;
		PermutationTable _permutation = {};

	private:
		static void _validateParameters(const Parameters &p_parameters);
		static void _validateRange(float p_min, float p_max);

		[[nodiscard]] static bool _isFinite(float p_value) noexcept;

		[[nodiscard]] static std::uint64_t _splitMix64(std::uint64_t &p_state) noexcept;
		[[nodiscard]] static PermutationTable _buildPermutationTable(std::uint32_t p_seed);

		[[nodiscard]] static int _fastFloor(float p_value) noexcept;

		[[nodiscard]] static float _lerp(float p_from, float p_to, float p_factor) noexcept;
		[[nodiscard]] static float _smoothStep(float p_value) noexcept;
		[[nodiscard]] static float _smootherStep(float p_value) noexcept;

		[[nodiscard]] float _fade(float p_value) const noexcept;

		[[nodiscard]] std::uint8_t _hash1D(int p_x) const noexcept;
		[[nodiscard]] std::uint8_t _hash2D(int p_x, int p_y) const noexcept;
		[[nodiscard]] std::uint8_t _hash3D(int p_x, int p_y, int p_z) const noexcept;

		[[nodiscard]] static float _gradient1D(std::uint8_t p_hash, float p_x) noexcept;
		[[nodiscard]] static float _gradient2D(std::uint8_t p_hash, float p_x, float p_y) noexcept;
		[[nodiscard]] static float _gradient3D(std::uint8_t p_hash, float p_x, float p_y, float p_z) noexcept;

		[[nodiscard]] float _noise1D(float p_x) const noexcept;
		[[nodiscard]] float _noise2D(float p_x, float p_y) const noexcept;
		[[nodiscard]] float _noise3D(float p_x, float p_y, float p_z) const noexcept;

		[[nodiscard]] static float _mapRawToRange(float p_value, float p_min, float p_max) noexcept;

	public:
		explicit Perlin();
		explicit Perlin(const Parameters &p_parameters);

		void reseed(std::uint32_t p_seed);
		void setParameters(const Parameters &p_parameters);

		[[nodiscard]] const Parameters &parameters() const noexcept;

		[[nodiscard]] float raw1D(float p_x) const;
		[[nodiscard]] float raw2D(float p_x, float p_y) const;
		[[nodiscard]] float raw3D(float p_x, float p_y, float p_z) const;

		[[nodiscard]] float sample1D(float p_x, float p_min = 0.0f, float p_max = 1.0f) const;
		[[nodiscard]] float sample2D(float p_x, float p_y, float p_min = 0.0f, float p_max = 1.0f) const;
		[[nodiscard]] float sample3D(float p_x, float p_y, float p_z, float p_min = 0.0f, float p_max = 1.0f) const;
	};
}