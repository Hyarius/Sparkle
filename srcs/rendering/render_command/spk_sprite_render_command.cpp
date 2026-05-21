#include "rendering/render_command/spk_sprite_render_command.hpp"

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

namespace spk
{
	SpriteRenderCommand::SpriteRenderCommand(
		const spk::SpriteSheet& p_spriteSheet,
		spk::Vector2UInt p_spriteCoordinates,
		spk::Rect2D p_screenRect,
		float p_depth) :
		_spriteSheet(p_spriteSheet),
		_spriteCoordinates(p_spriteCoordinates),
		_screenRect(p_screenRect),
		_depth(p_depth)
	{
	}

	void SpriteRenderCommand::_ensureSpriteCommand() const
	{
		if (_spriteCommand == nullptr)
		{
			_spriteCommand = std::make_unique<spk::DrawSpriteRenderCommand>(
				_spriteSheet,
				_spriteCoordinates,
				_screenRect,
				_depth);
		}
	}

	void SpriteRenderCommand::execute(spk::IRenderContext& p_renderContext)
	{
		_ensureSpriteCommand();
		_spriteCommand->execute(p_renderContext);
	}
}

#endif
