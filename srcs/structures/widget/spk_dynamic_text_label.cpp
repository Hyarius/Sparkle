#include "structures/widget/spk_dynamic_text_label.hpp"

namespace spk
{
	DynamicTextLabel::DynamicTextLabel(const std::string& p_name, spk::Widget* p_parent) :
		spk::TextLabel(p_name, p_parent)
	{
	}

	DynamicTextLabel::DynamicTextLabel(const std::string& p_name, const TextProducer& p_textProducer, spk::Widget* p_parent) :
		spk::TextLabel(p_name, p_parent)
	{
		setTextProducer(p_textProducer);
	}

	void DynamicTextLabel::_refreshText()
	{
		if (static_cast<bool>(_textProducer) == false)
		{
			return;
		}

		setText(_textProducer());
	}

	void DynamicTextLabel::_restartTimer()
	{
		const spk::Duration expectedDuration = _refreshTimer.expectedDuration();
		_refreshTimer = spk::Timer(expectedDuration);
		_refreshTimer.start();
	}

	void DynamicTextLabel::_onUpdate(const spk::UpdateTick& p_tick)
	{
		spk::TextLabel::_onUpdate(p_tick);

		if (_needsImmediateRefresh == true)
		{
			_refreshText();
			_restartTimer();
			_needsImmediateRefresh = false;
			return;
		}

		if (_refreshTimer.hasTimedOut() == true)
		{
			_refreshText();
			_restartTimer();
		}
	}

	void DynamicTextLabel::setTextProducer(const TextProducer& p_textProducer)
	{
		_textProducer = p_textProducer;
		if (_textProducer != nullptr)
		{
			_refreshText();
			_restartTimer();
			_needsImmediateRefresh = false;
		}
	}

	void DynamicTextLabel::setRefreshDuration(const spk::Duration& p_refreshDuration)
	{
		_refreshTimer = spk::Timer(p_refreshDuration);
		_refreshTimer.start();
	}

	void DynamicTextLabel::refresh()
	{
		_refreshText();
		_restartTimer();
		_needsImmediateRefresh = false;
	}
}
