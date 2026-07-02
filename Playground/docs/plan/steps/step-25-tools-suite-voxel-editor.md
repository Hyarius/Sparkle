# Step 25 — EreliaTools shell + shared widgets + **Voxel Modeler**

**Phase I · needs step 06 (may be pulled earlier if hand-editing JSON hurts) ·
parallel-safe with Phase J**

## Goal

The tool suite executable (D15) with its shared building blocks, and the anchor tool: the
voxel definition modeler (author shapes, preview meshed + in neighbour context, save JSON).

## Reading

[tooling.md](../../03-systems/tooling.md) (the spec) ·
[12-class-tooling.puml](../../diagrams/12-class-tooling.puml) ·
[howto/add-tool-tab.md](../../howto/add-tool-tab.md) · spk widgets:
`spk_interface_window`, `spk_linear_layout`, `spk_text_edit`, `spk_spin_box`,
`spk_push_button`, `spk_scroll_area`.

## Files

- **Modify** `Playground/CMakeLists.txt` — `EreliaTools` exe = glob `Playground/tools/**`,
  links PlaygroundCore (step 01 left the placeholder).
- `Playground/tools/main.cpp` — window + registries load + `ToolsWindow`.
- `tools/core/`: `tools_window.hpp/.cpp` (tab bar + page area + status bar + unsaved-change
  prompts), `i_editor_page.hpp`, `tool_services.hpp` (registries, JsonWriter, status),
  `json_writer.hpp/.cpp` (**pretty, stable key order** — definition structs → ordered JSON;
  round-trip tested).
- `tools/widgets/`: `viewport3d.hpp/.cpp` (widget hosting its own `GameEngine` + `Camera3D`
  + orbit input, scene-build callback, renders in its rect — D04 makes the extra camera
  pass trivial), `property_panel.hpp/.cpp` (typed rows: int/float/string/enum/vec-fields/
  checkbox + **polymorphic type picker** driven by a factory's registered type names +
  change callbacks), `id_picker.hpp/.cpp` (searchable registry dropdown),
  `atlas_cell_picker.hpp/.cpp` (click a cell on a rendered sprite sheet — voxel/mask/icon
  atlases).
- `tools/pages/voxel_modeler_page.hpp/.cpp` — left: definition list (+New/Duplicate/
  Delete); center: Viewport3D with two preview modes (lone voxel — all orientations row;
  3×3 neighbour context for occlusion sanity) + a **CardinalHeights overlay** (colored
  edge posts at the five height points); right: PropertyPanel (traversal, tags, shape type
  picker → per-shape fields + texture-slot atlas pickers). Save → `voxels/<id>.json` +
  registry hot-reload + preview rebuild.

## Tests (`[test]`)

JsonWriter round-trips for every currently-authored domain sample (parse→write→parse
equality + stable ordering); page dirty-state logic; type-picker subobject swap preserving
common fields. (Viewport/UX: hand-validated.)

## Definition of Done

`[tools]` (user validates): create a new "fence" voxel (crossPlane, passable), texture it,
see it meshed lone + in context with correct occlusion and heights overlay; save; `[run]`
the game — place it via a testground map edit and see it in-world. Diff of the written
JSON is clean and minimal.
