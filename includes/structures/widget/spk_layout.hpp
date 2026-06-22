#pragma once

#include <memory>
#include <vector>

#include "structures/math/spk_rect_2d.hpp"
#include "structures/math/spk_vector2.hpp"
#include "structures/widget/spk_resizable_element.hpp"
#include "structures/widget/spk_widget.hpp"

namespace spk
{
	class Layout : public spk::ResizableElement
	{
	public:
		enum class SizePolicy
		{
			Fixed,
			Minimum,
			Maximum,
			Extend,
			HorizontalExtend,
			VerticalExtend
		};

		class Element
		{
			friend class Layout;

		private:
			spk::Widget *_widget = nullptr;
			spk::Layout *_layout = nullptr;
			SizePolicy _sizePolicy = SizePolicy::Extend;
			spk::Vector2UInt _size = {0, 0};

		public:
			Element();
			Element(spk::Widget *p_widget, SizePolicy p_sizePolicy, const spk::Vector2UInt &p_size);
			Element(spk::Layout *p_layout, SizePolicy p_sizePolicy, const spk::Vector2UInt &p_size);

			[[nodiscard]] spk::Widget *widget() const;
			[[nodiscard]] spk::Layout *layout() const;

			[[nodiscard]] bool isWidget() const;
			[[nodiscard]] bool isLayout() const;

			[[nodiscard]] spk::Vector2UInt minimalSize() const;
			[[nodiscard]] spk::Vector2UInt fixedSize() const;
			[[nodiscard]] spk::Vector2UInt maximalSize() const;
			void setGeometry(const spk::Rect2D &p_geometry);

			void setSizePolicy(SizePolicy p_sizePolicy);
			[[nodiscard]] SizePolicy sizePolicy() const;

			void setSize(const spk::Vector2UInt &p_size);
			[[nodiscard]] const spk::Vector2UInt &size() const;
		};

	protected:
		std::vector<std::unique_ptr<Element>> _elements;
		spk::Vector2UInt _elementPadding = {0, 0};

	public:
		~Layout() override = default;

		void setGeometry(const spk::Rect2D &p_geometry) override = 0;

		[[nodiscard]] spk::Vector2UInt minimalSize() const;
		[[nodiscard]] spk::Vector2UInt fixedSize() const;
		[[nodiscard]] spk::Vector2UInt maximalSize() const;

		virtual void clear();

		Element *addWidget(spk::Widget *p_widget, SizePolicy p_sizePolicy = SizePolicy::Extend);
		Element *addLayout(spk::Layout *p_layout, SizePolicy p_sizePolicy = SizePolicy::Extend);
		void removeElement(Element *p_element);
		void removeWidget(spk::Widget *p_widget);
		void removeLayout(spk::Layout *p_layout);

		void setElementPadding(const spk::Vector2UInt &p_padding);
		[[nodiscard]] const spk::Vector2UInt &elementPadding() const;

		[[nodiscard]] const std::vector<std::unique_ptr<Element>> &elements() const;
		[[nodiscard]] std::vector<std::unique_ptr<Element>> &elements();
	};
}
