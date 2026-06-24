#include "structures/game_engine/spk_game_engine.hpp"

#include <algorithm>

namespace spk
{
	GameEngine::GameEngine()
	{
		activate();
	}

	GameEngine::~GameEngine()
	{
		// Detach every entity while the registries are still alive.
		clearEntities();
	}

	spk::ComponentRegistry &GameEngine::componentRegistry()
	{
		return _componentRegistry;
	}

	const spk::ComponentRegistry &GameEngine::componentRegistry() const
	{
		return _componentRegistry;
	}

	spk::ComponentLogicRegistry &GameEngine::logicRegistry()
	{
		return _logicRegistry;
	}

	const spk::ComponentLogicRegistry &GameEngine::logicRegistry() const
	{
		return _logicRegistry;
	}

	void GameEngine::destroyEntity(spk::Entity &p_entity)
	{
		auto iterator = std::find_if(
			_entities.begin(),
			_entities.end(),
			[&p_entity](const std::unique_ptr<spk::Entity> &p_entry) {
				return (p_entry.get() == &p_entity);
			});

		if (iterator == _entities.end())
		{
			return;
		}

		// Detaching this entity's own components happens in ~Entity (via _registry);
		// its children are orphaned by the hierarchy trait but remain registered.
		_entities.erase(iterator);
		_componentRegistry.invalidateProcessable();
	}

	void GameEngine::clearEntities()
	{
		_entities.clear();
		_componentRegistry.clear();
	}

	const std::vector<std::unique_ptr<spk::Entity>> &GameEngine::entities() const
	{
		return _entities;
	}

	void GameEngine::update(const spk::UpdateTick &p_tick)
	{
		if (isActivated() == false)
		{
			return;
		}

		// Frame heartbeat: always recompute the processable subset so activation
		// changes anywhere in the hierarchy are picked up.
		_componentRegistry.refreshProcessable();
		_logicRegistry.update(p_tick, _componentRegistry);
	}

	void GameEngine::synchronize()
	{
		if (isActivated() == false)
		{
			return;
		}

		_componentRegistry.refreshProcessableIfDirty();
		_logicRegistry.synchronize(_componentRegistry);
	}

	void GameEngine::render(spk::RenderUnitBuilder &p_builder)
	{
		if (isActivated() == false)
		{
			return;
		}

		_componentRegistry.refreshProcessableIfDirty();
		_logicRegistry.render(p_builder, _componentRegistry);
	}
}
