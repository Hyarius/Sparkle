#include "battle/phases/battle_orchestrator.hpp"
namespace pg
{
	BattleOrchestrator::~BattleOrchestrator()
	{
		reset();
	}
	void BattleOrchestrator::reset()
	{
		if (_current)
		{
			_current->exit();
			_current = nullptr;
		}
	}
	void BattleOrchestrator::transitionTo(IBattlePhase &p)
	{
		if (_current == &p)
		{
			return;
		}
		if (_current)
		{
			_current->exit();
		}
		_current = &p;
		_current->enter();
	}
	void BattleOrchestrator::tick(float s)
	{
		if (_current)
		{
			_current->tick(s);
		}
	}
	IBattlePhase *BattleOrchestrator::current() const noexcept
	{
		return _current;
	}
}
