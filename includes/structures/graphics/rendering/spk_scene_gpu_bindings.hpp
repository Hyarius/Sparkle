#pragma once

#include <cstdint>

// Keep this contract synchronized with resources/shaders/mesh_3d.
namespace spk::SceneGpuBindings
{
	inline constexpr std::uint32_t Camera = 1;
	inline constexpr std::uint32_t Model = 2;
	inline constexpr std::uint32_t LightingHeader = 3;
	inline constexpr std::uint32_t DirectionalLights = 4;
	inline constexpr std::uint32_t PointLights = 5;
	inline constexpr std::uint32_t SpotLights = 6;
	inline constexpr std::uint32_t DirectionalShadow = 7;

	inline constexpr std::uint32_t MaterialTextureUnit = 0;
	inline constexpr std::uint32_t DirectionalShadowUnit0 = 8;
	inline constexpr std::uint32_t DirectionalShadowUnit1 = 9;
	inline constexpr std::uint32_t DirectionalShadowUnit2 = 10;
	inline constexpr std::uint32_t DirectionalShadowUnit3 = 11;
}
