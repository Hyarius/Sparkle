#pragma once

#include "battle/battle_event.hpp"
#include "structures/widget/spk_interface_window.hpp"
#include "structures/widget/spk_text_label.hpp"

#include <string>

namespace pg
{
	class BattleContext;
	class BattleUnit;

	class CreatureInfoWindow : public spk::InterfaceWindow<spk::Panel>
	{
	public:
		enum class ContentKind
		{
			Owned,
			WildEnemy,
			TrainerEnemy
		};

		struct Content
		{
			ContentKind kind = ContentKind::Owned;
			std::string stats;
			std::string abilities;
			std::string passives;
			std::string taming;
		};

	private:
		spk::TextLabel _stats;
		spk::TextLabel _abilities;
		spk::TextLabel _passives;
		spk::TextLabel _taming;
		BattleContext *_context = nullptr;
		BattleUnit *_unit = nullptr;
		spk::ContractProvider<const BattleEvent *>::Contract _battleEventContract;
		Content _content;

		void _refresh();

	protected:
		void _onGeometryChange() override;

	public:
		explicit CreatureInfoWindow(const std::string &p_name, spk::Widget *p_parent = nullptr);

		static Content contentFor(const BattleUnit &p_unit);
		void bind(BattleContext &p_context, BattleUnit &p_unit);
		void unbind();
		[[nodiscard]] BattleUnit *unit() const noexcept;
		[[nodiscard]] const Content &displayedContent() const noexcept;
	};
}
