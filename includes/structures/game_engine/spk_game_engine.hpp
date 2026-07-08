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

	class GameEngine : public spk::ActivableTrait
	{
		friend class spk::GameEngineWidget;
		friend struct spk::GameEngineTester;

	private:
		spk::UUID _id = spk::UUID::generate();
		spk::ComponentRegistry _components{_id};
		spk::ComponentLogicRegistry _logicRegistry;

		void update(const spk::UpdateContext &p_tick);
		[[nodiscard]] spk::RenderUnit buildRenderUnit();

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

		void addEntity(spk::Entity *p_entity);

		void removeEntity(spk::Entity *p_entity);
	};
}
