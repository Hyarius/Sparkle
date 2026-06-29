#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "component2d.hpp"
#include "structures/math/spk_vector2.hpp"
#include "structures/system/time/spk_duration.hpp"

namespace pg
{
	struct Animation2D
	{
		std::vector<spk::Vector2UInt> frames;
		spk::Duration frameDuration{120.0L, spk::TimeUnit::Millisecond};
		bool loop = true;
	};

	class AnimationController2D : public Component2D
	{
	private:
		std::unordered_map<std::wstring, Animation2D> _animations;
		std::wstring _currentName;
		bool _hasCurrent = false;
		bool _playing = false;
		double _elapsedSeconds = 0.0;

	public:
		AnimationController2D() = default;

		void addAnimation(const std::wstring &p_name, const Animation2D &p_animation)
		{
			_animations[p_name] = p_animation;
		}

		[[nodiscard]] bool hasAnimation(const std::wstring &p_name) const
		{
			return _animations.find(p_name) != _animations.end();
		}

		void play(const std::wstring &p_name)
		{
			if (_animations.find(p_name) == _animations.end())
			{
				return;
			}

			if (_hasCurrent == false || _currentName != p_name)
			{
				_currentName = p_name;
				_hasCurrent = true;
				_elapsedSeconds = 0.0;
			}

			_playing = true;
		}
		
		void stop()
		{
			_playing = false;
		}

		[[nodiscard]] bool isPlaying() const
		{
			return _playing;
		}

		[[nodiscard]] bool hasCurrent() const
		{
			return _hasCurrent;
		}

		[[nodiscard]] const std::wstring &currentName() const
		{
			return _currentName;
		}

		[[nodiscard]] const Animation2D *currentAnimation() const
		{
			if (_hasCurrent == false)
			{
				return nullptr;
			}

			auto iterator = _animations.find(_currentName);
			return iterator == _animations.end() ? nullptr : &iterator->second;
		}

		[[nodiscard]] double elapsedSeconds() const
		{
			return _elapsedSeconds;
		}

		void addElapsed(double p_seconds)
		{
			_elapsedSeconds += p_seconds;
		}
	};
}
