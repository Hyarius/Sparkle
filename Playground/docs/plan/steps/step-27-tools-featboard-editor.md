# Step 27 — Feat Board editor (GraphCanvas)

**Phase I · needs steps 25, 17**

## Goal

Author species feat boards visually: a node-graph canvas with requirement/reward editing —
the `Team_Editor_WindowV2.png` wireframe's left canvas, standalone.

## Reading

[tooling.md](../../03-systems/tooling.md) ·
[creatures-feats.md §2](../../03-systems/creatures-feats.md) ·
[02-data-model.md §6](../../02-data-model.md).

## Files

`tools/widgets/graph_canvas.hpp/.cpp` — pannable/zoomable 2D canvas widget: nodes (position,
size, color, icon, selected state), edges, hit-testing, drag-move, edge-drag linking,
box-select; generic (promotion candidate, step 37).
`tools/pages/featboard_editor_page.hpp/.cpp` — left: board list (+New); center: GraphCanvas
(nodes colored by kind, root ringed, drag to reposition [writes `position`], drag
node→node to link/unlink, right-click add/delete node — **new nodes get
`spk::UUID::generate()`**, D34); right: node PropertyPanel — displayName, kind, repeatLimit,
requirement list (add/remove; per-entry type picker from the **FeatRequirement factory** +
scope/repeat/type fields; new entries get UUIDs) and reward list (type picker + IdPickers
for ability/status/form). Save validation before write: connected graph, unique uuids,
neighbour symmetry (auto-fix offered), form nodes tier-ascending in serialized order
(auto-reorder offered), dangling ability/status ids (D35).
Species linkage: a species picker header shows which species use the board.

## Tests (`[test]`)

Validation rules each (craft failing boards); auto-reorder correctness; canvas hit-test
math (zoom/pan transforms); writer round-trip incl. uuids stability (editing a field must
not regenerate uuids — progress compatibility!).

## Definition of Done

`[tools]` (user validates): open sprout-board, add a node with a `dealDamage` requirement
and a stat reward, link it, save; `[run]` — the node appears reachable after its neighbour
completes and its reward applies. An existing save keeps its progress (uuids unchanged).
