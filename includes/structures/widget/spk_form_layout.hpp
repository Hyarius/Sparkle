#pragma once

#include "structures/widget/spk_layout.hpp"

namespace spk
{
	class FormLayout : public spk::Layout
	{
	private:
		using Layout::addWidget;
		using Layout::removeElement;
		using Layout::removeLayout;
		using Layout::removeWidget;

		[[nodiscard]] spk::Vector2UInt _computeMinimalSize() const;

	public:
		struct FormElement
		{
			Element *labelElement = nullptr;
			Element *fieldElement = nullptr;
		};

		FormLayout();

		FormElement addRow(
			spk::Widget *p_labelWidget,
			spk::Widget *p_fieldWidget,
			SizePolicy p_labelPolicy = SizePolicy::Minimum,
			SizePolicy p_fieldPolicy = SizePolicy::Extend);

		void removeRow(const FormElement &p_row);

		[[nodiscard]] size_t nbRow() const;

		void setGeometry(const spk::Rect2D &p_geometry) override;
	};
}
