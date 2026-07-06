#pragma once

#include "core/observable_resource.hpp"
#include "structures/container/spk_observable_value.hpp"
#include "structures/widget/spk_panel.hpp"
#include "structures/widget/spk_text_label.hpp"
#include "ui/progress_bar_widget.hpp"

#include <array>
#include <memory>
#include <vector>

namespace pg
{
	class BattleContext;
	class BattleUnit;

	class TurnOrderStrip : public spk::Panel
	{
	public:
		static constexpr std::size_t Capacity = 12;

	private:
		spk::TextLabel _title;
		std::array<std::unique_ptr<spk::TextLabel>, Capacity> _names;
		std::array<std::unique_ptr<ProgressBarWidget>, Capacity> _bars;
		BattleContext *_context = nullptr;
		std::vector<ObservableFloatResource::Contract> _turnContracts;
		std::vector<spk::ObservableValue<float>::Contract> _rateContracts;
		std::vector<BattleUnit *> _order;
		std::size_t _refreshCount = 0;

	protected:
		void _onGeometryChange() override;

	public:
		explicit TurnOrderStrip(const std::string &p_name, spk::Widget *p_parent = nullptr);

		static float timeToReady(const BattleUnit &p_unit) noexcept;
		static std::vector<BattleUnit *> sortedUnits(const BattleContext &p_context);
		void bind(BattleContext &p_context);
		void unbind();
		void refresh();
		[[nodiscard]] const std::vector<BattleUnit *> &order() const noexcept;
		[[nodiscard]] std::size_t refreshCount() const noexcept;
	};
}
