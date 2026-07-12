#pragma once

#include <algorithm>
#include <array>
#include <concepts>
#include <memory>
#include <stdexcept>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <vector>

#include "structures/game_engine/spk_camera_3d.hpp"
#include "structures/game_engine/spk_component_logic.hpp"
#include "structures/game_engine/spk_component_registry.hpp"
#include "structures/graphics/rendering/command/spk_camera_update_render_command.hpp"
#include "structures/graphics/rendering/command/spk_directional_light_update_render_command.hpp"
#include "structures/graphics/rendering/pipeline/spk_render_plan.hpp"
#include "structures/graphics/spk_directional_light.hpp"

namespace spk
{
	class ComponentLogicRegistry
	{
	private:
		static constexpr std::size_t PhaseCount = static_cast<std::size_t>(spk::RenderPhase::Count);

		std::vector<std::unique_ptr<spk::IComponentLogic>> _logics;
		std::unordered_map<std::type_index, spk::IComponentLogic *> _lookup;
		std::vector<spk::IComponentLogic *> _updateEventOrder;
		std::array<std::vector<spk::IComponentLogic *>, PhaseCount> _renderContributors;
		std::vector<spk::PriorizableTrait::Contract> _priorityContracts;
		std::vector<spk::IComponentLogic::RenderPriorityContract> _renderPriorityContracts;
		bool _updateEventOrderDirty = false;
		std::array<bool, PhaseCount> _renderOrderDirty{};

		void _ensureUpdateEventSorted()
		{
			if (!_updateEventOrderDirty)
			{
				return;
			}
			_updateEventOrder.clear();
			_updateEventOrder.reserve(_logics.size());
			for (const auto &logic : _logics)
			{
				_updateEventOrder.push_back(logic.get());
			}
			std::stable_sort(_updateEventOrder.begin(), _updateEventOrder.end(), [](const auto *p_lhs, const auto *p_rhs) {
				return p_lhs->priority() > p_rhs->priority();
			});
			_updateEventOrderDirty = false;
		}

		void _ensureRenderSorted(RenderPhase p_phase)
		{
			const std::size_t index = static_cast<std::size_t>(p_phase);
			if (index >= PhaseCount)
			{
				throw std::invalid_argument("Invalid render phase");
			}
			if (!_renderOrderDirty[index])
			{
				return;
			}
			auto &contributors = _renderContributors[index];
			contributors.clear();
			for (const auto &logic : _logics)
			{
				if (containsRenderPhase(logic->renderPhases(), p_phase))
				{
					contributors.push_back(logic.get());
				}
			}
			std::stable_sort(contributors.begin(), contributors.end(), [p_phase](const auto *p_lhs, const auto *p_rhs) {
				return p_lhs->renderPriority(p_phase) > p_rhs->renderPriority(p_phase);
			});
			_renderOrderDirty[index] = false;
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
			_renderPriorityContracts.clear();
			_priorityContracts.clear();
			_lookup.clear();
			_updateEventOrder.clear();
			for (auto &contributors : _renderContributors)
			{
				contributors.clear();
			}
			_logics.clear();
			_updateEventOrderDirty = false;
			_renderOrderDirty.fill(false);
		}

		template <typename TLogic, typename... TArguments>
			requires std::derived_from<TLogic, spk::IComponentLogic>
		TLogic &add(TArguments &&...p_arguments)
		{
			const std::type_index typeIndex(typeid(TLogic));
			if (auto iterator = _lookup.find(typeIndex); iterator != _lookup.end())
			{
				return static_cast<TLogic &>(*iterator->second);
			}

			auto logic = std::make_unique<TLogic>(std::forward<TArguments>(p_arguments)...);
			TLogic &result = *logic;
			const RenderPhaseMask phases = result.renderPhases();
			if ((phases & ~allRenderPhases()) != RenderPhaseMask::None)
			{
				throw std::invalid_argument("Component logic declares an invalid render phase mask");
			}

			_priorityContracts.push_back(result.subscribeToPriorityChange([this, logicPointer = logic.get()]() {
				_updateEventOrderDirty = true;
				for (std::size_t index = 0; index < PhaseCount; ++index)
				{
					if (containsRenderPhase(logicPointer->renderPhases(), static_cast<RenderPhase>(index)))
					{
						_renderOrderDirty[index] = true;
					}
				}
			}));
			_renderPriorityContracts.push_back(result.subscribeToRenderPriorityChange([this](RenderPhase p_phase) {
				if (p_phase < RenderPhase::Count)
				{
					_renderOrderDirty[static_cast<std::size_t>(p_phase)] = true;
				}
			}));

			_lookup.emplace(typeIndex, logic.get());
			_logics.push_back(std::move(logic));
			_updateEventOrderDirty = true;
			for (std::size_t index = 0; index < PhaseCount; ++index)
			{
				if (containsRenderPhase(phases, static_cast<RenderPhase>(index)))
				{
					_renderOrderDirty[index] = true;
				}
			}
			return result;
		}

		template <typename TLogic>
			requires std::derived_from<TLogic, spk::IComponentLogic>
		[[nodiscard]] TLogic *get()
		{
			auto iterator = _lookup.find(std::type_index(typeid(TLogic)));
			return iterator == _lookup.end() ? nullptr : static_cast<TLogic *>(iterator->second);
		}

		template <typename TLogic>
			requires std::derived_from<TLogic, spk::IComponentLogic>
		[[nodiscard]] const TLogic *get() const
		{
			auto iterator = _lookup.find(std::type_index(typeid(TLogic)));
			return iterator == _lookup.end() ? nullptr : static_cast<const TLogic *>(iterator->second);
		}

		void update(const spk::UpdateContext &p_tick, spk::ComponentRegistry &p_registry)
		{
			_ensureUpdateEventSorted();
			for (spk::IComponentLogic *logic : _updateEventOrder)
			{
				if (logic != nullptr && logic->isActivated())
				{
					logic->onUpdate(p_tick, p_registry);
				}
			}
		}

		void renderPhase(
			const spk::RenderPhaseContext &p_context,
			spk::RenderPass &p_pass,
			spk::ComponentRegistry &p_registry)
		{
			_ensureRenderSorted(p_context.phase);
			for (spk::IComponentLogic *logic : _renderContributors[static_cast<std::size_t>(p_context.phase)])
			{
				if (logic == nullptr || !logic->isActivated())
				{
					continue;
				}
				p_pass.recordContributor(p_context.phase);
				logic->onRenderPhase(p_context, p_pass, p_registry);
			}
		}

		// Compatibility entry point for callers that already own the active target.
		// Work is still recorded and flattened by semantic phase, but no target
		// transition is emitted because this legacy API has no target description.
		void render(spk::RenderUnitBuilder &p_builder, spk::ComponentRegistry &p_registry)
		{
			const spk::Viewport viewport(spk::Rect2D(0, 0, 1, 1));
			const spk::RenderFrameRequest request{
				.mainTarget = spk::RenderTargetReference{.frameBuffer = nullptr, .viewport = viewport},
				.mainClear = {}};
			spk::RenderFrameContext frame{
				.request = request, .components = p_registry, .mainCamera = spk::Camera3D::mainCamera()};
			spk::RenderPass pass(spk::RenderPassDescription{.id = {.kind = spk::RenderPassKind::MainScene, .instance = 0, .debugName = "CompatibilityMain"}, .target = request.mainTarget, .clear = {}, .phases = {spk::RenderPhase::PassSetup, spk::RenderPhase::SceneOpaque, spk::RenderPhase::SceneTransparent, spk::RenderPhase::SceneOverlay}});
			if (frame.mainCamera != nullptr)
			{
				pass.emplace<spk::CameraUpdateRenderCommand>(
					spk::RenderPhase::PassSetup, 1u, frame.mainCamera->viewProjectionMatrix());
				pass.emplace<spk::DirectionalLightUpdateRenderCommand>(
					spk::RenderPhase::PassSetup,
					3u,
					spk::DirectionalLight{
						.direction = spk::Vector3(1.0f, -2.0f, 0.5f).normalized(),
						.color = spk::Color(1.0f, 0.95f, 0.85f),
						.ambient = 0.35f});
			}
			for (spk::RenderPhase phase : pass.description().phases)
			{
				const spk::RenderPhaseContext context{
					.frame = frame,
					.pass = pass.description(),
					.phase = phase,
					.passInstance = 0};
				renderPhase(context, pass, p_registry);
				for (auto &command : pass.takeCommands(phase))
				{
					p_builder.add(std::move(command));
				}
			}
		}

		template <typename TEvent>
		void dispatchEvent(TEvent &p_event, spk::ComponentRegistry &p_registry)
		{
			_ensureUpdateEventSorted();
			for (spk::IComponentLogic *logic : _updateEventOrder)
			{
				if (logic == nullptr || !logic->isActivated() || p_event.isConsumed())
				{
					continue;
				}
				logic->onEvent(p_event, p_registry);
			}
		}
	};
}
