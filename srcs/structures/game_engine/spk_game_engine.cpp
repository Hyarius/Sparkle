#include "structures/game_engine/spk_game_engine.hpp"

#include "structures/game_engine/spk_component_store.hpp"

namespace spk
{
	GameEngine::GameEngine()
	{
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

	void GameEngine::update(const spk::UpdateTick &p_tick)
	{
		if (isActivated() == false)
		{
			return;
		}

		_logicRegistry.update(p_tick, _components);
	}

	spk::RenderUnit GameEngine::buildRenderUnit()
	{
		spk::RenderUnitBuilder builder;

		if (isActivated() == true)
		{
			_logicRegistry.render(builder, _components);
		}

		return builder.build();
	}
}
