#include "structures/widget/spk_screen.hpp"

namespace spk
{
	Screen::Screen(const std::string &p_name, spk::Widget *p_parent) :
		spk::Widget(p_name, p_parent)
	{
		_activationContract = subscribeToActivation([this]() {
			if (_activeScreen != nullptr && _activeScreen != this)
			{
				_activeScreen->deactivate();
			}
			_activeScreen = this;
		});
	}

	Screen::~Screen()
	{
		if (_activeScreen == this)
		{
			_activeScreen = nullptr;
		}
	}

	Screen *Screen::activeScreen()
	{
		return _activeScreen;
	}
}
