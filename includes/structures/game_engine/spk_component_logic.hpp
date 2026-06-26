#pragma once

#include <concepts>
#include <vector>

#include "structures/design_pattern/spk_activable_trait.hpp"
#include "structures/design_pattern/spk_priorizable_trait.hpp"
#include "structures/game_engine/spk_component.hpp"
#include "structures/game_engine/spk_component_registry.hpp"
#include "structures/graphics/rendering/unit/spk_render_unit_builder.hpp"
#include "structures/system/event/spk_events.hpp"
#include "structures/system/event/spk_update_tick.hpp"

namespace spk
{
	class ComponentLogicRegistry;

	class IComponentLogic : public spk::ActivableTrait, public spk::PriorizableTrait
	{
		friend class spk::ComponentLogicRegistry;

	private:
		virtual void onUpdate(const spk::UpdateTick &p_tick, spk::ComponentRegistry &p_registry) = 0;
		virtual void onRender(
			spk::RenderUnitBuilder &p_builder,
			spk::ComponentRegistry &p_registry) = 0;

		virtual void onEvent(spk::WindowCloseRequestedEvent &p_event, spk::ComponentRegistry &p_registry) = 0;
		virtual void onEvent(spk::WindowDestroyedEvent &p_event, spk::ComponentRegistry &p_registry) = 0;
		virtual void onEvent(spk::WindowMovedEvent &p_event, spk::ComponentRegistry &p_registry) = 0;
		virtual void onEvent(spk::WindowResizedEvent &p_event, spk::ComponentRegistry &p_registry) = 0;
		virtual void onEvent(spk::WindowFocusGainedEvent &p_event, spk::ComponentRegistry &p_registry) = 0;
		virtual void onEvent(spk::WindowFocusLostEvent &p_event, spk::ComponentRegistry &p_registry) = 0;
		virtual void onEvent(spk::WindowShownEvent &p_event, spk::ComponentRegistry &p_registry) = 0;
		virtual void onEvent(spk::WindowHiddenEvent &p_event, spk::ComponentRegistry &p_registry) = 0;
		virtual void onEvent(spk::MouseEnteredWindowEvent &p_event, spk::ComponentRegistry &p_registry) = 0;
		virtual void onEvent(spk::MouseLeftWindowEvent &p_event, spk::ComponentRegistry &p_registry) = 0;
		virtual void onEvent(spk::MouseMovedEvent &p_event, spk::ComponentRegistry &p_registry) = 0;
		virtual void onEvent(spk::MouseWheelScrolledEvent &p_event, spk::ComponentRegistry &p_registry) = 0;
		virtual void onEvent(spk::MouseButtonPressedEvent &p_event, spk::ComponentRegistry &p_registry) = 0;
		virtual void onEvent(spk::MouseButtonReleasedEvent &p_event, spk::ComponentRegistry &p_registry) = 0;
		virtual void onEvent(spk::MouseButtonDoubleClickedEvent &p_event, spk::ComponentRegistry &p_registry) = 0;
		virtual void onEvent(spk::KeyPressedEvent &p_event, spk::ComponentRegistry &p_registry) = 0;
		virtual void onEvent(spk::KeyReleasedEvent &p_event, spk::ComponentRegistry &p_registry) = 0;
		virtual void onEvent(spk::TextInputEvent &p_event, spk::ComponentRegistry &p_registry) = 0;

	protected:
		IComponentLogic();

	public:
		~IComponentLogic() override;

		IComponentLogic(const IComponentLogic &) = delete;
		IComponentLogic &operator=(const IComponentLogic &) = delete;

		IComponentLogic(IComponentLogic &&) noexcept = delete;
		IComponentLogic &operator=(IComponentLogic &&) noexcept = delete;
	};

	template <typename TComponent>
		requires std::derived_from<TComponent, spk::Component>
	class ComponentLogic : public spk::IComponentLogic
	{
	public:
		using ComponentType = TComponent;

	private:
		template <typename TEvent>
		void _dispatchEvent(
			TEvent &p_event,
			spk::ComponentRegistry &p_registry,
			void (spk::ComponentLogic<TComponent>::*p_started)(TEvent &),
			void (spk::ComponentLogic<TComponent>::*p_parse)(TEvent &, TComponent &),
			void (spk::ComponentLogic<TComponent>::*p_execute)(TEvent &))
		{
			if (isActivated() == false || p_event.isConsumed() == true)
			{
				return;
			}

			(this->*p_started)(p_event);

			const std::vector<spk::Component *> &components = p_registry.components<TComponent>();

			for (spk::Component *component : components)
			{
				if (component == nullptr || component->isProcessable() == false || p_event.isConsumed() == true)
				{
					continue;
				}

				(this->*p_parse)(p_event, *static_cast<TComponent *>(component));
			}

			(this->*p_execute)(p_event);
		}

		void onUpdate(const spk::UpdateTick &p_tick, spk::ComponentRegistry &p_registry) final
		{
			if (isActivated() == false)
			{
				return;
			}

			_onUpdateStarted(p_tick);

			const std::vector<spk::Component *> &components = p_registry.components<TComponent>();

			for (spk::Component *component : components)
			{
				if (component != nullptr && component->isProcessable() == true)
				{
					_parseComponentForUpdate(p_tick, *static_cast<TComponent *>(component));
				}
			}

			_executeUpdate(p_tick);
		}

		void onRender(
			spk::RenderUnitBuilder &p_builder,
			spk::ComponentRegistry &p_registry) final
		{
			if (isActivated() == false)
			{
				return;
			}

			_onRenderStarted();

			const std::vector<spk::Component *> &components = p_registry.components<TComponent>();

			for (spk::Component *component : components)
			{
				if (component != nullptr && component->isProcessable() == true)
				{
					_parseComponentForRender(*static_cast<TComponent *>(component));
				}
			}

			_executeRender(p_builder);
		}

		void onEvent(spk::WindowCloseRequestedEvent &p_event, spk::ComponentRegistry &p_registry) final
		{
			_dispatchEvent(p_event, p_registry,
				&ComponentLogic::_onWindowCloseRequestedEventStarted,
				&ComponentLogic::_parseComponentForWindowCloseRequestedEvent,
				&ComponentLogic::_executeWindowCloseRequestedEvent);
		}

		void onEvent(spk::WindowDestroyedEvent &p_event, spk::ComponentRegistry &p_registry) final
		{
			_dispatchEvent(p_event, p_registry,
				&ComponentLogic::_onWindowDestroyedEventStarted,
				&ComponentLogic::_parseComponentForWindowDestroyedEvent,
				&ComponentLogic::_executeWindowDestroyedEvent);
		}

		void onEvent(spk::WindowMovedEvent &p_event, spk::ComponentRegistry &p_registry) final
		{
			_dispatchEvent(p_event, p_registry,
				&ComponentLogic::_onWindowMovedEventStarted,
				&ComponentLogic::_parseComponentForWindowMovedEvent,
				&ComponentLogic::_executeWindowMovedEvent);
		}

		void onEvent(spk::WindowResizedEvent &p_event, spk::ComponentRegistry &p_registry) final
		{
			_dispatchEvent(p_event, p_registry,
				&ComponentLogic::_onWindowResizedEventStarted,
				&ComponentLogic::_parseComponentForWindowResizedEvent,
				&ComponentLogic::_executeWindowResizedEvent);
		}

		void onEvent(spk::WindowFocusGainedEvent &p_event, spk::ComponentRegistry &p_registry) final
		{
			_dispatchEvent(p_event, p_registry,
				&ComponentLogic::_onWindowFocusGainedEventStarted,
				&ComponentLogic::_parseComponentForWindowFocusGainedEvent,
				&ComponentLogic::_executeWindowFocusGainedEvent);
		}

		void onEvent(spk::WindowFocusLostEvent &p_event, spk::ComponentRegistry &p_registry) final
		{
			_dispatchEvent(p_event, p_registry,
				&ComponentLogic::_onWindowFocusLostEventStarted,
				&ComponentLogic::_parseComponentForWindowFocusLostEvent,
				&ComponentLogic::_executeWindowFocusLostEvent);
		}

		void onEvent(spk::WindowShownEvent &p_event, spk::ComponentRegistry &p_registry) final
		{
			_dispatchEvent(p_event, p_registry,
				&ComponentLogic::_onWindowShownEventStarted,
				&ComponentLogic::_parseComponentForWindowShownEvent,
				&ComponentLogic::_executeWindowShownEvent);
		}

		void onEvent(spk::WindowHiddenEvent &p_event, spk::ComponentRegistry &p_registry) final
		{
			_dispatchEvent(p_event, p_registry,
				&ComponentLogic::_onWindowHiddenEventStarted,
				&ComponentLogic::_parseComponentForWindowHiddenEvent,
				&ComponentLogic::_executeWindowHiddenEvent);
		}

		void onEvent(spk::MouseEnteredWindowEvent &p_event, spk::ComponentRegistry &p_registry) final
		{
			_dispatchEvent(p_event, p_registry,
				&ComponentLogic::_onMouseEnteredEventStarted,
				&ComponentLogic::_parseComponentForMouseEnteredEvent,
				&ComponentLogic::_executeMouseEnteredEvent);
		}

		void onEvent(spk::MouseLeftWindowEvent &p_event, spk::ComponentRegistry &p_registry) final
		{
			_dispatchEvent(p_event, p_registry,
				&ComponentLogic::_onMouseLeftEventStarted,
				&ComponentLogic::_parseComponentForMouseLeftEvent,
				&ComponentLogic::_executeMouseLeftEvent);
		}

		void onEvent(spk::MouseMovedEvent &p_event, spk::ComponentRegistry &p_registry) final
		{
			_dispatchEvent(p_event, p_registry,
				&ComponentLogic::_onMouseMovedEventStarted,
				&ComponentLogic::_parseComponentForMouseMovedEvent,
				&ComponentLogic::_executeMouseMovedEvent);
		}

		void onEvent(spk::MouseWheelScrolledEvent &p_event, spk::ComponentRegistry &p_registry) final
		{
			_dispatchEvent(p_event, p_registry,
				&ComponentLogic::_onMouseWheelScrolledEventStarted,
				&ComponentLogic::_parseComponentForMouseWheelScrolledEvent,
				&ComponentLogic::_executeMouseWheelScrolledEvent);
		}

		void onEvent(spk::MouseButtonPressedEvent &p_event, spk::ComponentRegistry &p_registry) final
		{
			_dispatchEvent(p_event, p_registry,
				&ComponentLogic::_onMouseButtonPressedEventStarted,
				&ComponentLogic::_parseComponentForMouseButtonPressedEvent,
				&ComponentLogic::_executeMouseButtonPressedEvent);
		}

		void onEvent(spk::MouseButtonReleasedEvent &p_event, spk::ComponentRegistry &p_registry) final
		{
			_dispatchEvent(p_event, p_registry,
				&ComponentLogic::_onMouseButtonReleasedEventStarted,
				&ComponentLogic::_parseComponentForMouseButtonReleasedEvent,
				&ComponentLogic::_executeMouseButtonReleasedEvent);
		}

		void onEvent(spk::MouseButtonDoubleClickedEvent &p_event, spk::ComponentRegistry &p_registry) final
		{
			_dispatchEvent(p_event, p_registry,
				&ComponentLogic::_onMouseButtonDoubleClickedEventStarted,
				&ComponentLogic::_parseComponentForMouseButtonDoubleClickedEvent,
				&ComponentLogic::_executeMouseButtonDoubleClickedEvent);
		}

		void onEvent(spk::KeyPressedEvent &p_event, spk::ComponentRegistry &p_registry) final
		{
			_dispatchEvent(p_event, p_registry,
				&ComponentLogic::_onKeyPressedEventStarted,
				&ComponentLogic::_parseComponentForKeyPressedEvent,
				&ComponentLogic::_executeKeyPressedEvent);
		}

		void onEvent(spk::KeyReleasedEvent &p_event, spk::ComponentRegistry &p_registry) final
		{
			_dispatchEvent(p_event, p_registry,
				&ComponentLogic::_onKeyReleasedEventStarted,
				&ComponentLogic::_parseComponentForKeyReleasedEvent,
				&ComponentLogic::_executeKeyReleasedEvent);
		}

		void onEvent(spk::TextInputEvent &p_event, spk::ComponentRegistry &p_registry) final
		{
			_dispatchEvent(p_event, p_registry,
				&ComponentLogic::_onTextInputEventStarted,
				&ComponentLogic::_parseComponentForTextInputEvent,
				&ComponentLogic::_executeTextInputEvent);
		}

	protected:
		virtual void _onUpdateStarted(const spk::UpdateTick &p_tick) { (void)p_tick; }
		virtual void _parseComponentForUpdate(const spk::UpdateTick &p_tick, TComponent &p_component) { (void)p_tick; (void)p_component; }
		virtual void _executeUpdate(const spk::UpdateTick &p_tick) { (void)p_tick; }

		virtual void _onRenderStarted() {}
		virtual void _parseComponentForRender(TComponent &p_component) { (void)p_component; }
		virtual void _executeRender(spk::RenderUnitBuilder &p_builder) { (void)p_builder; }

		virtual void _onWindowCloseRequestedEventStarted(spk::WindowCloseRequestedEvent &p_event) { (void)p_event; }
		virtual void _parseComponentForWindowCloseRequestedEvent(spk::WindowCloseRequestedEvent &p_event, TComponent &p_component) { (void)p_event; (void)p_component; }
		virtual void _executeWindowCloseRequestedEvent(spk::WindowCloseRequestedEvent &p_event) { (void)p_event; }

		virtual void _onWindowDestroyedEventStarted(spk::WindowDestroyedEvent &p_event) { (void)p_event; }
		virtual void _parseComponentForWindowDestroyedEvent(spk::WindowDestroyedEvent &p_event, TComponent &p_component) { (void)p_event; (void)p_component; }
		virtual void _executeWindowDestroyedEvent(spk::WindowDestroyedEvent &p_event) { (void)p_event; }

		virtual void _onWindowMovedEventStarted(spk::WindowMovedEvent &p_event) { (void)p_event; }
		virtual void _parseComponentForWindowMovedEvent(spk::WindowMovedEvent &p_event, TComponent &p_component) { (void)p_event; (void)p_component; }
		virtual void _executeWindowMovedEvent(spk::WindowMovedEvent &p_event) { (void)p_event; }

		virtual void _onWindowResizedEventStarted(spk::WindowResizedEvent &p_event) { (void)p_event; }
		virtual void _parseComponentForWindowResizedEvent(spk::WindowResizedEvent &p_event, TComponent &p_component) { (void)p_event; (void)p_component; }
		virtual void _executeWindowResizedEvent(spk::WindowResizedEvent &p_event) { (void)p_event; }

		virtual void _onWindowFocusGainedEventStarted(spk::WindowFocusGainedEvent &p_event) { (void)p_event; }
		virtual void _parseComponentForWindowFocusGainedEvent(spk::WindowFocusGainedEvent &p_event, TComponent &p_component) { (void)p_event; (void)p_component; }
		virtual void _executeWindowFocusGainedEvent(spk::WindowFocusGainedEvent &p_event) { (void)p_event; }

		virtual void _onWindowFocusLostEventStarted(spk::WindowFocusLostEvent &p_event) { (void)p_event; }
		virtual void _parseComponentForWindowFocusLostEvent(spk::WindowFocusLostEvent &p_event, TComponent &p_component) { (void)p_event; (void)p_component; }
		virtual void _executeWindowFocusLostEvent(spk::WindowFocusLostEvent &p_event) { (void)p_event; }

		virtual void _onWindowShownEventStarted(spk::WindowShownEvent &p_event) { (void)p_event; }
		virtual void _parseComponentForWindowShownEvent(spk::WindowShownEvent &p_event, TComponent &p_component) { (void)p_event; (void)p_component; }
		virtual void _executeWindowShownEvent(spk::WindowShownEvent &p_event) { (void)p_event; }

		virtual void _onWindowHiddenEventStarted(spk::WindowHiddenEvent &p_event) { (void)p_event; }
		virtual void _parseComponentForWindowHiddenEvent(spk::WindowHiddenEvent &p_event, TComponent &p_component) { (void)p_event; (void)p_component; }
		virtual void _executeWindowHiddenEvent(spk::WindowHiddenEvent &p_event) { (void)p_event; }

		virtual void _onMouseEnteredEventStarted(spk::MouseEnteredWindowEvent &p_event) { (void)p_event; }
		virtual void _parseComponentForMouseEnteredEvent(spk::MouseEnteredWindowEvent &p_event, TComponent &p_component) { (void)p_event; (void)p_component; }
		virtual void _executeMouseEnteredEvent(spk::MouseEnteredWindowEvent &p_event) { (void)p_event; }

		virtual void _onMouseLeftEventStarted(spk::MouseLeftWindowEvent &p_event) { (void)p_event; }
		virtual void _parseComponentForMouseLeftEvent(spk::MouseLeftWindowEvent &p_event, TComponent &p_component) { (void)p_event; (void)p_component; }
		virtual void _executeMouseLeftEvent(spk::MouseLeftWindowEvent &p_event) { (void)p_event; }

		virtual void _onMouseMovedEventStarted(spk::MouseMovedEvent &p_event) { (void)p_event; }
		virtual void _parseComponentForMouseMovedEvent(spk::MouseMovedEvent &p_event, TComponent &p_component) { (void)p_event; (void)p_component; }
		virtual void _executeMouseMovedEvent(spk::MouseMovedEvent &p_event) { (void)p_event; }

		virtual void _onMouseWheelScrolledEventStarted(spk::MouseWheelScrolledEvent &p_event) { (void)p_event; }
		virtual void _parseComponentForMouseWheelScrolledEvent(spk::MouseWheelScrolledEvent &p_event, TComponent &p_component) { (void)p_event; (void)p_component; }
		virtual void _executeMouseWheelScrolledEvent(spk::MouseWheelScrolledEvent &p_event) { (void)p_event; }

		virtual void _onMouseButtonPressedEventStarted(spk::MouseButtonPressedEvent &p_event) { (void)p_event; }
		virtual void _parseComponentForMouseButtonPressedEvent(spk::MouseButtonPressedEvent &p_event, TComponent &p_component) { (void)p_event; (void)p_component; }
		virtual void _executeMouseButtonPressedEvent(spk::MouseButtonPressedEvent &p_event) { (void)p_event; }

		virtual void _onMouseButtonReleasedEventStarted(spk::MouseButtonReleasedEvent &p_event) { (void)p_event; }
		virtual void _parseComponentForMouseButtonReleasedEvent(spk::MouseButtonReleasedEvent &p_event, TComponent &p_component) { (void)p_event; (void)p_component; }
		virtual void _executeMouseButtonReleasedEvent(spk::MouseButtonReleasedEvent &p_event) { (void)p_event; }

		virtual void _onMouseButtonDoubleClickedEventStarted(spk::MouseButtonDoubleClickedEvent &p_event) { (void)p_event; }
		virtual void _parseComponentForMouseButtonDoubleClickedEvent(spk::MouseButtonDoubleClickedEvent &p_event, TComponent &p_component) { (void)p_event; (void)p_component; }
		virtual void _executeMouseButtonDoubleClickedEvent(spk::MouseButtonDoubleClickedEvent &p_event) { (void)p_event; }

		virtual void _onKeyPressedEventStarted(spk::KeyPressedEvent &p_event) { (void)p_event; }
		virtual void _parseComponentForKeyPressedEvent(spk::KeyPressedEvent &p_event, TComponent &p_component) { (void)p_event; (void)p_component; }
		virtual void _executeKeyPressedEvent(spk::KeyPressedEvent &p_event) { (void)p_event; }

		virtual void _onKeyReleasedEventStarted(spk::KeyReleasedEvent &p_event) { (void)p_event; }
		virtual void _parseComponentForKeyReleasedEvent(spk::KeyReleasedEvent &p_event, TComponent &p_component) { (void)p_event; (void)p_component; }
		virtual void _executeKeyReleasedEvent(spk::KeyReleasedEvent &p_event) { (void)p_event; }

		virtual void _onTextInputEventStarted(spk::TextInputEvent &p_event) { (void)p_event; }
		virtual void _parseComponentForTextInputEvent(spk::TextInputEvent &p_event, TComponent &p_component) { (void)p_event; (void)p_component; }
		virtual void _executeTextInputEvent(spk::TextInputEvent &p_event) { (void)p_event; }
	};
}
