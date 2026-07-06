#pragma once

#include <string>

#include <sparkle.hpp>

namespace pg::tools
{
	class EditorPageState
	{
	private:
		bool _dirty = false;

	public:
		void markChanged() noexcept
		{
			_dirty = true;
		}

		void markSaved() noexcept
		{
			_dirty = false;
		}

		[[nodiscard]] bool hasUnsavedChanges() const noexcept
		{
			return _dirty;
		}
	};

	class IEditorPage : public spk::Widget
	{
	protected:
		EditorPageState _state;

	public:
		IEditorPage(const std::string &p_name, spk::Widget *p_parent) :
			spk::Widget(p_name, p_parent)
		{
		}

		~IEditorPage() override = default;

		[[nodiscard]] virtual std::string title() const = 0;
		[[nodiscard]] virtual bool hasUnsavedChanges() const
		{
			return _state.hasUnsavedChanges();
		}
		virtual void save() = 0;
		virtual void reload() = 0;
	};
}
