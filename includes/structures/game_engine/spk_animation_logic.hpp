#pragma once

#include <algorithm>
#include <cstddef>

#include "structures/game_engine/spk_animation_2d.hpp"
#include "structures/game_engine/spk_component_logic.hpp"
#include "structures/game_engine/spk_entity.hpp"
#include "structures/game_engine/spk_entity_2d.hpp"
#include "structures/game_engine/spk_sprite_renderer_2d.hpp"

namespace spk
{
	class AnimationLogic : public spk::ComponentLogic<AnimationController2D>
	{
	public:
	protected:
		void _parseComponentForUpdate(const spk::UpdateContext &p_tick, AnimationController2D &p_controller) override
		{
			spk::Entity *entity = p_controller.entity();
			if (entity == nullptr)
			{
				return;
			}

			SpriteRenderer2D *renderer = entity->component<SpriteRenderer2D>();
			const Animation2D *animation = p_controller.currentAnimation();
			if (renderer == nullptr || renderer->spriteSheet() == nullptr || animation == nullptr || animation->frames.empty() == true)
			{
				return;
			}

			std::size_t frameIndex = 0;

			if (p_controller.isPlaying() == true)
			{
				p_controller.addElapsed(p_tick.deltaTime.seconds());

				const double frameSeconds = std::max(1e-6, animation->frameDuration.seconds());
				const std::size_t rawFrame = static_cast<std::size_t>(p_controller.elapsedSeconds() / frameSeconds);

				if (animation->loop == true)
				{
					frameIndex = rawFrame % animation->frames.size();
				}
				else
				{
					frameIndex = std::min(rawFrame, animation->frames.size() - 1);
				}
			}

			renderer->setSprite(renderer->spriteSheet()->sprite(animation->frames[frameIndex]));
		}
	};
}
