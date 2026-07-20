#pragma once

#include <algorithm>
#include <concepts>
#include <memory>
#include <stdexcept>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <vector>

#include "structures/game_engine/rendering/spk_scene_render_passes.hpp"
#include "structures/game_engine/rendering/spk_scene_render_priorities.hpp"
#include "structures/game_engine/spk_camera_3d.hpp"
#include "structures/game_engine/spk_component_logic.hpp"
#include "structures/game_engine/spk_component_registry.hpp"
#include "structures/graphics/rendering/command/spk_camera_update_render_command.hpp"
#include "structures/graphics/rendering/command/spk_directional_light_update_render_command.hpp"
#include "structures/graphics/rendering/pipeline/spk_render_pipeline.hpp"
#include "structures/graphics/rendering/spk_scene_gpu_bindings.hpp"
#include "structures/graphics/spk_directional_light.hpp"

namespace spk
{
	class ComponentLogicRegistry
	{
	private:
		std::vector<std::unique_ptr<spk::IComponentLogic>> _logics;
		std::unordered_map<std::type_index, spk::IComponentLogic *> _lookup;
		std::vector<spk::IComponentLogic *> _updateEventOrder;
		std::vector<spk::PriorizableTrait::Contract> _priorityContracts;
		bool _updateEventOrderDirty = false;

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
			_updateEventOrder.clear();
			_logics.clear();
			_updateEventOrderDirty = false;
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
			_priorityContracts.push_back(result.subscribeToPriorityChange([this]() {
				_updateEventOrderDirty = true;
			}));

			_lookup.emplace(typeIndex, logic.get());
			_logics.push_back(std::move(logic));
			_updateEventOrderDirty = true;
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

		void render(
			const spk::SceneRenderBuildContext &p_context,
			spk::ComponentRegistry &p_registry)
		{
			_ensureUpdateEventSorted();
			for (spk::IComponentLogic *logic : _updateEventOrder)
			{
				if (logic == nullptr || !logic->isActivated())
				{
					continue;
				}
				logic->onRender(p_context, p_registry);
			}
		}

		// Compatibility entry point for callers that already own the active target.
		// Work is still recorded and flattened by semantic phase, but no target
		// transition is emitted because this legacy API has no target description.
		void render(spk::RenderUnitBuilder &p_builder, spk::ComponentRegistry &p_registry)
		{
			const spk::Viewport viewport(spk::Rect2D(0, 0, 1, 1));
			const spk::SceneRenderFrameRequest request{
				.mainTarget = spk::RenderTargetReference{.frameBuffer = nullptr, .viewport = viewport, .activeTarget = true},
				.mainClear = {}};
			spk::RenderPipeline passes;
			spk::RenderFrameBuildContext frame{.passes = passes};
			passes.emplace<spk::RenderPass>(std::string(spk::SceneRenderPasses::MainOpaque), spk::SceneRenderPriorities::MainOpaque, {.target = request.mainTarget, .clear = {}});
			passes.emplace<spk::RenderPass>(std::string(spk::SceneRenderPasses::MainTransparent), spk::SceneRenderPriorities::MainTransparent, {.target = request.mainTarget, .clear = {}});
			passes.emplace<spk::RenderPass>(std::string(spk::SceneRenderPasses::MainOverlay), spk::SceneRenderPriorities::MainOverlay, {.target = request.mainTarget, .clear = {}});
			spk::Camera3D *mainCamera = spk::Camera3D::mainCamera();
			if (mainCamera != nullptr)
			{
				for (const auto type : {spk::SceneRenderPasses::MainOpaque, spk::SceneRenderPasses::MainTransparent, spk::SceneRenderPasses::MainOverlay})
				{
					passes.require(type).emplace<spk::CameraUpdateRenderCommand>(spk::SceneGpuBindings::Camera, mainCamera->viewProjectionMatrix(), mainCamera->viewMatrix());
				}
			}
			spk::SceneRenderBuildContext context{.frame = frame, .request = request, .components = p_registry, .mainCamera = mainCamera};
			render(context, p_registry);
			spk::RenderUnit unit = passes.build().compile();
			for (auto &command : unit.takeCommands())
			{
				p_builder.add(std::move(command));
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
