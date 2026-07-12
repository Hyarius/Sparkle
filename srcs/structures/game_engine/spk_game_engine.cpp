#include "structures/game_engine/spk_game_engine.hpp"

#include "structures/game_engine/spk_component_store.hpp"
#include "structures/game_engine/rendering/spk_scene_lighting_render_feature.hpp"

namespace spk
{
	GameEngine::GameEngine()
	{
		_renderPipeline.emplaceFeature<spk::SceneLightingRenderFeature>();
		activate();
	}

	GameEngine::~GameEngine()
	{
		spk::ComponentStore::instance().clearEngine(_id);
	}

	const spk::UUID &GameEngine::id() const
	{
		return _id;
	}

	spk::ComponentRegistry &GameEngine::componentRegistry()
	{
		return _components;
	}

	const spk::ComponentRegistry &GameEngine::componentRegistry() const
	{
		return _components;
	}

	spk::ComponentLogicRegistry &GameEngine::logicRegistry()
	{
		return _logicRegistry;
	}

	const spk::ComponentLogicRegistry &GameEngine::logicRegistry() const
	{
		return _logicRegistry;
	}

	spk::SceneRenderPipeline &GameEngine::renderPipeline() noexcept
	{
		return _renderPipeline;
	}
	const spk::SceneRenderPipeline &GameEngine::renderPipeline() const noexcept
	{
		return _renderPipeline;
	}

	void GameEngine::addEntity(spk::Entity *p_entity)
	{
		if (p_entity == nullptr)
		{
			return;
		}

		p_entity->_setEngineId(_id);
	}

	void GameEngine::removeEntity(spk::Entity *p_entity)
	{
		if (p_entity == nullptr || p_entity->_engineId != _id)
		{
			return;
		}

		p_entity->_setEngineId(spk::UUID::null());
	}

	void GameEngine::update(const spk::UpdateContext &p_tick)
	{
		_profiler = p_tick.profiler;
		++_frameIndex;
		if (isActivated() == false)
		{
			return;
		}

		_logicRegistry.update(p_tick, _components);
	}

	spk::RenderUnit GameEngine::buildRenderUnit()
	{
		const spk::Viewport compatibilityViewport(spk::Rect2D(0, 0, 1, 1));
		return buildRenderUnit(spk::SceneRenderFrameRequest{.mainTarget = spk::RenderTargetReference{.frameBuffer = nullptr, .viewport = compatibilityViewport}, .mainClear = {}});
	}

	void GameEngine::contributeRenderPasses(spk::RenderFrameBuildContext &p_frame, const spk::SceneRenderFrameRequest &p_request)
	{
		_renderPipeline.buildPasses(
			p_frame, _renderScope, p_request, _logicRegistry, _components, _profiler, isActivated());
	}

	spk::RenderPlan GameEngine::buildRenderPlan(const spk::SceneRenderFrameRequest &p_request)
	{
		spk::RenderPassBucketPack passes;
		spk::RenderFrameBuildContext frame{.passes = passes, .frameIndex = _frameIndex};
		contributeRenderPasses(frame, p_request);
		return passes.build();
	}

	spk::RenderUnit GameEngine::buildRenderUnit(const spk::SceneRenderFrameRequest &p_request)
	{
		spk::RenderPlan plan = buildRenderPlan(p_request);
		return plan.compile();
	}
}
