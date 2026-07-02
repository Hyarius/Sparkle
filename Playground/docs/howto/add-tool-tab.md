# How-to — Add an editor tab to EreliaTools

Prereq: the tools shell exists (step 25). Reference layout:
[tooling.md](../03-systems/tooling.md).

## 1. Page class

```cpp
// Playground/tools/pages/my_editor_page.hpp
class MyEditorPage : public pg::tools::IEditorPage   // itself a spk::Widget subclass
{
public:
    MyEditorPage(spk::Widget *p_parent, pg::tools::ToolServices &p_services);
    std::string name() const override { return "My Editor"; }
    bool hasUnsavedChanges() const override;
    void save() override;      // JsonWriter → resources/data/<domain>/ + registry hot-reload
protected:
    void _onGeometryChange() override;   // layout children here (widget-resize convention)
};
```

`ToolServices` provides: the loaded registries, `JsonWriter`, the status bar, and dirty-state
plumbing. Pages edit **in-memory copies**; `save()` is the only writer.

## 2. Compose from the shared blocks

`pg::tools::Viewport3D` (3D preview — give it a scene-build lambda),
`pg::tools::PropertyPanel` (typed rows + polymorphic type picker),
`pg::tools::IdPicker` (registry dropdown), `pg::tools::GraphCanvas` (node graphs).
Lay them out with `spk::LinearLayout`/`spk::GridLayout` in `_onGeometryChange`
(never in per-frame code — geometry cascades on resize automatically).

## 3. Register the tab

```cpp
// tools/main.cpp
toolsWindow.addPage(std::make_unique<MyEditorPage>(&toolsWindow.pageArea(), services));
```

## 4. Verify

`[tools]`; open the tab, edit, save; confirm the JSON diff is clean (stable key order) and
the **game** still loads it (`[run]`). Round-trip test for the domain per
[add-json-definition.md](add-json-definition.md) §4.
