#pragma once

#include <algorithm>
#include <concepts>
#include <memory>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <vector>

#include "structures/design_pattern/spk_contract_provider.hpp"
#include "structures/game_engine/spk_component_logic.hpp"
#include "structures/game_engine/spk_component_registry.hpp"

namespace spk
{
	// Owns the engine's logics and runs them, highest priority first.
	// Logics are added explicitly (engine.add<TLogic>()); there is no global catalog.
	class ComponentLogicRegistry
	{
	private:
		std::vector<std::unique_ptr<spk::IComponentLogic>> _logics;
		std::unordered_map<std::type_index, spk::IComponentLogic *> _lookup;
		std::vector<spk::PriorizableTrait::Contract> _priorityContracts;
		bool _orderDirty = false;

		void _ensureSorted()
		{
			if (_orderDirty == false)
			{
				return;
			}

			// Higher priority runs first; stable so equal priorities keep insertion order.
			std::stable_sort(
				_logics.begin(),
				_logics.end(),
				[](const std::unique_ptr<spk::IComponentLogic> &p_lhs, const std::unique_ptr<spk::IComponentLogic> &p_rhs) {
					return p_lhs->priority() > p_rhs->priority();
				});

			_orderDirty = false;
		}

	public:
		ComponentLogicRegistry() = default;
		~ComponentLogicRegistry() = default;

		ComponentLogicRegistry(const ComponentLogicRegistry &) = delete;
		ComponentLogicRegistry &operator=(const ComponentLogicRegistry &) = delete;

		ComponentLogicRegistry(ComponentLogicRegistry &&) noexcept = default;
		ComponentLogicRegistry &operator=(ComponentLogicRegistry &&) noexcept = default;

		void clear()
		{
			_priorityContracts.clear();
			_lookup.clear();
			_logics.clear();
			_orderDirty = false;
		}

		template <typename TLogic, typename... TArguments>
			requires std::derived_from<TLogic, spk::IComponentLogic>
		TLogic &add(TArguments &&...p_arguments)
		{
			const std::type_index typeIndex = std::type_index(typeid(TLogic));
			auto iterator = _lookup.find(typeIndex);

			if (iterator != _lookup.end())
			{
				return static_cast<TLogic &>(*iterator->second);
			}

			auto logic = std::make_unique<TLogic>(std::forward<TArguments>(p_arguments)...);
			TLogic &result = *logic;

			_priorityContracts.push_back(result.subscribeToPriorityChange([this]() { _orderDirty = true; }));
			_lookup.emplace(typeIndex, logic.get());
			_logics.push_back(std::move(logic));
			_orderDirty = true;

			return result;
		}

		template <typename TLogic>
			requires std::derived_from<TLogic, spk::IComponentLogic>
		[[nodiscard]] TLogic *get()
		{
			auto iterator = _lookup.find(std::type_index(typeid(TLogic)));
			return (iterator == _lookup.end() ? nullptr : static_cast<TLogic *>(iterator->second));
		}

		template <typename TLogic>
			requires std::derived_from<TLogic, spk::IComponentLogic>
		[[nodiscard]] const TLogic *get() const
		{
			auto iterator = _lookup.find(std::type_index(typeid(TLogic)));
			return (iterator == _lookup.end() ? nullptr : static_cast<const TLogic *>(iterator->second));
		}

		void update(const spk::UpdateTick &p_tick, spk::ComponentRegistry &p_registry)
		{
			_ensureSorted();

			for (const std::unique_ptr<spk::IComponentLogic> &logic : _logics)
			{
				if (logic != nullptr && logic->isActivated() == true)
				{
					logic->onUpdate(p_tick, p_registry);
				}
			}
		}

		void render(
			spk::RenderUnitBuilder &p_builder,
			spk::ComponentRegistry &p_registry)
		{
			_ensureSorted();

			for (const std::unique_ptr<spk::IComponentLogic> &logic : _logics)
			{
				if (logic != nullptr && logic->isActivated() == true)
				{
					logic->onRender(p_builder, p_registry);
				}
			}
		}

		// One template replaces the per-event dispatch loops: overload resolution on
		// IComponentLogic::onEvent selects the right virtual for TEvent.
		template <typename TEvent>
		void dispatchEvent(TEvent &p_event, spk::ComponentRegistry &p_registry)
		{
			_ensureSorted();

			for (const std::unique_ptr<spk::IComponentLogic> &logic : _logics)
			{
				if (logic == nullptr || logic->isActivated() == false || p_event.isConsumed() == true)
				{
					continue;
				}

				logic->onEvent(p_event, p_registry);
			}
		}
	};
}
