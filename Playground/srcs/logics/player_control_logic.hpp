#pragma once

#include <cmath>
#include <string>

#include "components/player_controller.hpp"
#include "structures/game_engine/spk_animation_2d.hpp"
#include "structures/game_engine/spk_component_logic.hpp"
#include "structures/game_engine/spk_entity.hpp"
#include "structures/game_engine/spk_entity_2d.hpp"
#include "structures/game_engine/spk_transform_2d.hpp"
#include "structures/system/device/input/spk_keyboard.hpp"

namespace pg
{
	class PlayerControlLogic : public spk::ComponentLogic<PlayerController>
	{
	protected:
		void _parseComponentForUpdate(const spk::UpdateTick &p_tick, PlayerController &p_controller) override
		{
			spk::Entity *entity = p_controller.entity();
			if (p_tick.keyboard == nullptr || entity == nullptr)
			{
				return;
			}

			spk::Transform2D *transform = entity->component<spk::Transform2D>();
			if (transform == nullptr)
			{
				return;
			}

			const spk::Keyboard &keyboard = *p_tick.keyboard;

			spk::Vector2 direction{0.0f, 0.0f};
			if (keyboard[spk::Keyboard::Z] == spk::InputState::Down) { direction.y += 1.0f; }
			if (keyboard[spk::Keyboard::S] == spk::InputState::Down) { direction.y -= 1.0f; }
			if (keyboard[spk::Keyboard::D] == spk::InputState::Down) { direction.x += 1.0f; }
			if (keyboard[spk::Keyboard::Q] == spk::InputState::Down) { direction.x -= 1.0f; }

			spk::AnimationController2D *animation = entity->component<spk::AnimationController2D>();

			if (direction.isZero() == true)
			{
				if (animation != nullptr)
				{
					animation->stop();
				}
				return;
			}

			const float distance = p_controller.speed() * static_cast<float>(p_tick.deltaTime.seconds());
			transform->move(direction.normalized() * distance);

			if (animation != nullptr)
			{
				std::wstring name;
				if (std::abs(direction.x) >= std::abs(direction.y))
				{
					name = L"side";
				}
				else if (direction.y > 0.0f)
				{
					name = L"up";
				}
				else
				{
					name = L"down";
				}
				animation->play(name);
			}
		}
	};
}
