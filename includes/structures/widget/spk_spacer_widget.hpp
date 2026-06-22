#pragma once

#include <string>

#include "structures/widget/spk_widget.hpp"

namespace spk
{
	class SpacerWidget : public spk::Widget
	{
	public:
		explicit SpacerWidget(const std::string &p_name, spk::Widget *p_parent = nullptr);
	};
}
