#pragma once

#include <cstddef>
#include <memory>

namespace spk
{
	class BlockableTrait
	{
	public:
		enum class Mode
		{
			Ignore,
			Delay
		};

	private:
		struct State
		{
			BlockableTrait* owner = nullptr;
			size_t nbIgnoreBlocks = 0;
			size_t nbDelayBlocks = 0;
			bool pending = false;
			bool alive = true;
		};

		std::shared_ptr<State> _state = std::make_shared<State>();

	protected:
		BlockableTrait()
		{
			_state->owner = this;
		}

		virtual ~BlockableTrait()
		{
			if (_state != nullptr)
			{
				_state->alive = false;
				_state->owner = nullptr;
				_state->pending = false;
			}
		}

		virtual void flushPending() = 0;

		void setPending()
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

	public:
		class Blocker
		{
		private:
			std::weak_ptr<State> _state;
			Mode _mode = Mode::Ignore;

			void _tryFlushDelayed(State& p_state)
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

		public:
			Blocker() = default;

			explicit Blocker(const std::shared_ptr<State>& p_state, Mode p_mode) :
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

			~Blocker()
			{
				release();
			}

			Blocker(const Blocker&) = delete;
			Blocker& operator=(const Blocker&) = delete;

			Blocker(Blocker&& p_other) noexcept :
				_state(std::move(p_other._state)),
				_mode(p_other._mode)
			{
			}

			Blocker& operator=(Blocker&& p_other) noexcept
			{
				if (this != &p_other)
				{
					release();
					_state = std::move(p_other._state);
					_mode = p_other._mode;
				}

				return *this;
			}

			void release()
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

			bool isValid() const
			{
				return (_state.expired() == false);
			}
		};

		Blocker block(Mode p_mode = Mode::Ignore)
		{
			return Blocker(_state, p_mode);
		}

		bool isBlocked() const
		{
			if (_state == nullptr)
			{
				return false;
			}

			return (_state->nbIgnoreBlocks > 0 || _state->nbDelayBlocks > 0);
		}

		bool isIgnoreBlocked() const
		{
			if (_state == nullptr)
			{
				return false;
			}

			return (_state->nbIgnoreBlocks > 0);
		}

		bool isDelayBlocked() const
		{
			if (_state == nullptr)
			{
				return false;
			}

			return (_state->nbDelayBlocks > 0);
		}
	};
}