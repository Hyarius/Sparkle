#include "showcase_page_registry.hpp"

#include <utility>

#include "pages/overview_page.hpp"
#include "pages/placeholder_page.hpp"

namespace showcase
{
	void ShowcasePageRegistry::add(
		std::string p_category,
		std::string p_name,
		std::function<std::unique_ptr<ShowcasePage>()> p_factory)
	{
		_entries.push_back({std::move(p_category), std::move(p_name), std::move(p_factory)});
	}

	const std::vector<ShowcasePageEntry> &ShowcasePageRegistry::entries() const
	{
		return _entries;
	}

	bool ShowcasePageRegistry::empty() const
	{
		return _entries.empty();
	}

	namespace
	{
		// Registers a placeholder page describing a planned widget/feature and its blockers.
		void addPlaceholder(
			ShowcasePageRegistry &p_registry,
			std::string p_category,
			std::string p_name,
			std::string p_description,
			std::string p_status,
			std::vector<std::string> p_dependencies)
		{
			p_registry.add(
				std::move(p_category),
				p_name,
				[name = p_name,
				 description = std::move(p_description),
				 status = std::move(p_status),
				 dependencies = std::move(p_dependencies)]() -> std::unique_ptr<ShowcasePage> {
					return std::make_unique<PlaceholderPage>(name, description, status, dependencies);
				});
		}
	}

	void registerDefaultPages(ShowcasePageRegistry &p_registry)
	{
		p_registry.add("Widgets", "Overview", []() -> std::unique_ptr<ShowcasePage> { return std::make_unique<OverviewPage>(); });

		// Milestone 1 - existing widgets. Implemented incrementally; placeholders for now.
		addPlaceholder(p_registry, "Widgets", "Labels / Text", "spk::TextLabel, DynamicTextLabel and TextArea showcase.", "partial", {});
		addPlaceholder(p_registry, "Widgets", "Panels / Slice9", "spk::Panel and nine-slice WidgetStyle rendering.", "partial", {});
		addPlaceholder(p_registry, "Widgets", "Buttons / Commands", "PushButton, IconButton, CheckableIconButton, CommandPanel.", "partial", {});
		addPlaceholder(p_registry, "Widgets", "Images / Animation", "spk::ImageLabel and spk::AnimationLabel.", "partial", {});
		addPlaceholder(p_registry, "Widgets", "Sliders", "spk::SliderBar, later paired with progress bars.", "partial", {});
		addPlaceholder(p_registry, "Widgets", "Input", "spk::TextEdit, SpinBox and NumericSpinBox.", "partial", {});
		addPlaceholder(p_registry, "Widgets", "Scroll", "spk::ScrollBar and ScrollArea clipping.", "partial", {});

		// Milestone 2 - layouts.
		addPlaceholder(p_registry, "Layouts", "Layout Lab", "Move widgets between horizontal, vertical, grid and form layouts.", "missing", {});
		addPlaceholder(p_registry, "Layouts", "Linear Layout", "Horizontal and vertical distribution behavior.", "missing", {});
		addPlaceholder(p_registry, "Layouts", "Grid Layout", "Row/column sizing, growth and per-cell policies.", "missing", {});
		addPlaceholder(p_registry, "Layouts", "Form Layout", "Label/field alignment for property panels.", "missing", {});
		addPlaceholder(p_registry, "Layouts", "Size Policy Matrix", "Compare SizePolicy behavior across layout types.", "missing", {});
		addPlaceholder(p_registry, "Layouts", "Dynamic Mutation", "Stress-test runtime structural layout changes.", "missing", {});
		addPlaceholder(p_registry, "Layouts", "Nested Layouts", "Layout-in-layout composition.", "missing", {});

		// Milestone 3 - diagnostics.
		addPlaceholder(p_registry, "Diagnostics", "Events / Focus", "Input routing, focus, hover and keyboard activation.", "missing", {});
		addPlaceholder(p_registry, "Diagnostics", "Clipping / Z-order", "Clipping boundaries, nested clipping and overlay ordering.", "missing", {});
		addPlaceholder(p_registry, "Diagnostics", "Theme / Style", "Stress-test WidgetStyle usage and state styling.", "missing", {});
		addPlaceholder(p_registry, "Diagnostics", "Metrics", "Runtime update/render timing and widget counts.", "missing", {});

		// Planned widgets.
		addPlaceholder(p_registry, "Planned", "Tooltip", "Widget-owned content rendered through an overlay layer.", "missing", {"OverlayHost"});
		addPlaceholder(p_registry, "Planned", "Popup / Overlay", "Generic overlay layer for popups, dropdowns and modals.", "missing", {});
		addPlaceholder(p_registry, "Planned", "Tabs", "spk::TabBar and spk::TabView.", "missing", {});
		addPlaceholder(p_registry, "Planned", "Dropdown / ComboBox", "List picker backed by the popup layer.", "missing", {"OverlayHost", "ListView", "Focus navigation"});
		addPlaceholder(p_registry, "Planned", "Checkbox / Radio / Toggle", "Standard boolean and exclusive-selection controls.", "missing", {});
		addPlaceholder(p_registry, "Planned", "ProgressBar", "Non-interactive horizontal/vertical progress widget.", "missing", {});
	}
}
