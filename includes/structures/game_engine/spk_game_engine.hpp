#pragma once

#include <concepts>
#include <stdexcept>
#include <utility>

#include "structures/container/spk_uuid.hpp"
#include "structures/design_pattern/spk_activable_trait.hpp"
#include "structures/game_engine/spk_component_logic_registry.hpp"
#include "structures/game_engine/spk_component_registry.hpp"
#include "structures/game_engine/spk_entity.hpp"
#include "structures/graphics/rendering/unit/spk_render_unit_builder.hpp"

namespace spk
{
	class GameEngineWidget;
	struct GameEngineTester;

	// An engine is identified by a UUID and is a VIEW over the process-wide
	// ComponentStore: it processes the components whose entity is registered with its
	// UUID. The caller creates an spk::Entity and registers it:
	//
	//     spk::GameEngine engine;
	//     spk::Entity     player;
	//     player.addComponent<...>();
	//     engine.addEntity(&player);
	//
	// addEntity stamps the entity's sub-tree with the engine UUID (its components move
	// into store[type][engineId]); re-parenting between engines migrates them; destroying
	// the engine drops its buckets. Activation is handled per-entity by the dual-state
	// flag, so it never touches the store.
	//
	// The phase methods (update / synchronize / buildRenderUnit / dispatchEvent) are
	// private: the engine is only ever driven by its owning GameEngineWidget.
	class GameEngine : public spk::ActivableTrait
	{
		friend class spk::GameEngineWidget;
		friend struct spk::GameEngineTester;

	private:
		spk::UUID _id = spk::UUID::generate();
		spk::ComponentRegistry _components{_id};
		spk::ComponentLogicRegistry _logicRegistry;

		void update(const spk::UpdateTick &p_tick);
		[[nodiscard]] spk::RenderUnit buildRenderUnit();

		// One template replaces the former 18 dispatchEvent overloads.
		template <typename TEvent>
		void dispatchEvent(TEvent &p_event)
		{
			if (isActivated() == false)
			{
				return;
			}

			_logicRegistry.dispatchEvent(p_event, _components);
		}

	public:
		GameEngine();
		~GameEngine() override;

		GameEngine(const GameEngine &) = delete;
		GameEngine &operator=(const GameEngine &) = delete;

		GameEngine(GameEngine &&) noexcept = delete;
		GameEngine &operator=(GameEngine &&) noexcept = delete;

		[[nodiscard]] const spk::UUID &id() const;

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

		// Stamp an externally-owned entity's sub-tree with this engine's UUID, wiring its
		// components into the store. No-op if p_entity is null.
		void addEntity(spk::Entity *p_entity);

		// Unstamp an entity previously registered here; the caller keeps ownership.
		// No-op if p_entity is null or was not registered with this engine.
		void removeEntity(spk::Entity *p_entity);
	};
}
