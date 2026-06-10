#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "structures/widget/spk_linear_layout.hpp"
#include "structures/widget/spk_push_button.hpp"
#include "structures/widget/spk_spacer_widget.hpp"
#include "structures/widget/spk_widget.hpp"

namespace spk
{
	class CommandPanel : public spk::Widget
	{
	public:
		using Callback = spk::ContractProvider<>::Callback;
		using Contract = spk::ContractProvider<>::Contract;

	private:
		spk::HorizontalLayout _layout;
		spk::SpacerWidget _spacer;
		spk::Layout::SizePolicy _sizePolicy = spk::Layout::SizePolicy::Minimum;
		std::unordered_map<std::string, std::unique_ptr<spk::PushButton>> _buttons;
		std::vector<spk::PushButton*> _orderedButtons;

		void _composeLayout();

	protected:
		void _onGeometryChange() override;

	public:
		explicit CommandPanel(const std::string& p_name, spk::Widget* p_parent = nullptr);

		spk::PushButton* addButton(const std::string& p_name, std::string_view p_label);
		[[nodiscard]] spk::PushButton* button(const std::string& p_name);
		[[nodiscard]] const spk::PushButton* button(const std::string& p_name) const;
		void removeButton(const std::string& p_name);
		[[nodiscard]] size_t nbButton() const;

		Contract subscribe(const std::string& p_name, Callback p_callback);

		void setSizePolicy(spk::Layout::SizePolicy p_sizePolicy);
		[[nodiscard]] spk::Layout::SizePolicy sizePolicy() const;

		void setElementPadding(const spk::Vector2UInt& p_padding);
	};
}
