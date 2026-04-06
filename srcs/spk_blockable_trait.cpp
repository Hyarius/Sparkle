#include "spk_blockable_trait.hpp"

#include <utility>

namespace spk
{
	BlockableTrait::BlockableTrait()
	{
		_state->owner = this;
	}

	BlockableTrait::~BlockableTrait()
	{
		if (_state != nullptr)
		{
			_state->alive = false;
			_state->owner = nullptr;
			_state->pending = false;
		}
	}

	void BlockableTrait::setPending()
	{
		if (_state == nullptr)
		{
			return;
		}

		if (_state->nbDelayBlocks > 0)
		{
			_state->pending = true;
		}
	}

	void BlockableTrait::Blocker::_tryFlushDelayed(State& p_state)
	{
		if (p_state.alive == false ||
			p_state.owner == nullptr ||
			p_state.pending == false)
		{
			return;
		}

		if (p_state.nbIgnoreBlocks > 0 || p_state.nbDelayBlocks > 0)
		{
			return;
		}

		p_state.pending = false;
		p_state.owner->flushPending();
	}

	BlockableTrait::Blocker::Blocker(const std::shared_ptr<State>& p_state, Mode p_mode) :
		_state(p_state),
		_mode(p_mode)
	{
		std::shared_ptr<State> state = _state.lock();
		if (state == nullptr)
		{
			return;
		}

		if (_mode == Mode::Delay)
		{
			++state->nbDelayBlocks;
		}
		else
		{
			++state->nbIgnoreBlocks;
		}
	}

	BlockableTrait::Blocker::~Blocker()
	{
		release();
	}

	BlockableTrait::Blocker::Blocker(Blocker&& p_other) noexcept :
		_state(std::move(p_other._state)),
		_mode(p_other._mode)
	{
	}

	BlockableTrait::Blocker& BlockableTrait::Blocker::operator=(Blocker&& p_other) noexcept
	{
		if (this != &p_other)
		{
			release();
			_state = std::move(p_other._state);
			_mode = p_other._mode;
		}

		return *this;
	}

	void BlockableTrait::Blocker::release()
	{
		std::shared_ptr<State> state = _state.lock();
		if (state != nullptr)
		{
			if (_mode == Mode::Delay)
			{
				if (state->nbDelayBlocks > 0)
				{
					--state->nbDelayBlocks;
				}
			}
			else
			{
				if (state->nbIgnoreBlocks > 0)
				{
					--state->nbIgnoreBlocks;
				}
			}

			_tryFlushDelayed(*state);
		}

		_state.reset();
	}

	bool BlockableTrait::Blocker::isValid() const
	{
		return (_state.expired() == false);
	}

	BlockableTrait::Blocker BlockableTrait::block(Mode p_mode)
	{
		return Blocker(_state, p_mode);
	}

	bool BlockableTrait::isBlocked() const
	{
		if (_state == nullptr)
		{
			return false;
		}

		return (_state->nbIgnoreBlocks > 0 || _state->nbDelayBlocks > 0);
	}

	bool BlockableTrait::isIgnoreBlocked() const
	{
		if (_state == nullptr)
		{
			return false;
		}

		return (_state->nbIgnoreBlocks > 0);
	}

	bool BlockableTrait::isDelayBlocked() const
	{
		if (_state == nullptr)
		{
			return false;
		}

		return (_state->nbDelayBlocks > 0);
	}
}