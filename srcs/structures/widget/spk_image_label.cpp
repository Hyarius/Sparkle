#include "structures/widget/spk_image_label.hpp"

#include <stdexcept>
#include <utility>

#include "structures/graphics/rendering/command/spk_image_render_command.hpp"

namespace spk
{
	ImageLabel::ImageLabel(const std::string& p_name, spk::Widget* p_parent) :
		spk::Widget(p_name, p_parent)
	{
		activate();
	}

	ImageLabel::ImageLabel(
		const std::string& p_name,
		std::shared_ptr<spk::Texture> p_texture,
		spk::Widget* p_parent) :
		spk::Widget(p_name, p_parent)
	{
		setTexture(std::move(p_texture));
		activate();
	}

	spk::RenderUnit ImageLabel::_buildRenderUnit() const
	{
		spk::RenderUnitBuilder builder;

		if (_texture != nullptr && geometry().empty() == false)
		{
			builder.emplace<spk::ImageRenderCommand>(
				*_texture,
				_section,
				geometry(),
				_depth);
		}

		return builder.build();
	}

	void ImageLabel::setTexture(std::shared_ptr<spk::Texture> p_texture)
	{
		if (p_texture == nullptr)
		{
			throw std::invalid_argument("ImageLabel texture cannot be null");
		}

		_texture = std::move(p_texture);
		invalidateRenderUnit();
	}

	void ImageLabel::setSection(const spk::Texture::Section& p_section)
	{
		if (_section == p_section)
		{
			return;
		}

		_section = p_section;
		invalidateRenderUnit();
	}

	void ImageLabel::setDepth(float p_depth)
	{
		if (_depth == p_depth)
		{
			return;
		}

		_depth = p_depth;
		invalidateRenderUnit();
	}

	const std::shared_ptr<spk::Texture>& ImageLabel::texture() const
	{
		return _texture;
	}

	const spk::Texture::Section& ImageLabel::section() const
	{
		return _section;
	}

	float ImageLabel::depth() const
	{
		return _depth;
	}
}
