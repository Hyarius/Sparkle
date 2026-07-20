#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <vector>

#include "structures/game_engine/rendering/spk_scene_render_pipeline_feature.hpp"
#include "structures/graphics/geometry/spk_color.hpp"

namespace spk
{
	class UniformBufferObject;
	class ShaderStorageBufferObject;
	class FrameBufferObject;
	struct LightingHeaderGpuData;

	struct EnvironmentLighting
	{
		spk::Color ambientColor{1.0f, 1.0f, 1.0f, 1.0f};
		float ambientIntensity = 0.20f;
	};

	struct SceneLightingDiagnostics
	{
		std::size_t processableDirectional = 0, processablePoint = 0, processableSpot = 0;
		std::size_t selectedDirectional = 0, selectedPoint = 0, selectedSpot = 0;
		std::size_t discardedDirectional = 0, discardedPoint = 0, discardedSpot = 0;
	};
	struct DirectionalShadowSettings
	{
		spk::Vector2UInt resolution{2048, 2048};
		std::size_t cascadeCount = 3;
		float maximumDistance = 192.0f;
		float splitLambda = 0.65f;
		float depthPadding = 48.0f;
		float constantBias = 0.0008f;
		float normalBias = 0.0025f;
		std::size_t pcfRadius = 1;
		float transitionFraction = 0.10f;
	};

	class SceneLightingRenderFeature final : public spk::ISceneRenderPipelineFeature
	{
		struct FrameResources;
		spk::EnvironmentLighting _environment;
		spk::SceneLightingDiagnostics _diagnostics;
		spk::DirectionalShadowSettings _shadowSettings;
		std::vector<std::shared_ptr<const spk::FrameBufferObject>> _shadowTargets;
		std::shared_ptr<const FrameResources> _frame;

	public:
		static constexpr std::size_t MaxDirectionalLights = 4;
		static constexpr std::size_t MaxPointLights = 128;
		static constexpr std::size_t MaxSpotLights = 64;

		void setEnvironmentLighting(const spk::EnvironmentLighting &p_environment);
		[[nodiscard]] const spk::EnvironmentLighting &environmentLighting() const noexcept;
		[[nodiscard]] const spk::SceneLightingDiagnostics &diagnostics() const noexcept;
		void setDirectionalShadowSettings(const spk::DirectionalShadowSettings &p_settings);
		[[nodiscard]] const spk::DirectionalShadowSettings &directionalShadowSettings() const noexcept;

		void prepareFrame(const spk::SceneRenderBuildContext &p_context) override;
		void declarePasses(const spk::SceneRenderBuildContext &p_context, spk::RenderPipeline &p_passes) override;
		void contribute(const spk::SceneRenderBuildContext &p_context) override;
	};
}
