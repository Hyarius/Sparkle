#pragma once

#include <cstddef>
#include <memory>
#include <string>

#include "structures/graphics/texture/spk_sprite_sheet.hpp"
#include "structures/system/time/spk_duration.hpp"
#include "structures/system/time/spk_timer.hpp"
#include "structures/widget/spk_widget.hpp"

namespace spk
{
	class AnimationLabel : public spk::Widget
	{
	private:
		std::shared_ptr<spk::SpriteSheet> _spriteSheet;
		size_t _currentSprite = 0;
		size_t _rangeStart = 0;
		size_t _rangeEnd = 0;
		float _depth = 0.0f;
		spk::Timer _timer = spk::Timer(spk::Duration(125.0L, spk::TimeUnit::Millisecond));

	protected:
		[[nodiscard]] spk::RenderUnit _buildRenderUnit() const override;
		void _onUpdate(const spk::UpdateContext &p_tick) override;

	public:
		explicit AnimationLabel(const std::string &p_name, spk::Widget *p_parent = nullptr);
		AnimationLabel(
			const std::string &p_name,
			std::shared_ptr<spk::SpriteSheet> p_spriteSheet,
			spk::Widget *p_parent = nullptr);

		void setSpriteSheet(std::shared_ptr<spk::SpriteSheet> p_spriteSheet);
		void setLoopSpeed(const spk::Duration &p_duration);
		void setAnimationRange(size_t p_start, size_t p_end);
		void setDepth(float p_depth);

		[[nodiscard]] const std::shared_ptr<spk::SpriteSheet> &spriteSheet() const;
		[[nodiscard]] size_t currentFrame() const;
		[[nodiscard]] size_t rangeStart() const;
		[[nodiscard]] size_t rangeEnd() const;
		[[nodiscard]] float depth() const;
	};
}
