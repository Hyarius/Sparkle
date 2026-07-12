#include "structures/game_engine/spk_light_3d.hpp"

#include <variant>

namespace
{
	template <typename TSettings>
	[[nodiscard]]
	TSettings &initializeOrGet(spk::LightShape &p_shape)
	{
		if (std::holds_alternative<std::monostate>(p_shape))
		{
			p_shape.emplace<TSettings>();
		}

		return std::get<TSettings>(p_shape);
	}
}

namespace spk
{
	bool Light3D::hasType() const noexcept
	{
		return std::holds_alternative<std::monostate>(_shape) == false;
	}

	bool Light3D::isDirectional() const noexcept
	{
		return std::holds_alternative<spk::DirectionalLightSettings>(_shape);
	}

	bool Light3D::isPoint() const noexcept
	{
		return std::holds_alternative<spk::PointLightSettings>(_shape);
	}

	bool Light3D::isSpot() const noexcept
	{
		return std::holds_alternative<spk::SpotLightSettings>(_shape);
	}

	spk::DirectionalLightSettings &Light3D::directional()
	{
		return initializeOrGet<spk::DirectionalLightSettings>(_shape);
	}

	const spk::DirectionalLightSettings &Light3D::directional() const
	{
		return std::get<spk::DirectionalLightSettings>(_shape);
	}

	spk::PointLightSettings &Light3D::point()
	{
		return initializeOrGet<spk::PointLightSettings>(_shape);
	}

	const spk::PointLightSettings &Light3D::point() const
	{
		return std::get<spk::PointLightSettings>(_shape);
	}

	spk::SpotLightSettings &Light3D::spot()
	{
		return initializeOrGet<spk::SpotLightSettings>(_shape);
	}

	const spk::SpotLightSettings &Light3D::spot() const
	{
		return std::get<spk::SpotLightSettings>(_shape);
	}

	spk::Color &Light3D::color() noexcept
	{
		return _color;
	}

	const spk::Color &Light3D::color() const noexcept
	{
		return _color;
	}

	float &Light3D::intensity() noexcept
	{
		return _intensity;
	}

	float Light3D::intensity() const noexcept
	{
		return _intensity;
	}

	bool &Light3D::castsShadows() noexcept
	{
		return _castsShadows;
	}

	bool Light3D::castsShadows() const noexcept
	{
		return _castsShadows;
	}

	int &Light3D::selectionPriority() noexcept
	{
		return _selectionPriority;
	}

	int Light3D::selectionPriority() const noexcept
	{
		return _selectionPriority;
	}
}