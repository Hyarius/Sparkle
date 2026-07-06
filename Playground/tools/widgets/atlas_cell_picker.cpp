#include "tools/widgets/atlas_cell_picker.hpp"

#include <algorithm>
#include <utility>

namespace pg::tools
{
	AtlasCellPicker::AtlasCellPicker(
		const std::string &p_name,
		std::shared_ptr<spk::SpriteSheet> p_atlas,
		spk::Widget *p_parent) :
		spk::Widget(p_name, p_parent),
		_atlas(std::move(p_atlas)),
		_image(p_name + "/Cell", _atlas, this),
		_column(p_name + "/Column", this),
		_row(p_name + "/Row", this)
	{
		_column.setLimits(0, VoxelShape::AtlasColumns - 1);
		_row.setLimits(0, VoxelShape::AtlasRows - 1);
		_column.subscribeToEdition([this](int p_value) {
			_cell.column = p_value;
			_refresh(true);
		}).relinquish();
		_row.subscribeToEdition([this](int p_value) {
			_cell.row = p_value;
			_refresh(true);
		}).relinquish();
		_image.activate();
		_column.activate();
		_row.activate();
		_layout.setElementPadding({4, 4});
		_layout.addWidget(&_image, spk::Layout::SizePolicy::Fixed)->setSize({40, 40});
		_layout.addWidget(&_column);
		_layout.addWidget(&_row);
		configureMinimalSizeGenerator([this]() { return _layout.minimalSize(); });
		_refresh(false);
	}

	void AtlasCellPicker::_onGeometryChange()
	{
		_layout.setGeometry(geometry().atOrigin());
	}

	void AtlasCellPicker::_onMouseButtonPressedEvent(spk::MouseButtonPressedEvent &p_event)
	{
		if (p_event->button != spk::Mouse::Left || _atlas == nullptr ||
			!_image.absoluteGeometry().contains(p_event.device().position))
		{
			return;
		}
		const spk::Rect2D &rect = _image.absoluteGeometry();
		const int localX = p_event.device().position.x - rect.anchor.x;
		const int localY = p_event.device().position.y - rect.anchor.y;
		_cell.column = std::clamp(
			localX * VoxelShape::AtlasColumns / static_cast<int>(std::max(1u, rect.width())),
			0,
			VoxelShape::AtlasColumns - 1);
		_cell.row = std::clamp(
			localY * VoxelShape::AtlasRows / static_cast<int>(std::max(1u, rect.height())),
			0,
			VoxelShape::AtlasRows - 1);
		_refresh(true);
		p_event.consume();
	}

	void AtlasCellPicker::_refresh(bool p_notify)
	{
		_column.setValue(_cell.column);
		_row.setValue(_cell.row);
		if (_atlas != nullptr)
		{
			_image.setSection(_atlas->sprite({static_cast<unsigned int>(_cell.column), static_cast<unsigned int>(_cell.row)}));
		}
		if (p_notify && _callback)
		{
			_callback(_cell);
		}
	}

	void AtlasCellPicker::setCell(AtlasCell p_cell, bool p_notify)
	{
		_cell.column = std::clamp(p_cell.column, 0, VoxelShape::AtlasColumns - 1);
		_cell.row = std::clamp(p_cell.row, 0, VoxelShape::AtlasRows - 1);
		_refresh(p_notify);
	}

	void AtlasCellPicker::subscribeToSelection(std::function<void(const AtlasCell &)> p_callback)
	{
		_callback = std::move(p_callback);
	}

	const AtlasCell &AtlasCellPicker::cell() const noexcept
	{
		return _cell;
	}
}
