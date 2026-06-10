#pragma once

#include <string>

#include "structures/math/spk_vector2.hpp"
#include "structures/widget/spk_widget.hpp"

namespace spk
{
	class ContainerWidget : public spk::Widget
	{
	private:
		spk::Widget* _content = nullptr;
		spk::Vector2Int _contentAnchor = {0, 0};
		spk::Vector2UInt _contentSize = {0, 0};

		void _refreshContentGeometry();

	protected:
		void _onGeometryChange() override;

	public:
		explicit ContainerWidget(const std::string& p_name, spk::Widget* p_parent = nullptr);

		void setContent(spk::Widget* p_content);
		void setContentAnchor(const spk::Vector2Int& p_contentAnchor);
		void setContentSize(const spk::Vector2UInt& p_contentSize);

		[[nodiscard]] spk::Widget* content();
		[[nodiscard]] const spk::Widget* content() const;
		[[nodiscard]] const spk::Vector2Int& contentAnchor() const;
		[[nodiscard]] const spk::Vector2UInt& contentSize() const;
	};
}
