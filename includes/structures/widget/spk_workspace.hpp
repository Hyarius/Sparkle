#pragma once

#include <algorithm>
#include <string>
#include <type_traits>

#include "structures/widget/spk_action_bar.hpp"
#include "structures/widget/spk_widget.hpp"

namespace spk
{
	template <typename TContentType>
		requires std::is_base_of_v<spk::Widget, TContentType>
	class Workspace : public spk::Widget
	{
	private:
		TContentType _content;
		spk::MenuBar _menuBar;

	protected:
		void _onGeometryChange() override
		{
			_menuBar.setGeometry(geometry().atOrigin());

			const unsigned int contentHeight =
				(geometry().height() > _menuBar.height()) ? geometry().height() - _menuBar.height() : 0;

			_content.setGeometry(spk::Rect2D(0, static_cast<int>(_menuBar.height()), geometry().width(), contentHeight));
		}

	public:
		explicit Workspace(const std::string &p_name, spk::Widget *p_parent = nullptr) :
			spk::Widget(p_name, p_parent),
			_content(p_name + "::content", this),
			_menuBar(p_name + "::menuBar", this)
		{
			_content.activate();
			_menuBar.activate();

			configureMinimalSizeGenerator([this]() {
				const spk::Vector2UInt menuBarSize = _menuBar.minimalSize();
				const spk::Vector2UInt contentSize = _content.minimalSize();

				return spk::Vector2UInt(
					std::max(menuBarSize.x, contentSize.x),
					_menuBar.height() + contentSize.y);
			});

			activate();
		}

		[[nodiscard]] spk::MenuBar &menuBar()
		{
			return _menuBar;
		}

		[[nodiscard]] const spk::MenuBar &menuBar() const
		{
			return _menuBar;
		}

		[[nodiscard]] TContentType &content()
		{
			return _content;
		}

		[[nodiscard]] const TContentType &content() const
		{
			return _content;
		}
	};
}
