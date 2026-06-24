#pragma once

#include <concepts>
#include <memory>
#include <utility>
#include <vector>

#include "structures/design_pattern/spk_activable_trait.hpp"
#include "structures/game_engine/spk_component_logic_registry.hpp"
#include "structures/game_engine/spk_component_registry.hpp"
#include "structures/game_engine/spk_entity.hpp"

namespace spk
{
	class GameEngine : public spk::ActivableTrait
	{
	private:
		// Order matters: the registries must outlive the entities, because ~Entity
		// unregisters its components from _componentRegistry. Entities are therefore
		// declared last so they are destroyed first.
		spk::ComponentRegistry _componentRegistry;
		spk::ComponentLogicRegistry _logicRegistry;
		std::vector<std::unique_ptr<spk::Entity>> _entities;

	public:
		GameEngine();
		~GameEngine() override;

		GameEngine(const GameEngine &) = delete;
		GameEngine &operator=(const GameEngine &) = delete;

		GameEngine(GameEngine &&) noexcept = delete;
		GameEngine &operator=(GameEngine &&) noexcept = delete;

		[[nodiscard]] spk::ComponentRegistry &componentRegistry();
		[[nodiscard]] const spk::ComponentRegistry &componentRegistry() const;

		[[nodiscard]] spk::ComponentLogicRegistry &logicRegistry();
		[[nodiscard]] const spk::ComponentLogicRegistry &logicRegistry() const;

		template <typename TLogic, typename... TArguments>
			requires std::derived_from<TLogic, spk::IComponentLogic>
		TLogic &add(TArguments &&...p_arguments)
		{
			return _logicRegistry.add<TLogic>(std::forward<TArguments>(p_arguments)...);
		}

		template <typename TLogic>
			requires std::derived_from<TLogic, spk::IComponentLogic>
		[[nodiscard]] TLogic *logic()
		{
			return _logicRegistry.get<TLogic>();
		}

		template <typename TLogic>
			requires std::derived_from<TLogic, spk::IComponentLogic>
		[[nodiscard]] const TLogic *logic() const
		{
			return _logicRegistry.get<TLogic>();
		}

		template <typename TLogic>
			requires std::derived_from<TLogic, spk::IComponentLogic>
		[[nodiscard]] TLogic &requireLogic()
		{
			TLogic *result = _logicRegistry.get<TLogic>();

			if (result == nullptr)
			{
				throw std::runtime_error("Requested logic does not exist");
			}

			return *result;
		}

		template <typename TEntity, typename... TArguments>
			requires std::derived_from<TEntity, spk::Entity>
		TEntity &addEntity(TArguments &&...p_arguments)
		{
			auto entity = std::make_unique<TEntity>(std::forward<TArguments>(p_arguments)...);
			TEntity &result = *entity;
			_entities.push_back(std::move(entity));

			// Incrementally register this entity's (and its sub-tree's) components.
			result._setRegistry(&_componentRegistry);

			return result;
		}

		void destroyEntity(spk::Entity &p_entity);
		void clearEntities();

		[[nodiscard]] const std::vector<std::unique_ptr<spk::Entity>> &entities() const;

		void update(const spk::UpdateTick &p_tick);
		void synchronize();
		void render(spk::RenderUnitBuilder &p_builder);

		// One template replaces the former 18 dispatchEvent overloads.
		template <typename TEvent>
		void dispatchEvent(TEvent &p_event)
		{
			if (isActivated() == false)
			{
				return;
			}

			_componentRegistry.refreshProcessableIfDirty();
			_logicRegistry.dispatchEvent(p_event, _componentRegistry);
		}
	};
}
