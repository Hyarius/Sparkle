#pragma once

#include "spk_widget.hpp"

namespace spk
{
	class IModule
	{
	private:
		spk::Widget* _widget = nullptr;

	public:
		virtual ~IModule();

		virtual void bind(spk::Widget* p_widget);

		[[nodiscard]] spk::Widget* widget();
		[[nodiscard]] const spk::Widget* widget() const;
	};
}
