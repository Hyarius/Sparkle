#include "structures/widget/spk_animation_label.hpp"

#include <stdexcept>
#include <utility>

#include "structures/graphics/rendering/command/spk_sprite_render_command.hpp"

namespace spk
{
	AnimationLabel::AnimationLabel(const std::string &p_name, spk::Widget *p_parent) :
		spk::Widget(p_name, p_parent)
	{
		activate();
	}

	AnimationLabel::AnimationLabel(
		const std::string &p_name,
		std::shared_ptr<spk::SpriteSheet> p_spriteSheet,
		spk::Widget *p_parent) :
		spk::Widget(p_name, p_parent)
	{
		setSpriteSheet(std::move(p_spriteSheet));
		activate();
	}

	spk::RenderUnit AnimationLabel::_buildRenderUnit() const
	{
		spk::RenderUnitBuilder builder;

		if (_spriteSheet != nullptr &&
			_spriteSheet->sprites().empty() == false &&
			geometry().empty() == false)
		{
			const spk::Vector2UInt spriteCoordinates = {
				static_cast<unsigned int>(_currentSprite % _spriteSheet->nbSprite().x),
				static_cast<unsigned int>(_currentSprite / _spriteSheet->nbSprite().x)};

			builder.emplace<spk::SpriteRenderCommand>(
				*_spriteSheet,
				spriteCoordinates,
				geometry(),
				_depth);
		}

		return builder.build();
	}

	void AnimationLabel::_onUpdate(const spk::UpdateTick &p_tick)
	{
		(void)p_tick;

		if (_spriteSheet == nullptr || _spriteSheet->sprites().empty() == true)
		{
			return;
		}

		if (_timer.state() == spk::Timer::State::Running)
		{
			return;
		}

		if (_rangeEnd >= _rangeStart && _rangeEnd < _spriteSheet->sprites().size())
		{
			_currentSprite++;
			if (_currentSprite > _rangeEnd)
			{
				_currentSprite = _rangeStart;
			}
		}
		else
		{
			_currentSprite = (_currentSprite + 1) % _spriteSheet->sprites().size();
		}

		invalidateRenderUnit();
		_timer.start();
	}

	void AnimationLabel::setSpriteSheet(std::shared_ptr<spk::SpriteSheet> p_spriteSheet)
	{
		if (p_spriteSheet == nullptr)
		{
			throw std::invalid_argument("AnimationLabel sprite sheet cannot be null");
		}

		_spriteSheet = std::move(p_spriteSheet);
		_currentSprite = 0;
		_rangeStart = 0;
		_rangeEnd = (_spriteSheet->sprites().empty() == false) ? _spriteSheet->sprites().size() - 1 : 0;
		_timer.stop();
		invalidateRenderUnit();
	}

	void AnimationLabel::setLoopSpeed(const spk::Duration &p_duration)
	{
		_timer = spk::Timer(p_duration);
		_currentSprite = _rangeStart;
		invalidateRenderUnit();
	}

	void AnimationLabel::setAnimationRange(size_t p_start, size_t p_end)
	{
		if (p_end < p_start)
		{
			throw std::invalid_argument("AnimationLabel animation range end cannot be lower than its start");
		}

		_rangeStart = p_start;
		_rangeEnd = p_end;
		_currentSprite = p_start;
		invalidateRenderUnit();
	}

	void AnimationLabel::setDepth(float p_depth)
	{
		if (_depth == p_depth)
		{
			return;
		}

		_depth = p_depth;
		invalidateRenderUnit();
	}

	const std::shared_ptr<spk::SpriteSheet> &AnimationLabel::spriteSheet() const
	{
		return _spriteSheet;
	}

	size_t AnimationLabel::currentFrame() const
	{
		return _currentSprite;
	}

	size_t AnimationLabel::rangeStart() const
	{
		return _rangeStart;
	}

	size_t AnimationLabel::rangeEnd() const
	{
		return _rangeEnd;
	}

	float AnimationLabel::depth() const
	{
		return _depth;
	}
}
