#pragma once

#include <functional>
#include <string>

#include "structures/system/time/spk_duration.hpp"
#include "structures/system/time/spk_timer.hpp"
#include "structures/widget/spk_text_label.hpp"

namespace spk
{
	class DynamicTextLabel : public spk::TextLabel
	{
	public:
		using TextProducer = std::function<std::string()>;

		static inline const spk::Duration DefaultRefreshDuration = spk::Duration(1000.0L, spk::TimeUnit::Millisecond);

	private:
		TextProducer _textProducer;
		spk::Timer _refreshTimer = spk::Timer(DefaultRefreshDuration);
		bool _needsImmediateRefresh = true;

		void _refreshText();
		void _restartTimer();

	protected:
		void _onUpdate(const spk::UpdateTick &p_tick) override;

	public:
		explicit DynamicTextLabel(const std::string &p_name, spk::Widget *p_parent = nullptr);
		DynamicTextLabel(
			const std::string &p_name,
			const TextProducer &p_textProducer,
			spk::Widget *p_parent = nullptr);

		void setTextProducer(const TextProducer &p_textProducer);
		void setRefreshDuration(const spk::Duration &p_refreshDuration);
		void refresh();
	};
}
