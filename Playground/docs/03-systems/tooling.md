# System — Tooling Suite (EreliaTools)

One executable, tab-per-editor (D15), reading and writing the same `resources/data/*.json`
the game loads. Tools are first-class deliverables: the user authors all content through
them, and their reusable widgets are prime `spk::` promotion candidates (D18).

Diagrams: [12-class-tooling.puml](../diagrams/12-class-tooling.puml).
Plan: steps [25](../plan/steps/step-25-tools-suite-voxel-editor.md) →
[28](../plan/steps/step-28-tools-encounter-ability-editors.md).
Reference for editor layouts: Unity editor windows (`FeatBoardEditorWindow`,
`EncounterTeamEditorWindow`, `Team_Editor_WindowV2.png`, `Encounter_Table_UI.png`).

---

## Shell (`Playground/tools/`)

```
EreliaTools (exe, links PlaygroundCore)
  ToolsWindow
    tab bar (spk widgets) → one IEditorPage active
    shared services: registries (loaded like the game), JsonWriter (pretty, stable key
    order so diffs stay reviewable), dirty-state + save prompt, status bar
  IEditorPage { name(); activate(); deactivate(); hasUnsavedChanges(); save(); }
```

Editing model: pages edit **in-memory copies** of definitions; Save writes the JSON file
and hot-reloads the page's registry. No undo stack v1 (file-level revert = reload); add
command-stack undo only when the modeler demands it.

## Shared building blocks (build once, in step 25; promotion candidates)

- **`pg::tools::Viewport3D`** — a widget hosting its own `GameEngine` + `Camera3D` +
  orbit controls, rendering a preview scene into its rect (the multi-camera design D04
  makes this trivial: the page's render pass = its own CameraUpdate + draws). Used by the
  voxel modeler, model preview, encounter board preview, world editor.
- **`pg::tools::PropertyPanel`** — reflection-free property editor: rows of
  label + typed field widgets (int/float/string/enum dropdown/vec3/id-picker), built
  programmatically per definition type. The **polymorphic type picker** (dropdown of a
  factory's registered types → swaps the edited subobject, preserving common fields) is the
  port of Unity's `ManagedReferenceTypePicker` — every editor with `Effect`/
  `FeatRequirement`/`VoxelShape` fields needs it.
- **`pg::tools::IdPicker`** — searchable dropdown over a registry's ids (abilities,
  statuses, voxels…).
- **`pg::tools::GraphCanvas`** — pannable/zoomable node canvas (nodes at positions, edges,
  selection, drag) for the feat-board editor.

## The editors

| Tab | Step | Edits | Layout essentials |
|---|---|---|---|
| **Voxel Modeler** | 25 | `voxels/*.json` (+ later `models/*.json`) | left: definition list + New; center: `Viewport3D` previewing the shape meshed alone *and* in a 3×3 neighbour context (occlusion sanity); right: PropertyPanel — traversal, tags, shape type picker, per-slot texture picker (atlas grid picker widget), live height overlay showing CardinalHeights |
| **World / Structure Editor** | 26 | `maps/*.json` | Viewport3D over a `VoxelWorld`; voxel palette bar; place/erase/orient tools (mouse picking reuses `MousePicker`); box-fill tool; marker placement; structure stamping; save as map or structure |
| **Feat Board Editor** | 27 | `featboards/*.json` | GraphCanvas center (nodes colored by kind, root highlighted, adjacency edges; drag nodes, link/unlink); right: node PropertyPanel (kind, repeatLimit, requirement list + type picker, reward list + type picker); validation pass on save (connected graph, existing ids, form-tier sanity) |
| **Encounter / Team Editor** | 28 | `encounter-tables/*.json`, team presets | tier accordion (10 tiers) → weighted team rows; team editor: 6 slots, per-slot species/form/ai IdPickers + completedNodes picker (from the species' board); board preview via Viewport3D |
| **Ability / Status Editor** | 28 | `abilities/*.json`, `statuses/*.json` | list + PropertyPanel (cost/range/AoE/profile fields, effect list with type picker); live generated rules text preview (same `describe()` used by the HUD) |

The **model editor** (rich creature models: parts, per-face textures) grows inside the
Voxel Modeler tab after step 25's shape editing is solid — same viewport + property
machinery, `models/*.json` output (schema versioned for growth).

## Dependencies

Uses: PlaygroundCore (registries, mesher, board preview, describe()), Sparkle widgets.
Used by: the user. Game and tools never run simultaneously against the same files
(no live sync needed; tools hot-reload their own registries on save).

## Testing

Headless: JsonWriter round-trip (parse→write→parse equality, stable ordering), page
dirty-state logic, feat-board validation rules. Everything visual/interactive:
hand-validated (tools are themselves validation aids for game data).
