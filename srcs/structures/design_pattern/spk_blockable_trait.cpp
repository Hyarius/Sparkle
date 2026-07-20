#include "structures/design_pattern/spk_blockable_trait.hpp"

#include <utility>

namespace spk
{
	void BlockableTrait::_executeOrBlock(Operation p_operation)
	{
		if (!p_operation)
		{
			throw std::invalid_argument("spk::BlockableTrait: empty operation");
		}
		{
			std::scoped_lock lock(_state->mutex);
			if (_state->nbIgnoreBlocks != 0)
			{
				return;
			}
			if (_state->nbDeferBlocks != 0)
			{
				_state->deferredOperations.emplace_back(std::move(p_operation));
				return;
			}
		}
		p_operation();
	}

	BlockableTrait::Blocker::Blocker(const std::shared_ptr<State> &p_state, Mode p_mode) :
		_state(p_state),
		_mode(p_mode)
	{
	}
	BlockableTrait::Blocker::~Blocker() noexcept
	{
		_release(false);
	}
	BlockableTrait::Blocker::Blocker(Blocker &&p_other) noexcept :
		_state(std::move(p_other._state)),
		_mode(p_other._mode)
	{
	}
	BlockableTrait::Blocker &BlockableTrait::Blocker::operator=(Blocker &&p_other) noexcept
	{
		if (this != &p_other)
		{
			_release(false);
			_state = std::move(p_other._state);
			_mode = p_other._mode;
		}
		return *this;
	}

	void BlockableTrait::Blocker::_release(bool p_throwIfInvalid)
	{
		const std::shared_ptr<State> state = _state.lock();
		if (!state)
		{
			if (p_throwIfInvalid)
			{
				throw std::logic_error("spk::BlockableTrait: invalid blocker release");
			}
			return;
		}
		std::deque<Operation> operations;
		{
			std::scoped_lock lock(state->mutex);
			std::size_t &count = _mode == Mode::Ignore ? state->nbIgnoreBlocks : state->nbDeferBlocks;
			if (count == 0)
			{
				_state.reset();
				if (p_throwIfInvalid)
				{
					throw std::logic_error("spk::BlockableTrait: invalid blocker release");
				}
				return;
			}
			--count;
			_state.reset();
			if (state->nbIgnoreBlocks == 0 && state->nbDeferBlocks == 0)
			{
				operations = std::move(state->deferredOperations);
			}
		}
		for (Operation &operation : operations)
		{
			operation();
		}
	}

	void BlockableTrait::Blocker::release()
	{
		_release(true);
	}
	bool BlockableTrait::Blocker::isValid() const noexcept
	{
		return !_state.expired();
	}

	BlockableTrait::Blocker BlockableTrait::block(Mode p_mode)
	{
		std::scoped_lock lock(_state->mutex);
		std::size_t &count = p_mode == Mode::Ignore ? _state->nbIgnoreBlocks : _state->nbDeferBlocks;
		if (count == std::numeric_limits<std::size_t>::max())
		{
			throw std::overflow_error("spk::BlockableTrait: blocker count overflow");
		}
		++count;
		return Blocker(_state, p_mode);
	}
	bool BlockableTrait::isBlocked() const noexcept
	{
		return isIgnoreBlocked() || isDeferBlocked();
	}
	bool BlockableTrait::isIgnoreBlocked() const noexcept
	{
		std::scoped_lock lock(_state->mutex);
		return _state->nbIgnoreBlocks != 0;
	}
	bool BlockableTrait::isDeferBlocked() const noexcept
	{
		std::scoped_lock lock(_state->mutex);
		return _state->nbDeferBlocks != 0;
	}
}
