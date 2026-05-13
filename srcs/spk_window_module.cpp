#include "spk_module.hpp"

namespace spk
{
	IModule::~IModule() = default;

	void IModule::bind(spk::Widget* p_widget)
	{
		_widget = p_widget;
	}

	spk::Widget* IModule::widget()
	{
		return _widget;
	}

	const spk::Widget* IModule::widget() const
	{
		return _widget;
	}
}
