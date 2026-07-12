#pragma once

#include <variant>

#include "structures/game_engine/spk_component_3d.hpp"
#include "structures/graphics/geometry/spk_color.hpp"

namespace spk
{
	struct DirectionalLightSettings
	{
	};

	struct PointLightSettings
	{
		float range = 10.0f;
	};

	struct SpotLightSettings
	{
		float range = 15.0f;
		float innerHalfAngleDegrees = 20.0f;
		float outerHalfAngleDegrees = 30.0f;
	};

	using LightShape = std::variant<
		std::monostate,
		spk::DirectionalLightSettings,
		spk::PointLightSettings,
		spk::SpotLightSettings>;

	/**
	 * Editable 3D light component.
	 *
	 * Directional and spot directions use the owning Entity3D local -Z axis,
	 * transformed by semantic world rotation. The direction represents ray
	 * travel from the emitter into the scene.
	 *
	 * Data is edited directly through mutable accessors. The component does not
	 * validate authored values.
	 */
	class Light3D final : public spk::Component3D
	{
	private:
		spk::LightShape _shape = std::monostate{};
		spk::Color _color{1.0f, 1.0f, 1.0f, 1.0f};
		float _intensity = 1.0f;
		bool _castsShadows = false;

		/**
		 * Renderer-side selection priority.
		 *
		 * This is not a render-pass priority. It is used when bounded GPU light
		 * lists must discard lights and, later, when selecting the directional
		 * light allowed to own cascaded shadows.
		 */
		int _selectionPriority = 0;

	public:
		Light3D() = default;
		~Light3D() override = default;

		[[nodiscard]] bool hasType() const noexcept;
		[[nodiscard]] bool isDirectional() const noexcept;
		[[nodiscard]] bool isPoint() const noexcept;
		[[nodiscard]] bool isSpot() const noexcept;

		/**
		 * Typed access to the currently active settings.
		 *
		 * std::bad_variant_access is thrown when the requested settings are not
		 * the active variant.
		 */
		[[nodiscard]] spk::DirectionalLightSettings &directional();
		[[nodiscard]] const spk::DirectionalLightSettings &directional() const;

		[[nodiscard]] spk::PointLightSettings &point();
		[[nodiscard]] const spk::PointLightSettings &point() const;

		[[nodiscard]] spk::SpotLightSettings &spot();
		[[nodiscard]] const spk::SpotLightSettings &spot() const;

		[[nodiscard]] spk::Color &color() noexcept;
		[[nodiscard]] const spk::Color &color() const noexcept;

		[[nodiscard]] float &intensity() noexcept;
		[[nodiscard]] float intensity() const noexcept;

		[[nodiscard]] bool &castsShadows() noexcept;
		[[nodiscard]] bool castsShadows() const noexcept;

		[[nodiscard]] int &selectionPriority() noexcept;
		[[nodiscard]] int selectionPriority() const noexcept;
	};
}
