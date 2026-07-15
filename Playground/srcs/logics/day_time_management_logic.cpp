#include "logics/day_time_management_logic.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

#include "structures/game_engine/rendering/spk_scene_lighting_render_feature.hpp"
#include "structures/math/spk_quaternion.hpp"

namespace pg
{
	DayTimeManagementLogic::DayTimeManagementLogic(spk::Entity3D &p_sunEntity, spk::SceneLightingRenderFeature &p_lighting) :
		_sunEntity(p_sunEntity),
		_lighting(p_lighting)
	{
	}
	float DayTimeManagementLogic::timeOfDayHours() const noexcept
	{
		return _timeOfDayHours;
	}

	void DayTimeManagementLogic::_parseComponentForUpdate(const spk::UpdateContext &p_tick, spk::Light3D &light)
	{
		if (light.entity() != &_sunEntity)
		{
			return;
		}
		// Daylight runs 08:00 -> 21:00 (13 artistic hours) in 15 real minutes;
		// the remaining 11 artistic hours take 5 real minutes. Advancing the two
		// portions at separate rates makes their real durations exact.
		constexpr float sunriseHour = 8.0f;
		constexpr float sunsetHour = 21.0f;
		constexpr float dayLengthHours = sunsetHour - sunriseHour;
		constexpr float nightLengthHours = 24.0f - dayLengthHours;
		constexpr float dayDurationSeconds = 15.0f * 60.0f;
		constexpr float nightDurationSeconds = 5.0f * 60.0f;
		float remainingSeconds = static_cast<float>(p_tick.deltaTime.seconds());
		while (remainingSeconds > 0.0f)
		{
			const bool isDaytime = (_timeOfDayHours >= sunriseHour) && (_timeOfDayHours < sunsetHour);
			const float hoursPerSecond = isDaytime ? dayLengthHours / dayDurationSeconds : nightLengthHours / nightDurationSeconds;
			const float boundary = isDaytime ? sunsetHour : (_timeOfDayHours < sunriseHour ? sunriseHour : 24.0f + sunriseHour);
			const float hoursToBoundary = boundary - _timeOfDayHours;
			const float secondsToBoundary = hoursToBoundary / hoursPerSecond;
			if (remainingSeconds < secondsToBoundary)
			{
				_timeOfDayHours += remainingSeconds * hoursPerSecond;
				break;
			}
			_timeOfDayHours = std::fmod(boundary, 24.0f);
			remainingSeconds -= secondsToBoundary;
		}
		// The clock is warped onto the symmetric solar curve so sunrise/sunset land
		// on the selected artistic hours while elevation and azimuth stay continuous.
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
		_sunEntity.transform().setRotation(spk::Quaternion::lookAt({0.0f, 0.0f, 0.0f}, {std::cos(azimuth) * horizontal, -elevation, std::sin(azimuth) * horizontal}));
	}
}
