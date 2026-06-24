#pragma once

#include "structures/design_pattern/spk_activable_trait.hpp"

namespace spk
{
	class Entity;

	class Component : public spk::ActivableTrait
	{
		friend class Entity;

	private:
		spk::Entity *_entity = nullptr;

		void _attach(spk::Entity *p_entity);
		void _detach();

	protected:
		Component();

	public:
		~Component() override;

		Component(const Component &) = delete;
		Component &operator=(const Component &) = delete;

		Component(Component &&) noexcept = delete;
		Component &operator=(Component &&) noexcept = delete;

		[[nodiscard]] spk::Entity *entity();
		[[nodiscard]] const spk::Entity *entity() const;
		[[nodiscard]] bool hasEntity() const;
		[[nodiscard]] bool isProcessable() const;
	};
}
