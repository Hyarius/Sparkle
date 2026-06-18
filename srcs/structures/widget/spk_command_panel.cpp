#include "structures/widget/spk_command_panel.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace spk
{
	CommandPanel::CommandPanel(const std::string& p_name, spk::Widget* p_parent) :
		spk::Widget(p_name, p_parent),
		_spacer(p_name + "::spacer", this)
	{
		_layout.setElementPadding({10, 10});
		_composeLayout();

		sizeHint().configureMinimalGenerator(
			[this]()
			{
				spk::Vector2UInt result = {0, 0};

				if (_buttons.empty() == false)
				{
					for (const auto& [buttonName, button] : _buttons)
					{
						const spk::Vector2UInt buttonSize = button->minimalSize();
						result.x += buttonSize.x;
						result.y = std::max(result.y, buttonSize.y);
					}

					result.x += _layout.elementPadding().x * static_cast<unsigned int>(_buttons.size() - 1);
				}

				return result;
			});

		activate();
	}

	void CommandPanel::_onGeometryChange()
	{
		_layout.setGeometry(geometry().atOrigin());
	}

	void CommandPanel::_composeLayout()
	{
		_layout.clear();

		if (_sizePolicy == spk::Layout::SizePolicy::Minimum || _sizePolicy == spk::Layout::SizePolicy::VerticalExtend)
		{
			_layout.addWidget(&_spacer, spk::Layout::SizePolicy::Extend);
		}

		for (auto* button : _orderedButtons)
		{
			_layout.addWidget(button, _sizePolicy);
		}

		_onGeometryChange();
	}

	spk::PushButton* CommandPanel::addButton(const std::string& p_name, std::string_view p_label)
	{
		if (_buttons.contains(p_name) == true)
		{
			throw std::runtime_error("Button [" + p_name + "] already exists in the command panel [" + name() + "]");
		}

		std::unique_ptr<spk::PushButton>& newButton =
			_buttons.emplace(p_name, std::make_unique<spk::PushButton>(name() + "::" + p_name, p_label, this)).first->second;

		_orderedButtons.push_back(newButton.get());

		_composeLayout();

		return newButton.get();
	}

	spk::PushButton* CommandPanel::button(const std::string& p_name)
	{
		if (_buttons.contains(p_name) == false)
		{
			throw std::runtime_error("Button [" + p_name + "] doesn't exist in the command panel [" + name() + "]");
		}
		return _buttons.at(p_name).get();
	}

	const spk::PushButton* CommandPanel::button(const std::string& p_name) const
	{
		if (_buttons.contains(p_name) == false)
		{
			throw std::runtime_error("Button [" + p_name + "] doesn't exist in the command panel [" + name() + "]");
		}
		return _buttons.at(p_name).get();
	}

	void CommandPanel::removeButton(const std::string& p_name)
	{
		auto it = _buttons.find(p_name);
		if (it == _buttons.end())
		{
			return;
		}

		_orderedButtons.erase(std::remove(_orderedButtons.begin(), _orderedButtons.end(), it->second.get()), _orderedButtons.end());
		_buttons.erase(it);

		_composeLayout();
	}

	size_t CommandPanel::nbButton() const
	{
		return _buttons.size();
	}

	CommandPanel::Contract CommandPanel::subscribe(const std::string& p_name, Callback p_callback)
	{
		return button(p_name)->subscribeToClick(std::move(p_callback));
	}

	void CommandPanel::setSizePolicy(spk::Layout::SizePolicy p_sizePolicy)
	{
		_sizePolicy = p_sizePolicy;
		_composeLayout();
	}

	spk::Layout::SizePolicy CommandPanel::sizePolicy() const
	{
		return _sizePolicy;
	}

	void CommandPanel::setElementPadding(const spk::Vector2UInt& p_padding)
	{
		_layout.setElementPadding(p_padding);
		_onGeometryChange();
	}
}
