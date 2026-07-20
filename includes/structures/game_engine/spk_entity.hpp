#pragma once

#include <concepts>
#include <memory>
#include <stdexcept>
#include <typeindex>
#include <typeinfo>
#include <utility>
#include <vector>

#include "structures/container/spk_uuid.hpp"
#include "structures/design_pattern/spk_activable_trait.hpp"
#include "structures/design_pattern/spk_inherence_trait.hpp"
#include "structures/game_engine/spk_component.hpp"
#include "structures/game_engine/spk_component_store.hpp"

namespace spk
{
	class GameEngine;

	class Entity : public spk::InherenceTrait<spk::Entity>, public spk::ActivableTrait
	{
		friend class spk::GameEngine;

	private:
		spk::UUID _uuid;
		std::vector<std::unique_ptr<spk::Component>> _components;

		spk::UUID _engineId;

		bool _globalActivated = false;

		spk::ActivableTrait::Contract _activationContract;
		spk::ActivableTrait::Contract _deactivationContract;

		void _refreshGlobalActivated();

		void _setEngineId(const spk::UUID &p_engineId);

	protected:
		void _onParentChanged(spk::Entity *p_oldParent, spk::Entity *p_newParent) noexcept override;

	public:
		explicit Entity(spk::Entity *p_parent = nullptr);
		explicit Entity(const spk::UUID &p_uuid, spk::Entity *p_parent = nullptr);
		~Entity() override;

		Entity(const Entity &) = delete;
		Entity &operator=(const Entity &) = delete;

		Entity(Entity &&) noexcept = delete;
		Entity &operator=(Entity &&) noexcept = delete;

		[[nodiscard]] const spk::UUID &uuid() const;
		[[nodiscard]] bool isHierarchyActivated() const;

		[[nodiscard]] const std::vector<std::unique_ptr<spk::Component>> &components() const;

		template <typename TComponent, typename... TArguments>
			requires std::derived_from<TComponent, spk::Component>
		TComponent &addComponent(TArguments &&...p_arguments)
		{
			auto component = std::make_unique<TComponent>(std::forward<TArguments>(p_arguments)...);
			component->_attach(this);

			TComponent &result = *component;
			spk::Component *raw = component.get();
			_components.push_back(std::move(component));

			if (_engineId.isNull() == false)
			{
				spk::ComponentStore::instance().add(std::type_index(typeid(*raw)), _engineId, raw);
			}

			return result;
		}

		void removeComponent(spk::Component &p_component);

		template <typename TComponent>
			requires std::derived_from<TComponent, spk::Component>
		void removeComponent()
		{
			TComponent *found = component<TComponent>();

			if (found != nullptr)
			{
				removeComponent(*found);
			}
		}

		void clearComponents();

		template <typename TComponent>
			requires std::derived_from<TComponent, spk::Component>
		[[nodiscard]] TComponent *component()
		{
			for (const std::unique_ptr<spk::Component> &entry : _components)
			{
				if (TComponent *typed = dynamic_cast<TComponent *>(entry.get()); typed != nullptr)
				{
					return typed;
				}
			}

			return nullptr;
		}

		template <typename TComponent>
			requires std::derived_from<TComponent, spk::Component>
		[[nodiscard]] const TComponent *component() const
		{
			for (const std::unique_ptr<spk::Component> &entry : _components)
			{
				if (const TComponent *typed = dynamic_cast<const TComponent *>(entry.get()); typed != nullptr)
				{
					return typed;
				}
			}

			return nullptr;
		}

		template <typename TComponent>
			requires std::derived_from<TComponent, spk::Component>
		[[nodiscard]] TComponent &requireComponent()
		{
			TComponent *result = component<TComponent>();

			if (result == nullptr)
			{
				throw std::runtime_error("Requested component is missing from entity [" + _uuid.toString() + "]");
			}

			return *result;
		}

		template <typename TComponent>
			requires std::derived_from<TComponent, spk::Component>
		[[nodiscard]] const TComponent &requireComponent() const
		{
			const TComponent *result = component<TComponent>();

			if (result == nullptr)
			{
				throw std::runtime_error("Requested component is missing from entity [" + _uuid.toString() + "]");
			}

			return *result;
		}
	};
}
