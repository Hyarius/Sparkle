#include "structures/game_engine/rendering/spk_scene_lighting_render_feature.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>
#include <vector>

#include "structures/game_engine/spk_component_registry.hpp"
#include "structures/game_engine/spk_camera_3d.hpp"
#include "structures/game_engine/spk_entity_3d.hpp"
#include "structures/game_engine/spk_light_3d.hpp"
#include "structures/game_engine/spk_transform_3d.hpp"
#include "structures/game_engine/rendering/spk_scene_render_passes.hpp"
#include "structures/game_engine/rendering/spk_scene_render_priorities.hpp"
#include "structures/game_engine/rendering/spk_shadow_render_pass.hpp"
#include "structures/graphics/rendering/command/spk_scene_lighting_update_render_command.hpp"
#include "structures/graphics/rendering/command/spk_directional_shadow_update_render_command.hpp"
#include "structures/graphics/rendering/spk_scene_gpu_bindings.hpp"
#include "structures/graphics/rendering/spk_scene_lighting_gpu_data.hpp"
#include "structures/graphics/rendering/pipeline/spk_render_pipeline.hpp"
#include "structures/graphics/spk_shader_storage_buffer_object.hpp"
#include "structures/graphics/spk_uniform_buffer_object.hpp"
#include "structures/graphics/spk_framebuffer_object.hpp"

namespace
{
	[[nodiscard]] bool finiteNonNegative(float value) { return std::isfinite(value) && value >= 0.0f; }
	[[nodiscard]] bool validColor(const spk::Color &value) { return finiteNonNegative(value.r) && finiteNonNegative(value.g) && finiteNonNegative(value.b); }

	struct Candidate
	{
		const spk::Light3D *light;
		float distanceSquared = 0.0f;
	};
	[[nodiscard]] bool ordered(const Candidate &left, const Candidate &right)
	{
		if (left.light->selectionPriority() != right.light->selectionPriority())
			return left.light->selectionPriority() > right.light->selectionPriority();
		if (left.distanceSquared != right.distanceSquared)
			return left.distanceSquared < right.distanceSquared;
		return left.light->entity()->uuid().bytes() < right.light->entity()->uuid().bytes();
	}

	template <typename TRecord>
	[[nodiscard]] std::shared_ptr<const spk::ShaderStorageBufferObject> uploadArray(
		GLuint binding, const std::vector<TRecord> &records)
	{
		const std::size_t size = std::max<std::size_t>(sizeof(TRecord), records.size() * sizeof(TRecord));
		auto buffer = std::make_shared<spk::ShaderStorageBufferObject>(binding, spk::BufferObject::Usage::StaticDraw, size, sizeof(TRecord));
		if (!records.empty()) buffer->edit(records.data(), records.size() * sizeof(TRecord));
		return buffer;
	}
}

namespace spk
{
	struct SceneLightingRenderFeature::FrameResources
	{
		std::shared_ptr<const spk::UniformBufferObject> header;
		std::shared_ptr<const spk::ShaderStorageBufferObject> directional, point, spot;
		spk::DirectionalShadowGpuData shadow{};
		std::shared_ptr<const spk::UniformBufferObject> shadowBuffer;
		std::vector<std::shared_ptr<const spk::FrameBufferObject>> shadowTargets;
		std::size_t cascadeCount = 0;
	};

	void SceneLightingRenderFeature::setEnvironmentLighting(const spk::EnvironmentLighting &value)
	{
		if (!validColor(value.ambientColor) || !finiteNonNegative(value.ambientIntensity))
			throw std::invalid_argument("Environment lighting must contain finite non-negative RGB and intensity values");
		_environment = value;
	}
	const spk::EnvironmentLighting &SceneLightingRenderFeature::environmentLighting() const noexcept { return _environment; }
	const spk::SceneLightingDiagnostics &SceneLightingRenderFeature::diagnostics() const noexcept { return _diagnostics; }
	void SceneLightingRenderFeature::setDirectionalShadowSettings(const spk::DirectionalShadowSettings &value)
	{
		if (value.resolution.x == 0 || value.resolution.y == 0 || value.cascadeCount == 0 || value.cascadeCount > 4 || !std::isfinite(value.maximumDistance) || value.maximumDistance <= 0.0f || !std::isfinite(value.splitLambda) || value.splitLambda < 0.0f || value.splitLambda > 1.0f || !std::isfinite(value.depthPadding) || value.depthPadding < 0.0f || !finiteNonNegative(value.constantBias) || !finiteNonNegative(value.normalBias) || value.pcfRadius > 4 || !std::isfinite(value.transitionFraction) || value.transitionFraction < 0.0f || value.transitionFraction > 0.5f)
			throw std::invalid_argument("Invalid directional shadow settings");
		const bool replaceTargets = value.resolution != _shadowSettings.resolution || value.cascadeCount != _shadowSettings.cascadeCount;
		_shadowSettings = value;
		if (replaceTargets) _shadowTargets.clear();
	}
	const spk::DirectionalShadowSettings &SceneLightingRenderFeature::directionalShadowSettings() const noexcept { return _shadowSettings; }

	void SceneLightingRenderFeature::prepareFrame(const spk::SceneRenderBuildContext &context)
	{
		_diagnostics = {};
		std::vector<Candidate> directional, point, spot;
		for (spk::Component *component : context.components.components<spk::Light3D>())
		{
			auto *light = static_cast<spk::Light3D *>(component);
			if (!light->isProcessable() || !light->hasType() || !finiteNonNegative(light->intensity()) || !validColor(light->color()) || light->intensity() == 0.0f)
				continue;
			Candidate candidate{.light = light};
			if (context.mainCamera != nullptr)
				candidate.distanceSquared = light->transform().worldPosition().squaredDistance(context.mainCamera->position());
			if (light->isDirectional()) { ++_diagnostics.processableDirectional; directional.push_back(candidate); }
			else if (light->isPoint() && finiteNonNegative(light->point().range) && light->point().range > 0.0f) { ++_diagnostics.processablePoint; point.push_back(candidate); }
			else if (light->isSpot() && finiteNonNegative(light->spot().range) && light->spot().range > 0.0f) { ++_diagnostics.processableSpot; spot.push_back(candidate); }
		}
		std::sort(directional.begin(), directional.end(), ordered);
		const spk::Light3D *shadowOwner = nullptr;
		for (const Candidate &candidate : directional)
			if (candidate.light->castsShadows()) { shadowOwner = candidate.light; break; }
		std::sort(point.begin(), point.end(), ordered);
		std::sort(spot.begin(), spot.end(), ordered);
		directional.resize(std::min(directional.size(), MaxDirectionalLights));
		if (shadowOwner != nullptr && std::none_of(directional.begin(), directional.end(), [shadowOwner](const Candidate &candidate) { return candidate.light == shadowOwner; }))
		{
			Candidate replacement{.light = shadowOwner};
			if (context.mainCamera != nullptr) replacement.distanceSquared = shadowOwner->transform().worldPosition().squaredDistance(context.mainCamera->position());
			directional.back() = replacement; std::sort(directional.begin(), directional.end(), ordered);
		}
		point.resize(std::min(point.size(), MaxPointLights));
		spot.resize(std::min(spot.size(), MaxSpotLights));
		_diagnostics.selectedDirectional = directional.size(); _diagnostics.selectedPoint = point.size(); _diagnostics.selectedSpot = spot.size();
		_diagnostics.discardedDirectional = _diagnostics.processableDirectional - directional.size();
		_diagnostics.discardedPoint = _diagnostics.processablePoint - point.size();
		_diagnostics.discardedSpot = _diagnostics.processableSpot - spot.size();

		std::vector<spk::DirectionalLightGpuData> directionalData;
		std::vector<spk::PointLightGpuData> pointData;
		std::vector<spk::SpotLightGpuData> spotData;
		for (const Candidate &candidate : directional)
			directionalData.push_back({.directionAndIntensity = {candidate.light->transform().worldForward(), candidate.light->intensity()}, .color = candidate.light->color()});
		for (const Candidate &candidate : point)
			pointData.push_back({.positionAndRange = {candidate.light->transform().worldPosition(), candidate.light->point().range}, .colorAndIntensity = {candidate.light->color().r, candidate.light->color().g, candidate.light->color().b, candidate.light->intensity()}});
		for (const Candidate &candidate : spot)
		{
			const auto &settings = candidate.light->spot();
			const float radians = std::numbers::pi_v<float> / 180.0f;
			spotData.push_back({.positionAndRange = {candidate.light->transform().worldPosition(), settings.range}, .directionAndInnerCosine = {candidate.light->transform().worldForward(), std::cos(settings.innerHalfAngleDegrees * radians)}, .colorAndIntensity = {candidate.light->color().r, candidate.light->color().g, candidate.light->color().b, candidate.light->intensity()}, .outerCosineAndPadding = {std::cos(settings.outerHalfAngleDegrees * radians), 0, 0, 0}});
		}
		auto resources = std::make_shared<FrameResources>();
		spk::LightingHeaderGpuData header{.ambientColorAndIntensity = {_environment.ambientColor.r, _environment.ambientColor.g, _environment.ambientColor.b, _environment.ambientIntensity}, .directionalCount = static_cast<std::uint32_t>(directionalData.size()), .pointCount = static_cast<std::uint32_t>(pointData.size()), .spotCount = static_cast<std::uint32_t>(spotData.size())};
		auto headerBuffer = std::make_shared<spk::UniformBufferObject>(spk::SceneGpuBindings::LightingHeader, spk::BufferObject::Usage::StaticDraw, sizeof(header));
		headerBuffer->edit(&header, sizeof(header)); resources->header = headerBuffer;
		resources->directional = uploadArray(spk::SceneGpuBindings::DirectionalLights, directionalData);
		resources->point = uploadArray(spk::SceneGpuBindings::PointLights, pointData);
		resources->spot = uploadArray(spk::SceneGpuBindings::SpotLights, spotData);
		if (shadowOwner != nullptr && context.mainCamera != nullptr && context.mainCamera->nearPlane() > 0.0f && context.mainCamera->farPlane() > context.mainCamera->nearPlane())
		{
			for (std::size_t index = 0; index < directional.size(); ++index)
				if (directional[index].light == shadowOwner) header.shadowDirectionalIndex = static_cast<std::int32_t>(index);
			if (header.shadowDirectionalIndex >= 0)
			{
				const float nearDistance = context.mainCamera->nearPlane();
				const float farDistance = std::min(context.mainCamera->farPlane(), _shadowSettings.maximumDistance);
				if (farDistance > nearDistance)
				{
					if (_shadowTargets.size() != _shadowSettings.cascadeCount) {
						_shadowTargets.clear(); _shadowTargets.reserve(_shadowSettings.cascadeCount);
						for (std::size_t i = 0; i < _shadowSettings.cascadeCount; ++i) _shadowTargets.push_back(std::make_shared<spk::FrameBufferObject>(spk::FrameBufferObject::depthTextureTarget(_shadowSettings.resolution, spk::Texture::Format::Depth24)));
					}
					float previous = nearDistance;
					spk::Vector3 rayDirection;
					try
					{
						rayDirection = shadowOwner->transform().worldForward();
					}
					catch (const std::exception &exception)
					{
						throw std::runtime_error(std::string("Cannot resolve directional shadow owner orientation: ") + exception.what());
					}
					for (std::size_t i = 0; i < _shadowSettings.cascadeCount; ++i) {
						const float ratio = static_cast<float>(i + 1) / static_cast<float>(_shadowSettings.cascadeCount);
						const float logarithmic = nearDistance * std::pow(farDistance / nearDistance, ratio);
						const float linear = nearDistance + (farDistance - nearDistance) * ratio;
						const float split = linear + (logarithmic - linear) * _shadowSettings.splitLambda;
						const auto corners = context.mainCamera->frustumSliceCorners(previous, split);
						spk::Vector3 center{}; for (const auto &corner : corners) center += corner; center /= static_cast<float>(corners.size());
						float radius = 0.0f; for (const auto &corner : corners) radius = std::max(radius, corner.distance(center));
						radius = std::ceil(radius * 16.0f) / 16.0f;
						const spk::Matrix4x4 lightView = spk::Matrix4x4::lookAt(center - rayDirection * (radius + _shadowSettings.depthPadding), center, spk::Vector3(0, 1, 0));
						resources->shadow.lightViewProjection[i] = spk::Matrix4x4::ortho(-radius, radius, -radius, radius, 0.0f, 2.0f * (radius + _shadowSettings.depthPadding)) * lightView;
						if (i == 0) resources->shadow.cascadeFarDistances.x = split;
						else if (i == 1) resources->shadow.cascadeFarDistances.y = split;
						else if (i == 2) resources->shadow.cascadeFarDistances.z = split;
						else resources->shadow.cascadeFarDistances.w = split;
						previous = split;
					}
					resources->shadow.biasDistanceAndTransition = {_shadowSettings.constantBias, _shadowSettings.normalBias, _shadowSettings.maximumDistance, _shadowSettings.transitionFraction};
					resources->shadow.control = {static_cast<std::uint32_t>(_shadowSettings.cascadeCount), static_cast<std::uint32_t>(_shadowSettings.pcfRadius), 1u, 0u};
		resources->shadowTargets = _shadowTargets; resources->cascadeCount = _shadowSettings.cascadeCount;
				}
			}
		}
		headerBuffer->edit(&header, sizeof(header));
		auto shadowBuffer = std::make_shared<spk::UniformBufferObject>(spk::SceneGpuBindings::DirectionalShadow, spk::BufferObject::Usage::StaticDraw, sizeof(resources->shadow));
		shadowBuffer->edit(&resources->shadow, sizeof(resources->shadow)); resources->shadowBuffer = shadowBuffer;
		_frame = std::move(resources);
	}

	void SceneLightingRenderFeature::contribute(const spk::SceneRenderBuildContext &context)
	{
		if (_frame == nullptr) return;
		for (const auto type : {spk::SceneRenderPasses::MainOpaque, spk::SceneRenderPasses::MainTransparent, spk::SceneRenderPasses::MainOverlay})
		{
			auto &pass = context.frame.passes.require(type);
			pass.emplace<spk::SceneLightingUpdateRenderCommand>(_frame->header, _frame->directional, _frame->point, _frame->spot);
			std::array<std::shared_ptr<const spk::FrameBufferObject>, 4> targets{};
			for (std::size_t index = 0; index < _frame->shadowTargets.size() && index < targets.size(); ++index) targets[index] = _frame->shadowTargets[index];
			pass.emplace<spk::DirectionalShadowUpdateRenderCommand>(_frame->shadowBuffer, std::move(targets));
		}
	}

	void SceneLightingRenderFeature::declarePasses(const spk::SceneRenderBuildContext &context, spk::RenderPipeline &passes)
	{
		if (_frame == nullptr || _frame->cascadeCount == 0) return;
		for (std::size_t index = 0; index < _frame->cascadeCount; ++index) {
			const std::string name = "DirectionalShadow[" + std::to_string(index) + "]";
			auto target = _frame->shadowTargets[index];
			passes.emplace<spk::ShadowRenderPass>(name, spk::SceneRenderPriorities::ShadowBase + static_cast<std::int32_t>(index), {.target = {.ownedFrameBuffer = target, .frameBuffer = target.get(), .viewport = target->viewport()}, .clear = {.depth = 1.0f}}, static_cast<std::uint32_t>(index), _frame->shadow.lightViewProjection[index]);
		}
	}
}
