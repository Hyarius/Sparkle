#pragma once

#include <array>
#include <cstdint>
#include <type_traits>

#include "structures/graphics/geometry/spk_color.hpp"
#include "structures/math/spk_matrix.hpp"
#include "structures/math/spk_vector4.hpp"

namespace spk
{
	struct alignas(16) CameraGpuData
	{
		spk::Matrix4x4 viewProjection;
		spk::Matrix4x4 view;
	};

	// direction.xyz is the normalized ray-travel direction (emitter to scene).
	struct alignas(16) DirectionalLightGpuData
	{
		spk::Vector4 directionAndIntensity;
		spk::Color color;
	};
	struct alignas(16) PointLightGpuData
	{
		spk::Vector4 positionAndRange;
		spk::Vector4 colorAndIntensity;
	};
	struct alignas(16) SpotLightGpuData
	{
		spk::Vector4 positionAndRange;
		spk::Vector4 directionAndInnerCosine;
		spk::Vector4 colorAndIntensity;
		spk::Vector4 outerCosineAndPadding;
	};
	struct alignas(16) LightingHeaderGpuData
	{
		spk::Vector4 ambientColorAndIntensity;
		std::uint32_t directionalCount = 0;
		std::uint32_t pointCount = 0;
		std::uint32_t spotCount = 0;
		std::int32_t shadowDirectionalIndex = -1;
	};
	struct alignas(16) DirectionalShadowGpuData
	{
		std::array<spk::Matrix4x4, 4> lightViewProjection{};
		spk::Vector4 cascadeFarDistances{};
		spk::Vector4 biasDistanceAndTransition{};
		std::array<std::uint32_t, 4> control{};
	};

	static_assert(sizeof(CameraGpuData) == 128 && alignof(CameraGpuData) == 16 && std::is_trivially_copyable_v<CameraGpuData>);
	static_assert(sizeof(DirectionalLightGpuData) == 32 && alignof(DirectionalLightGpuData) == 16 && std::is_trivially_copyable_v<DirectionalLightGpuData>);
	static_assert(sizeof(PointLightGpuData) == 32 && alignof(PointLightGpuData) == 16 && std::is_trivially_copyable_v<PointLightGpuData>);
	static_assert(sizeof(SpotLightGpuData) == 64 && alignof(SpotLightGpuData) == 16 && std::is_trivially_copyable_v<SpotLightGpuData>);
	static_assert(sizeof(LightingHeaderGpuData) == 32 && alignof(LightingHeaderGpuData) == 16 && std::is_trivially_copyable_v<LightingHeaderGpuData>);
	static_assert(sizeof(DirectionalShadowGpuData) == 304 && alignof(DirectionalShadowGpuData) == 16 && std::is_trivially_copyable_v<DirectionalShadowGpuData>);
}
