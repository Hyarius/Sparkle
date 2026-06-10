#pragma once

#include <memory>
#include <string>

#include "structures/graphics/texture/spk_texture.hpp"
#include "structures/widget/spk_widget.hpp"

namespace spk
{
	class ImageLabel : public spk::Widget
	{
	private:
		std::shared_ptr<spk::Texture> _texture;
		spk::Texture::Section _section = {{0.0f, 0.0f}, {1.0f, 1.0f}};
		float _depth = 0.0f;

	protected:
		[[nodiscard]] spk::RenderUnit _buildRenderUnit() const override;

	public:
		explicit ImageLabel(const std::string& p_name, spk::Widget* p_parent = nullptr);
		ImageLabel(
			const std::string& p_name,
			std::shared_ptr<spk::Texture> p_texture,
			spk::Widget* p_parent = nullptr);

		void setTexture(std::shared_ptr<spk::Texture> p_texture);
		void setSection(const spk::Texture::Section& p_section);
		void setDepth(float p_depth);

		[[nodiscard]] const std::shared_ptr<spk::Texture>& texture() const;
		[[nodiscard]] const spk::Texture::Section& section() const;
		[[nodiscard]] float depth() const;
	};
}
