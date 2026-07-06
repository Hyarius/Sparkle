#pragma once

#include "voxel/voxel_shape.hpp"

#include <functional>
#include <memory>
#include <string>

#include <sparkle.hpp>

namespace pg::tools
{
	class AtlasCellPicker : public spk::Widget
	{
	private:
		std::shared_ptr<spk::SpriteSheet> _atlas;
		spk::ImageLabel _image;
		spk::HorizontalLayout _layout;
		spk::SpinBox<int> _column;
		spk::SpinBox<int> _row;
		AtlasCell _cell;
		std::function<void(const AtlasCell &)> _callback;

		void _onGeometryChange() override;
		void _onMouseButtonPressedEvent(spk::MouseButtonPressedEvent &p_event) override;
		void _refresh(bool p_notify);

	public:
		AtlasCellPicker(
			const std::string &p_name,
			std::shared_ptr<spk::SpriteSheet> p_atlas,
			spk::Widget *p_parent = nullptr);

		void setCell(AtlasCell p_cell, bool p_notify = false);
		void subscribeToSelection(std::function<void(const AtlasCell &)> p_callback);
		[[nodiscard]] const AtlasCell &cell() const noexcept;
	};
}
