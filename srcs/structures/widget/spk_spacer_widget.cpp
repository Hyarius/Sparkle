#include "structures/widget/spk_spacer_widget.hpp"

namespace spk
{
	SpacerWidget::SpacerWidget(const std::string &p_name, spk::Widget *p_parent) :
		spk::Widget(p_name, p_parent)
	{
		activate();
	}
}
