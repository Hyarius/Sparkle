# Sparkle UI Showcase Plan

## Goal

Create a dedicated showcase program for Sparkle UI widgets, similar in spirit to the reference "Neotolis UI Showcase": a single interactive application that presents every widget, documents its behavior, exposes live properties where useful, and makes missing UI primitives visible as concrete implementation tasks.

The showcase should not be a marketing page. It should be an engineering/demo tool for validating Sparkle's widget library, themes, interaction behavior, clipping, focus, scroll, popups, modals, layout recomputation, runtime widget movement, and future UI components.

The showcase should be useful even before every missing widget exists. Missing primitives should appear as explicit placeholder pages or status cards, so that the showcase also acts as a roadmap.

---

## 1. Guiding Principles

1. **Start with existing widgets.**
   The first milestone must compile and run using widgets that already exist in Sparkle.

2. **Make gaps visible instead of blocking on them.**
   Missing widgets such as tabs, combo boxes, tooltips, checkboxes, radio buttons, and progress bars should initially appear as placeholder cards with status, dependencies, and expected behavior.

3. **Treat layout behavior as a first-class feature.**
   Sparkle has several layout primitives. The showcase should validate not only individual widgets, but also how widgets react when placed in layouts, moved between layouts, resized, removed, or assigned different size policies.

4. **Prefer live diagnostics over static examples.**
   Each page should expose useful state: hovered widget, focused widget, selected demo widget, geometry, size policy, draw/update/render timing, clipping boundaries, and event logs where relevant.

5. **Use public APIs only.**
   The showcase should validate Sparkle as a user would use it. It should not depend on private widget internals.

6. **Keep implementation incremental.**
   The showcase should become useful quickly, then grow as missing UI primitives are implemented.

7. **Build as a standalone consumer project.**
   `UIShowcase` may live inside the Sparkle repository, but it should configure as its own project and import `Sparkle::Sparkle` itself. The Sparkle root `CMakeLists.txt` should not add or control it.

---

## 2. Required Widgets And Feature Gaps

### 2.1 Existing Widgets That Can Be Showcased Immediately

Sparkle already has enough widget coverage to build a first useful version of the showcase.

| Showcase area | Existing Sparkle support | Recommended page treatment |
| --- | --- | --- |
| Labels | `spk::TextLabel`, `spk::DynamicTextLabel`, `spk::TextArea` | Dedicated labels/text page. |
| Panels / Slice9 | `spk::Panel`, `spk::WidgetStyle`, nine-slice rendering | Dedicated panels and slice9 page. |
| Buttons | `spk::PushButton`, `spk::IconButton`, `spk::CheckableIconButton`, `spk::CommandPanel` | Dedicated buttons page. |
| Images / animation | `spk::ImageLabel`, `spk::AnimationLabel` | Dedicated images and animation page. |
| Sliders | `spk::SliderBar` | Dedicated sliders page, later paired with progress bars. |
| Scroll | `spk::ScrollBar`, `spk::ScrollArea<T>` | Dedicated scroll and clipping page. |
| Input | `spk::TextEdit`, `spk::SpinBox<T>`, `spk::NumericSpinBox<T>` | Dedicated input page. |
| Menus | `spk::MenuBar` | Dedicated menu/workspace page. |
| Dialog-like panels | `spk::PromptPanel`, `spk::MessageBox`, `spk::InformationMessageBox`, `spk::RequestMessageBox` | Dedicated dialogs page, plus modal-gap notes. |
| Layouts | `spk::HorizontalLayout`, `spk::VerticalLayout`, `spk::GridLayout`, `spk::FormLayout` | Full layout section, not a single generic page. |
| Debug text | `spk::DebugOverlay` | Diagnostics page. |
| Window-style container | `spk::IInterfaceWindow`, `spk::InterfaceWindow<T>` | Window/container page. |
| Container / viewport | `spk::ContainerWidget`, if available in the current widget tree | Container and viewport page. |
| Resizable widgets | `spk::ScalableWidget`, if available in the current widget tree | Scalable/resizable widget page. |
| Application shell | `spk::Workspace<T>`, `spk::Screen`, if available in the current widget tree | Application structure page. |

The first showcase milestone should use these before creating new core widgets.

### 2.2 Missing Or Incomplete Features

These are the gaps to close to reach a more complete showcase.

| Feature | Current state | Recommended implementation |
| --- | --- | --- |
| Global tooltips | No first-class tooltip API. | Add tooltip metadata to `spk::Widget`: `setTooltip(...)`, `tooltip()`, `clearTooltip()`. Render active tooltips through a root-level tooltip system backed by the overlay layer. |
| Popup / overlay layer | `MenuBar` handles its own dropdown geometry, but there is no generic overlay layer. | Add `spk::OverlayHost` or `spk::PopupLayer` for tooltips, dropdowns, context menus, and modals. Implement this before `TooltipHost`. |
| Theme / palette switch | `WidgetStyle::Collection` stores concrete styles, not semantic theme tokens. | Add `spk::Palette` / `spk::Theme` with named colors and style factories. The showcase can start by editing `WidgetStyle::Collection`, then migrate. |
| Tabs | No `TabBar` or `TabView`. | Add `spk::TabBar` for selectable icon/text tabs and `spk::TabView` for content switching. |
| Dropdown / ComboBox | No dropdown or list picker widget. | Add `spk::ListView` and `spk::ComboBox`, backed by the popup layer and optional `ScrollArea`. |
| Checkbox | `CheckableIconButton` exists, but no standard checkbox with text. | Add `spk::CheckBox`, likely composed from `CheckableIconButton` + `TextLabel`. |
| Radio buttons | No radio group abstraction. | Add `spk::RadioButton` and `spk::RadioGroup`, with exclusive selection contracts. |
| Toggle switch | No switch-style boolean widget. | Add `spk::ToggleSwitch`, probably composed from panels and a movable knob. |
| Progress bar | No non-interactive progress widget. | Add `spk::ProgressBar` with horizontal/vertical orientation and fill modes: stretch, crop/clip, possibly animated/striped. |
| True modal stack | Message boxes exist, but there is no backdrop, click-blocking stack, Escape routing, or nested modal management. | Add `spk::ModalHost`, using `PromptPanel` / `MessageBox` visuals initially. |
| Text selection / clipboard | `TextEdit` supports cursor, placeholder, obscured mode, validation, and UTF-8 glyph input. | Add selection range, Shift+arrows, drag select, double-click select, Ctrl+A/C/X/V, clipboard integration, and Tab focus traversal. |
| Hover style | `PushButton` tracks hover but has no separate hover style. | Add optional hover style for `PushButton` and related controls. |
| Keyboard focus traversal | Focus exists partially or indirectly, but there is no full showcase-facing traversal model. | Add Tab / Shift+Tab traversal, Enter/Space activation where appropriate, Escape routing, and focus debugging. |
| Layout debug overlay | Layouts compute geometry, but visual layout debugging is not first-class. | Add optional debug rectangles for layout cells, padding, content bounds, and final widget geometry. |
| Radial controls | No radial menu/progress/slider widget. | Treat as a later optional widget family once core showcase pages are stable. |

---

## 3. Layout Showcase Requirements

Layouts should be treated as a major showcase section, not as a small subsection of the widget gallery.

The goal is to validate how Sparkle reacts when child widgets are:

- added to a layout;
- removed from a layout;
- moved from one layout to another;
- assigned a different size policy;
- resized manually;
- placed in nested layouts;
- placed in a grid cell;
- placed in a scrollable or clipped area;
- made larger than their parent;
- given changing text/content at runtime.

The layout pages should explicitly display:

| Diagnostic | Purpose |
| --- | --- |
| Selected widget name | Know which widget is being edited. |
| Current parent widget | Validate ownership and hierarchy changes. |
| Current layout | Validate movement between layouts. |
| Current size policy | Validate layout rules. |
| Requested size | Compare requested size with computed geometry. |
| Final geometry | Validate computed position and size. |
| Padding | Validate spacing rules. |
| Hover/focus state | Validate interaction after moving widgets. |
| Clipping state | Validate behavior inside scroll areas or containers. |

### 3.1 Layout Lab Page

The `Layout Lab` page is the main interactive layout validation page.

Purpose:

> Validate how layouts adapt when the same widgets are moved between horizontal, vertical, grid, form, nested, and scrollable containers.

Recommended visual structure:

```text
+--------------------------------------------------------------------------------+
| Layout Lab                                                                     |
+----------------------+----------------------+----------------------+------------+
| Available widgets    | Horizontal layout    | Vertical layout      | Properties |
|                      | [A] [B] [C]          | [D]                  |            |
| Button A             |                      | [E]                  | Selected   |
| Button B             +----------------------+----------------------+ widget     |
| Label C              | Grid layout                                 | controls   |
| Panel D              | +----------+----------+----------+         |            |
| TextEdit E           | |          |          |          |         |            |
|                      | +----------+----------+----------+         |            |
+----------------------+---------------------------------------------+------------+
```

Recommended controls:

```text
Selected widget: Button A

Move to:
[ HorizontalLayout ]
[ VerticalLayout   ]
[ GridLayout       ]
[ FormLayout       ]
[ Unmanaged area   ]

Size policy:
[ Fixed            ]
[ Minimum          ]
[ Maximum          ]
[ Extend           ]
[ HorizontalExtend ]
[ VerticalExtend   ]

Requested size:
Width  [120]
Height [32]

Padding:
X [8]
Y [8]

Actions:
[ Remove selected widget ]
[ Reinsert selected widget ]
[ Swap selected with next ]
[ Randomize policies ]
[ Reset layout lab ]
```

Validation cases:

- moving from `HorizontalLayout` to `VerticalLayout`;
- moving from `VerticalLayout` to a specific `GridLayout` cell;
- moving from a layout to an unmanaged area;
- moving from a grid cell back to a linear layout;
- changing size policy while the widget is already attached;
- removing and reinserting widgets;
- changing padding at runtime;
- resizing the parent container;
- ensuring hover/focus state remains coherent after movement.

The page should not animate transitions. The goal is to observe layout recomputation, not visual motion.

### 3.2 Linear Layout Page

Purpose:

> Showcase pure horizontal and vertical distribution behavior.

Widgets to demonstrate:

- `spk::HorizontalLayout`;
- `spk::VerticalLayout`;
- nested horizontal-inside-vertical layouts;
- nested vertical-inside-horizontal layouts.

Recommended demos:

```text
Horizontal layout:
[Fixed 80] [Minimum] [Extend] [Extend]

Vertical layout:
[Fixed 40]
[Minimum]
[VerticalExtend]
[Maximum]
```

Controls:

- switch orientation;
- add child;
- remove child;
- change selected child size policy;
- change selected child requested size;
- change layout padding;
- toggle parent resizing;
- toggle debug geometry overlay.

Important cases:

| Case | Expected observation |
| --- | --- |
| One `Extend` item | Extra space should go mostly or entirely to that item. |
| Multiple `Extend` items | Extra space should be distributed predictably. |
| No extendable item | The fallback distribution should be visible. |
| Oversized fixed child | The result should reveal overflow or compression behavior. |
| Nested layout | Parent and child layout recomputation should remain coherent. |

### 3.3 Grid Layout Page

Purpose:

> Validate row/column sizing, automatic grid growth, fixed-grid variants, padding, empty cells, and size policies per cell.

Recommended controls:

```text
Grid controls:
Rows:    [ + row ] [ - row ]
Columns: [ + col ] [ - col ]

Selected cell:
Column [1]
Row    [2]

Cell content:
[None / Button / Label / Panel / TextEdit / Nested layout]

Cell policy:
[Fixed / Minimum / Maximum / Extend / HorizontalExtend / VerticalExtend]
```

Important demos:

- sparse grid;
- fixed-size widgets mixed with extend widgets;
- one extendable column;
- multiple extendable columns;
- one extendable row;
- multiple extendable rows;
- grid inside a scroll area;
- grid with empty cells;
- grid with nested linear layout in one cell;
- fixed-row grid;
- fixed-column grid;
- fixed-size grid.

### 3.4 Form Layout Page

Purpose:

> Validate label/field alignment and typical editor/property-panel forms.

Recommended demo:

```text
Username     [ TextEdit       ]
Age          [ NumericSpinBox ]
Volume       [ SliderBar      ]
Enabled      [ CheckableIcon  ]
Very long label used for overflow testing [ TextEdit ]
```

Controls:

- add row;
- remove selected row;
- toggle long labels;
- toggle disabled fields;
- change label size policy;
- change field size policy;
- change row padding;
- test label overflow;
- test field expansion.

This page should also become the basis for the showcase's own right-side properties panel.

### 3.5 Size Policy Matrix Page

Purpose:

> Make `Layout::SizePolicy` behavior visible and comparable across layout types.

Policies to compare:

```cpp
Fixed
Minimum
Maximum
Extend
HorizontalExtend
VerticalExtend
```

Recommended structure:

```text
                     HorizontalLayout     VerticalLayout     GridLayout
Fixed                [demo]               [demo]             [demo]
Minimum              [demo]               [demo]             [demo]
Maximum              [demo]               [demo]             [demo]
Extend               [demo]               [demo]             [demo]
HorizontalExtend     [demo]               [demo]             [demo]
VerticalExtend       [demo]               [demo]             [demo]
```

Each cell should render the same kind of child widget under the same parent size, so differences are immediately visible.

### 3.6 Dynamic Layout Mutation Page

Purpose:

> Stress-test runtime structural changes without animation.

Actions:

```text
[Move selected widget to horizontal layout]
[Move selected widget to vertical layout]
[Move selected widget to grid cell]
[Move selected widget to form row]
[Remove selected widget]
[Reinsert selected widget]
[Swap Button A and Button B]
[Change all widgets to Extend]
[Change all widgets to Fixed]
[Randomize policies]
[Clear all layouts]
[Rebuild default state]
```

The page should show before/after state:

| Field | Description |
| --- | --- |
| Previous layout | Layout that contained the widget before the operation. |
| New layout | Layout that contains the widget after the operation. |
| Previous geometry | Geometry before recomputation. |
| New geometry | Geometry after recomputation. |
| Previous policy | Policy before mutation. |
| New policy | Policy after mutation. |
| Focus survived | Whether focus remained coherent. |
| Hover survived | Whether hover state remained coherent. |

Potential operation sequence:

```cpp
sourceLayout.removeWidget(&button);
targetLayout.addWidget(&button, spk::Layout::SizePolicy::Extend);
```

For grid placement:

```cpp
sourceLayout.removeWidget(&button);
gridLayout.setWidget(column, row, &button, spk::Layout::SizePolicy::Extend);
```

The interesting result is not the movement itself. The page exists to verify that Sparkle correctly updates hierarchy, geometry, layout ownership, hover state, focus state, and clipping after structural changes.

### 3.7 Nested Layout Page

Purpose:

> Validate layout composition and layout-in-layout behavior.

Recommended examples:

```text
VerticalLayout
├── Header panel
├── HorizontalLayout
│   ├── Left panel
│   ├── Center panel
│   └── Right panel
└── Footer panel
```

```text
GridLayout
├── cell 0,0: VerticalLayout
├── cell 1,0: FormLayout
├── cell 0,1: ScrollArea
└── cell 1,1: HorizontalLayout
```

Controls:

- add nested layout;
- remove nested layout;
- move widget from nested child layout to parent layout;
- move widget from parent layout to nested child layout;
- resize root container;
- toggle debug rectangles per layout depth.

---

## 4. Proposed Showcase Pages

### 4.1 Navigation Structure

Recommended navigation grouping:

```text
Widgets
  Overview
  Labels / Text
  Panels / Slice9
  Buttons / Commands
  Images / Animation
  Sliders
  Input
  Scroll

Layouts
  Layout Lab
  Linear Layout
  Grid Layout
  Form Layout
  Size Policy Matrix
  Dynamic Layout Mutation
  Nested Layouts

Application Shell
  Menu / Workspace
  Dialogs / Message Boxes
  Interface Windows
  Container / Viewport
  Scalable Widgets
  Screens

Diagnostics
  Events / Focus
  Clipping / Z-order
  Theme / Style
  Metrics / Debug Overlay

Planned Widgets
  Tooltip
  Popup / Overlay
  Tabs
  Dropdown / ComboBox
  Checkbox / Radio / Toggle
  ProgressBar
  Radial Controls
```

### 4.2 Page File Layout

```text
UIShowcase/
  CMakeLists.txt
  planning.md
  includes/
    showcase_page.hpp
    showcase_page_registry.hpp
    showcase_root.hpp
    showcase_theme.hpp
    showcase_metrics.hpp
    showcase_status.hpp
    pages/
      overview_page.hpp
      labels_page.hpp
      panels_slice9_page.hpp
      buttons_page.hpp
      images_animation_page.hpp
      sliders_page.hpp
      input_page.hpp
      scroll_page.hpp
      layout_lab_page.hpp
      linear_layout_page.hpp
      grid_layout_page.hpp
      form_layout_page.hpp
      size_policy_page.hpp
      dynamic_layout_mutation_page.hpp
      nested_layout_page.hpp
      menu_workspace_page.hpp
      dialogs_page.hpp
      interface_window_page.hpp
      container_viewport_page.hpp
      scalable_widget_page.hpp
      screen_page.hpp
      events_focus_page.hpp
      clipping_z_order_page.hpp
      theme_style_page.hpp
      diagnostics_page.hpp
      missing_widgets_page.hpp
      tooltip_page.hpp
      popup_overlay_page.hpp
      tabs_page.hpp
      dropdown_page.hpp
      toggles_page.hpp
      progress_page.hpp
  srcs/
    main.cpp
    showcase_page_registry.cpp
    showcase_root.cpp
    showcase_theme.cpp
    showcase_metrics.cpp
    showcase_status.cpp
    pages/
      overview_page.cpp
      labels_page.cpp
      panels_slice9_page.cpp
      buttons_page.cpp
      images_animation_page.cpp
      sliders_page.cpp
      input_page.cpp
      scroll_page.cpp
      layout_lab_page.cpp
      linear_layout_page.cpp
      grid_layout_page.cpp
      form_layout_page.cpp
      size_policy_page.cpp
      dynamic_layout_mutation_page.cpp
      nested_layout_page.cpp
      menu_workspace_page.cpp
      dialogs_page.cpp
      interface_window_page.cpp
      container_viewport_page.cpp
      scalable_widget_page.cpp
      screen_page.cpp
      events_focus_page.cpp
      clipping_z_order_page.cpp
      theme_style_page.cpp
      diagnostics_page.cpp
      missing_widgets_page.cpp
      tooltip_page.cpp
      popup_overlay_page.cpp
      tabs_page.cpp
      dropdown_page.cpp
      toggles_page.cpp
      progress_page.cpp
```

Not every file needs to exist in milestone 0. Planned pages can initially be placeholder pages registered in the page registry.

---

## 5. Showcase Application Layout

The main window should use this structure:

```text
+--------------------------------------------------------------------------------+
| Sparkle UI Showcase  [Theme: Dark]  draw calls / update ms / render ms  hints   |
+--------------------------------------------------------------------------------+
| Navigation          | Selected page content                         | Properties |
|                     |                                               |            |
| Widgets             | Page title                                    | Live demo  |
|   Labels            | Description                                   | controls   |
|   Buttons           | Examples                                      |            |
|   Images            |                                               | Metrics    |
| Layouts             |                                               |            |
|   Layout Lab        |                                               |            |
|   Linear Layout     |                                               |            |
|   Grid Layout       |                                               |            |
|   Form Layout       |                                               |            |
| Diagnostics         |                                               |            |
| Planned Widgets     |                                               |            |
+--------------------------------------------------------------------------------+
```

Implementation widgets:

- `ShowcaseRoot : spk::Widget`
- top bar: `spk::Panel`, `spk::TextLabel`, `spk::PushButton`
- left navigation: `spk::ScrollArea<NavigationContent>`
- center content: `spk::ScrollArea<PageContent>`
- right properties: `spk::Panel` or `spk::ScrollArea<PropertiesContent>`
- overlay: future `spk::OverlayHost` or `spk::PopupLayer`

Recommended proportions:

| Area | Approximate width |
| --- | --- |
| Navigation | 220 px |
| Properties | 280-340 px |
| Content | Remaining space |
| Top bar | 36-48 px height |

---

## 6. Page Model

The showcase should use a small page abstraction so pages can be added without turning `ShowcaseRoot` into a large switch statement.

Possible interface:

```cpp
class ShowcasePage
{
public:
    virtual ~ShowcasePage() = default;

    [[nodiscard]] virtual std::string_view title() const = 0;
    [[nodiscard]] virtual std::string_view description() const = 0;

    virtual void buildContent(spk::Widget& p_parent) = 0;
    virtual void buildProperties(spk::Widget& p_parent) = 0;
};
```

Possible registry entry:

```cpp
struct ShowcasePageEntry
{
    std::string category;
    std::string name;
    std::function<std::unique_ptr<ShowcasePage>()> factory;
};
```

Every page should be able to expose a small status block:

```text
Status: implemented / partial / missing
Depends on: none / OverlayHost / FocusTraversal / Theme
Validated behavior:
- mouse interaction
- keyboard interaction
- layout behavior
- clipping behavior
- theme behavior
```

---

## 7. Status Badges

Each widget or feature card should have explicit engineering status.

Example for an existing widget:

```text
PushButton
Status: implemented
Coverage:
- normal click: yes
- hover: partial
- disabled state: pending
- keyboard activation: pending
- tooltip: pending
- theme tokens: pending
```

Example for a missing widget:

```text
ComboBox
Status: missing
Blocked by:
- OverlayHost
- ListView
- Focus navigation
Planned behavior:
- click opens popup
- keyboard navigation
- closes on outside click
- selection contract
```

The status cards make the showcase useful as a roadmap and as a validation dashboard.

---

## 8. Overlay, Popup, Tooltip, And Modal Design

The overlay infrastructure should be implemented before the tooltip system.

Recommended conceptual split:

| Concept | Meaning |
| --- | --- |
| Overlay | Visual layer above normal UI. |
| Popup | Overlay entry that may close on outside click or Escape. |
| Tooltip | Passive popup, no focus, no click blocking. |
| Modal | Overlay entry that blocks interaction behind it. |
| Dropdown | Popup attached to an anchor widget. |
| Context menu | Popup attached to a mouse position. |
| Message box | Modal content widget. |

Possible behavior enum:

```cpp
enum class OverlayBehavior
{
    Passive,
    Popup,
    Modal
};
```

Possible host:

```cpp
class OverlayHost : public spk::Widget
{
public:
    void showTooltip(/* tooltip description */);
    void showPopup(/* popup description */);
    void showModal(/* modal description */);

    void clearTooltip();
    void closeTopPopup();
    void closeTopModal();
};
```

Tooltips should then be implemented as one consumer of the overlay layer, not as a separate special-case rendering system.

### Tooltip API Notes

Tooltips should be implemented once at the `spk::Widget` level so every widget can expose tooltip content.

A tooltip should not be limited to plain text metadata. It should be a composable widget container, allowing the user to build any tooltip content using normal Sparkle widgets: panels, labels, images, layouts, icons, shortcuts, status rows, etc.

The tooltip content is logically attached to the tooltiped widget, but it must be rendered through an overlay/root layer so it can appear outside the bounds of the widget that owns it.

Suggested API shape:

```cpp
class Widget
{
public:
    template <typename TTooltip, typename... TArguments>
        requires std::derived_from<TTooltip, spk::Widget>
    TTooltip& setTooltip(TArguments&&... p_arguments);

    void setTooltip(std::unique_ptr<spk::Widget> p_tooltip);
    void clearTooltip();

    [[nodiscard]] spk::Widget* tooltip() noexcept;
    [[nodiscard]] const spk::Widget* tooltip() const noexcept;
    [[nodiscard]] bool hasTooltip() const noexcept;
};
```

Possible tooltip-specific wrapper:
```cpp
class TooltipContainer : public spk::ContainerWidget
{
private:
    float _delay = 0.45f;
    spk::Vector2Int _offset = {12, 12};

public:
    void setDelay(float p_delay);
    void setOffset(const spk::Vector2Int& p_offset);

    [[nodiscard]] float delay() const noexcept;
    [[nodiscard]] const spk::Vector2Int& offset() const noexcept;
};
```

Suggested behavior:
- tooltip content is a normal widget tree
- tooltip content is owned or referenced by the tooltiped widget
- TooltipHost inspects hover state and selects the active tooltip
- the active tooltip is temporarily presented in the overlay layer
- tooltip content can use layouts, panels, labels, images, and any other widgets
- tooltip rendering happens above normal widgets
- tooltip rendering should not be clipped by the tooltiped widget
- tooltips do not consume mouse events by default
- tooltips never block clicks by default
- tooltip placement clamps to the window
- tooltip placement flips when near edges
- hiding a tooltip should not destroy its content unless the owning widget is destroyed


## Important design distinction

You want this:

```text
Button
└── owns TooltipContainer
    └── Panel
        └── VerticalLayout
            ├── TextLabel("Delete file")
            ├── TextLabel("This action cannot be undone.")
            └── TextLabel("Shortcut: Del")
```

But rendered like this:

```text
Root / OverlayHost
└── active tooltip visual instance
```

So the tooltip is logically owned by the widget, but visually hosted by the overlay layer.

That is the correct model.

Better mental model

I would describe Sparkle tooltips as:

Widget-owned content, overlay-rendered presentation.

Not:

Widget-owned string metadata.
Example usage

Something like this would be much better:

```cpp
auto& tooltip = button.setTooltip<spk::TooltipContainer>();

auto& panel = tooltip.setContent<spk::Panel>();
auto& layout = panel.addLayout<spk::VerticalLayout>();

layout.addWidget(new spk::TextLabel("Delete file"));
layout.addWidget(new spk::TextLabel("This action cannot be undone."));
layout.addWidget(new spk::TextLabel("Shortcut: Del"));
```

Or, depending on your ownership model:

```cpp
auto tooltip = std::make_unique<spk::TooltipContainer>();

auto& content = tooltip->emplaceContent<spk::Panel>();
auto& layout = content.setLayout<spk::VerticalLayout>();

layout.addWidget(...);

button.setTooltip(std::move(tooltip));
```

One thing to avoid

I would avoid making tooltip rendering a normal child of the tooltiped widget.

This would be problematic:

```text
Button
└── Tooltip
```

If the tooltip is rendered as a normal child, it may be affected by:
- parent clipping
- parent geometry
- layout constraints
- scroll area clipping
- z-order issues
- mouse hit-testing issues.

Instead, the tooltiped widget should only own or reference the tooltip content. The actual display should be delegated to TooltipHost / OverlayHost.

---

## 9. Diagnostics Pages

### 9.1 Events / Focus Page

Purpose:

> Validate input routing, focus, hover, keyboard activation, and future modal/focus behavior.

Should display:

- hovered widget;
- focused widget;
- pressed widget;
- last mouse event;
- last keyboard event;
- last glyph input;
- focus traversal order;
- Tab / Shift+Tab behavior;
- Escape behavior;
- Enter/Space activation behavior;
- click-through behavior with overlays and modals.

### 9.2 Clipping / Z-order Page

Purpose:

> Validate clipping boundaries, nested clipping, z-order, and overlay ordering.

Demos:

- child panel partly outside parent;
- nested clipping;
- text overflow;
- image overflow;
- scroll area clipping;
- popup over scroll area;
- tooltip over normal content;
- modal backdrop over normal content;
- message box over backdrop.

### 9.3 Theme / Style Page

Purpose:

> Stress-test current `WidgetStyle::Collection` usage and prepare migration to semantic themes.

Demos:

- normal state;
- hover state;
- pressed state;
- disabled state;
- focused state;
- checked state;
- invalid input state;
- warning/error/info panels;
- alternate palettes.

### 9.4 Metrics / Debug Overlay Page

Purpose:

> Expose runtime information that helps validate performance and behavior.

Metrics:

- update time;
- render time;
- frame time;
- draw calls, if available;
- widget count;
- visible widget count;
- focused widget;
- hovered widget;
- active page;
- popup/modal count;
- current theme name.

---

## 10. Proposed Project Organization

`UIShowcase` should be stored in the Sparkle repository for convenience, but it should behave like an external consumer project.

The Sparkle root build must not know about the showcase.

This means:

- no `ENABLE_UI_SHOWCASE` option in Sparkle's root `CMakeLists.txt`;
- no `add_subdirectory(UIShowcase)` from Sparkle's root `CMakeLists.txt`;
- no showcase configure/build preset in Sparkle's root `CMakePresets.json`;
- no dependency from the Sparkle library target to showcase files;
- `UIShowcase` configures from its own source directory;
- `UIShowcase` imports `Sparkle::Sparkle` by itself.

Recommended folder structure:

```text
UIShowcase/
  CMakeLists.txt
  CMakePresets.json
  planning.md
  includes/
  srcs/
```

The root Sparkle project remains focused on the library, tests, tooling, installation, packaging, and optional internal playground. The showcase is only colocated with the repository; architecturally it is a separate application.

### 10.1 Dependency Strategy

`UIShowcase` should support two ways of consuming Sparkle.

| Mode | Purpose | CMake behavior |
| --- | --- | --- |
| Installed package mode | Validate Sparkle as a real external user would consume it. | `find_package(Sparkle CONFIG REQUIRED)` |
| Local source mode | Convenient development while the showcase lives inside the Sparkle repository. | `add_subdirectory(<Sparkle repo root> <showcase build>/sparkle-build EXCLUDE_FROM_ALL)` |

Installed package mode is the preferred validation mode because it proves that Sparkle's exported CMake package works correctly.

Local source mode is acceptable for day-to-day development, but it must be initiated from `UIShowcase/CMakeLists.txt`, not from Sparkle's root `CMakeLists.txt`.

### 10.2 Recommended `UIShowcase/CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.16)

project(SparkleUIShowcase VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(
    SPARKLE_UI_SHOWCASE_USE_LOCAL_SPARKLE
    "Build Sparkle from a local source checkout instead of using find_package(Sparkle)."
    ON
)

set(
    SPARKLE_UI_SHOWCASE_SPARKLE_SOURCE_DIR
    "${CMAKE_CURRENT_LIST_DIR}/.."
    CACHE PATH
    "Path to the Sparkle repository root when SPARKLE_UI_SHOWCASE_USE_LOCAL_SPARKLE is ON."
)

if (SPARKLE_UI_SHOWCASE_USE_LOCAL_SPARKLE)
    if (NOT EXISTS "${SPARKLE_UI_SHOWCASE_SPARKLE_SOURCE_DIR}/CMakeLists.txt")
        message(FATAL_ERROR "SPARKLE_UI_SHOWCASE_SPARKLE_SOURCE_DIR does not point to a CMake project.")
    endif()

    add_subdirectory(
        "${SPARKLE_UI_SHOWCASE_SPARKLE_SOURCE_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}/sparkle-build"
        EXCLUDE_FROM_ALL
    )
else()
    find_package(Sparkle CONFIG REQUIRED)
endif()

file(GLOB_RECURSE SPARKLE_UI_SHOWCASE_HEADERS
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/includes/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/includes/*.h"
)

file(GLOB_RECURSE SPARKLE_UI_SHOWCASE_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/srcs/*.cpp"
)

add_executable(SparkleUIShowcase
    ${SPARKLE_UI_SHOWCASE_HEADERS}
    ${SPARKLE_UI_SHOWCASE_SOURCES}
)

target_include_directories(SparkleUIShowcase
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/includes"
)

target_link_libraries(SparkleUIShowcase
    PRIVATE
        Sparkle::Sparkle
)

target_compile_features(SparkleUIShowcase PRIVATE cxx_std_23)
```

Important consequence:

```text
cmake --build build/debug
```

from the Sparkle root build should not build `SparkleUIShowcase`.

Instead, the showcase is configured separately:

```sh
cmake -S UIShowcase -B build/UIShowcase/debug --preset debug
cmake --build build/UIShowcase/debug
```

or, without presets:

```sh
cmake -S UIShowcase -B build/UIShowcase/debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/UIShowcase/debug
```

### 10.3 Recommended `UIShowcase/CMakePresets.json`

`UIShowcase` should have its own presets file instead of extending Sparkle's root presets.

```json
{
    "version": 3,
    "cmakeMinimumRequired": {
        "major": 3,
        "minor": 16,
        "patch": 0
    },
    "configurePresets": [
        {
            "name": "default",
            "hidden": true,
            "generator": "Ninja",
            "binaryDir": "${sourceDir}/../build/UIShowcase/${presetName}",
            "toolchainFile": "$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake",
            "cacheVariables": {
                "CMAKE_CXX_STANDARD": "23",
                "CMAKE_CXX_STANDARD_REQUIRED": "ON",
                "CMAKE_EXPORT_COMPILE_COMMANDS": "ON",
                "VCPKG_TARGET_TRIPLET": "x64-windows"
            }
        },
        {
            "name": "debug",
            "inherits": "default",
            "description": "Configure the standalone UI showcase in Debug mode using the local Sparkle source tree.",
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": "Debug",
                "SPARKLE_UI_SHOWCASE_USE_LOCAL_SPARKLE": "ON"
            }
        },
        {
            "name": "release",
            "inherits": "default",
            "description": "Configure the standalone UI showcase in Release mode using the local Sparkle source tree.",
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": "Release",
                "SPARKLE_UI_SHOWCASE_USE_LOCAL_SPARKLE": "ON"
            }
        },
        {
            "name": "installed-debug",
            "inherits": "default",
            "description": "Configure the standalone UI showcase in Debug mode using an installed Sparkle package.",
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": "Debug",
                "SPARKLE_UI_SHOWCASE_USE_LOCAL_SPARKLE": "OFF"
            }
        }
    ],
    "buildPresets": [
        {
            "name": "debug",
            "configurePreset": "debug",
            "targets": [
                "SparkleUIShowcase"
            ]
        },
        {
            "name": "release",
            "configurePreset": "release",
            "targets": [
                "SparkleUIShowcase"
            ]
        },
        {
            "name": "installed-debug",
            "configurePreset": "installed-debug",
            "targets": [
                "SparkleUIShowcase"
            ]
        }
    ]
}
```

For installed package mode, the user should pass one of these depending on how Sparkle was installed:

```sh
cmake -S UIShowcase -B build/UIShowcase/installed-debug --preset installed-debug -DSparkle_DIR=<path-to-SparkleConfig.cmake-folder>
```

or:

```sh
cmake -S UIShowcase -B build/UIShowcase/installed-debug --preset installed-debug -DCMAKE_PREFIX_PATH=<Sparkle-install-prefix>
```

### 10.4 Root Project Rules

Sparkle's root `CMakeLists.txt` should remain unchanged for the showcase.

Allowed root-level responsibilities:

- build `Sparkle`;
- build tests when `SPARKLE_BUILD_TESTS` is ON;
- build `Playground` when `SPARKLE_BUILD_PLAYGROUND` is ON;
- install and export `Sparkle::Sparkle`;
- package Sparkle.

Forbidden root-level responsibilities:

- declaring `ENABLE_UI_SHOWCASE`;
- calling `add_subdirectory(UIShowcase)`;
- adding `SparkleUIShowcase` to default build targets;
- adding showcase presets to the root `CMakePresets.json`;
- making library CI depend on the showcase unless a separate showcase CI job intentionally configures `UIShowcase` as its own project.

### 10.5 Why This Design Is Better

This validates more of Sparkle's public integration surface.

The showcase will catch issues that an internal `add_subdirectory(UIShowcase)` target could hide, such as:

- missing installed headers;
- missing generated resource headers in the install interface;
- incomplete exported CMake targets;
- missing transitive dependencies;
- incorrect `SparkleConfig.cmake`;
- accidental dependency on repository-relative paths;
- accidental dependency on private build targets.

The showcase should therefore be considered an integration consumer of Sparkle, not part of Sparkle's library build.

---

## 11. Implementation Milestones

### Milestone 0 — Scaffold

Goal: compile and launch an empty showcase shell.

Tasks:

- create standalone `UIShowcase/CMakeLists.txt`;
- create standalone `UIShowcase/CMakePresets.json`;
- import `Sparkle::Sparkle` from `UIShowcase` itself;
- verify that the Sparkle root build does not build the showcase;
- create `ShowcaseRoot`;
- create top bar;
- create navigation panel;
- create center content area;
- create right properties panel;
- create page interface;
- create page registry;
- register one placeholder overview page.

### Milestone 1 — Existing Widget Gallery

Goal: show existing widgets without creating new core primitives.

Pages:

- overview;
- labels/text;
- panels/slice9;
- buttons/commands;
- images/animation;
- sliders;
- input;
- scroll;
- menu/workspace;
- dialogs/message boxes.

### Milestone 2 — Layout Validation

Goal: make layout behavior visible and testable.

Pages:

- layout lab;
- linear layout;
- grid layout;
- form layout;
- size policy matrix;
- dynamic layout mutation;
- nested layouts.

This milestone should be prioritized early because it validates widget composition, not only individual widgets.

### Milestone 3 — Diagnostics

Goal: expose runtime behavior and debugging information.

Pages:

- events/focus;
- clipping/z-order;
- theme/style;
- metrics/debug overlay.

### Milestone 4 — Overlay Infrastructure

Goal: provide shared infrastructure for future UI primitives.

Tasks:

- implement `OverlayHost` or `PopupLayer`;
- support passive overlays;
- support popup overlays;
- support modal overlays;
- support outside-click closing;
- support Escape routing;
- support clamped/flipped positioning;
- migrate menu/dropdown-like behavior where appropriate.

### Milestone 5 — Tooltip System

Goal: add global tooltip support.

Tasks:

- add tooltip metadata to `spk::Widget`;
- add `spk::Tooltip` data object;
- add `TooltipHost` behavior on top of `OverlayHost`;
- add tooltip delay;
- add tooltip page;
- add tooltips to showcase controls.

### Milestone 6 — Missing Primitive Widgets

Goal: fill the most common widget gaps.

Recommended order:

1. `spk::ProgressBar`
2. `spk::CheckBox`
3. `spk::RadioButton` / `spk::RadioGroup`
4. `spk::ToggleSwitch`
5. `spk::TabBar` / `spk::TabView`
6. `spk::ListView`
7. `spk::ComboBox`

### Milestone 7 — Advanced Interaction

Goal: make Sparkle UI feel complete for real application use.

Tasks:

- keyboard focus traversal;
- text selection;
- clipboard integration;
- double-click word selection;
- drag selection;
- Shift+arrows;
- Ctrl+A/C/X/V;
- Escape and Enter routing;
- disabled-widget behavior;
- accessibility-oriented keyboard behavior where possible.

---

## 12. Implementation Notes And Risks

### Avoid depending on private internals

Good usage:

```cpp
button.setText("Click me");
button.subscribe(/* callback */);
slider.setValue(42);
```

Risky usage:

```cpp
button._hovered = true;
button._styleCollection[0] = style;
```

The showcase should push Sparkle toward better public APIs when something important is not externally testable.

### Layout mutation may reveal ownership problems

Moving widgets between layouts should be tested carefully.

Potential issues:

- source layout still references the moved widget;
- target layout does not recompute geometry;
- scroll area content size is not refreshed;
- hover state points to stale geometry;
- focus remains on a hidden or detached widget;
- nested layout removal leaves stale element references;
- grid cell replacement forgets to detach previous content.

The layout lab and dynamic mutation page exist specifically to reveal these issues.

### Placeholder pages are useful

A placeholder page should not be empty. It should show:

- feature status;
- expected public API;
- dependencies;
- interaction model;
- known open questions;
- implementation priority.

This keeps the showcase useful as a design document even before the widget exists.

---

## 13. Recommended First Build Order

Recommended immediate order:

1. create standalone `UIShowcase/CMakeLists.txt`;
2. create standalone `UIShowcase/CMakePresets.json`;
3. verify that `cmake -S UIShowcase ...` builds `SparkleUIShowcase`;
4. verify that the Sparkle root build does not build `SparkleUIShowcase`;
5. verify installed package mode with `find_package(Sparkle CONFIG REQUIRED)`;
6. implement `ShowcaseRoot`;
7. implement page interface and registry;
8. implement navigation and properties panel;
9. add overview page;
10. add linear layout page;
11. add grid layout page;
12. add form layout page;
13. add layout lab page;
14. add basic widget pages;
15. add diagnostics pages;
16. add planned-widget placeholder pages;
17. implement overlay infrastructure;
18. implement tooltip system;
19. implement missing primitive widgets one by one.

This order intentionally validates the showcase as an external integration point before building the widget pages. It also prioritizes layout validation early, because Sparkle's UI quality depends heavily on widget composition and geometry recomputation.

---

## 14. Final Page List

Minimum useful first version:

```text
Overview
Labels / Text
Panels / Slice9
Buttons / Commands
Images / Animation
Sliders
Input
Scroll
Linear Layout
Grid Layout
Form Layout
Layout Lab
Size Policy Matrix
Dynamic Layout Mutation
Nested Layouts
Dialogs / Message Boxes
Menu / Workspace
Events / Focus
Clipping / Z-order
Theme / Style
Diagnostics
Missing Widgets
```

Expanded target version:

```text
Overview
Labels / Text
Panels / Slice9
Buttons / Commands
Images / Animation
Sliders
Input
Scroll
Layout Lab
Linear Layout
Grid Layout
Form Layout
Size Policy Matrix
Dynamic Layout Mutation
Nested Layouts
Menu / Workspace
Dialogs / Message Boxes
Interface Windows
Container / Viewport
Scalable Widgets
Screens
Events / Focus
Clipping / Z-order
Theme / Style
Metrics / Debug Overlay
Tooltip
Popup / Overlay
Tabs
Dropdown / ComboBox
Checkbox / Radio / Toggle
ProgressBar
Radial Controls
```
