#pragma once

#include <string>

#include "structures/widget/spk_widget.hpp"

namespace spk
{
	class Screen : public spk::Widget
	{
	private:
		static inline Screen* _activeScreen = nullptr;

		spk::ActivableTrait::Contract _activationContract;

	public:
		explicit Screen(const std::string& p_name, spk::Widget* p_parent = nullptr);
		~Screen() override;

		[[nodiscard]] static Screen* activeScreen();
	};
}
