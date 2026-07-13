#include "logics/day_time_management_logic.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

#include "structures/math/spk_quaternion.hpp"
#include "structures/game_engine/rendering/spk_scene_lighting_render_feature.hpp"

namespace pg
{
	DayTimeManagementLogic::DayTimeManagementLogic(spk::Entity3D &p_sunEntity, spk::SceneLightingRenderFeature &p_lighting) : _sunEntity(p_sunEntity), _lighting(p_lighting) {}
	float DayTimeManagementLogic::timeOfDayHours() const noexcept { return _timeOfDayHours; }

	void DayTimeManagementLogic::_parseComponentForUpdate(const spk::UpdateContext &p_tick, spk::Light3D &light)
	{
		if (light.entity() != &_sunEntity) return;
		_timeOfDayHours = std::fmod(_timeOfDayHours + static_cast<float>(p_tick.deltaTime.seconds()) * (24.0f / 60.0f), 24.0f);
		// Daylight runs 08:00 -> 21:00 (13 h day, 11 h night): the clock is warped
		// onto the symmetric solar curve so sunrise/sunset land on those hours while
		// elevation and azimuth stay continuous across both transitions.
		constexpr float sunriseHour = 8.0f;
		constexpr float sunsetHour = 21.0f;
		constexpr float dayLengthHours = sunsetHour - sunriseHour;
		constexpr float nightLengthHours = 24.0f - dayLengthHours;
		const bool isDaytime = (_timeOfDayHours >= sunriseHour) && (_timeOfDayHours < sunsetHour);
		const float solarHours = isDaytime
			? 6.0f + (_timeOfDayHours - sunriseHour) * (12.0f / dayLengthHours)
			: 18.0f + std::fmod(_timeOfDayHours - sunsetHour + 24.0f, 24.0f) * (12.0f / nightLengthHours);
		const float elevation = std::sin((solarHours - 6.0f) * std::numbers::pi_v<float> / 12.0f);
		const float daylight = std::max(elevation, 0.0f);
		const float horizontal = std::max(0.08f, std::sqrt(std::max(1.0f - elevation * elevation, 0.0f)));
		const float azimuth = (solarHours - 12.0f) * std::numbers::pi_v<float> / 12.0f;
		// Keep the streamed world readable throughout the fast preview cycle.
		// This is an artistic exposure curve, not physically calibrated radiance.
		light.intensity() = 0.72f * daylight;
		light.color() = {1.0f, 0.62f + 0.34f * daylight, 0.45f + 0.39f * daylight, 1.0f};
		// Skylight deliberately carries more of the exposure than direct sun: it
		// keeps voxel side faces readable without clipping upward-facing grass.
		_lighting.setEnvironmentLighting({.ambientColor = {0.26f + 0.36f * daylight, 0.30f + 0.36f * daylight, 0.42f + 0.30f * daylight, 1.0f}, .ambientIntensity = 0.22f + 0.33f * daylight});
		// Sun rays travel from the emitter into the scene. A small horizontal floor
		// avoids the look-at singularity at a perfectly vertical noon direction.
		_sunEntity.transform().setRotation(spk::Quaternion::lookAt(
			{0.0f, 0.0f, 0.0f},
			{std::cos(azimuth) * horizontal, -elevation, std::sin(azimuth) * horizontal}));
	}
}
