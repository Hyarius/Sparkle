# Step 31 — Generated interiors: tunnels/caves, ports/sea links

**Phase J · needs step 30**

## Goal

Tunnel edges become playable: generated cave interiors (CellularAutomata) bound to their
two entrances via portals (D37), plus the abstract port/sea connectivity.

## Reading

[world.md §Interiors & portals / §generation](../../03-systems/world.md) ·
`includes/structures/math/spk_cellular_automata.hpp` (verify the real API) ·
[02-data-model.md §13b portals](../../02-data-model.md).

## Files

`srcs/world/generator/interior_generator.hpp/.cpp` — per tunnel feature (seeded by run
seed + feature id): carve a corridor-biased cave with CellularAutomata inside a bounded
map (size scaled to edge length, clamped), guarantee a walkable west↔east spine (A* check;
carve fallback corridor if the automaton pinched), dress with biome-appropriate voxels +
flora + optional prefab rooms, place `west`/`east` portals; output an in-memory
`MapDefinition`-equivalent cached by feature id.
`portal_service` — resolves generated-interior targets (feature id ↔ map instance) in
addition to authored maps; tunnel-entrance stamps from step 30 get their real portals
(placeholder door voxel removed).
Interior encounters: caves tag floors `caveFloor`; a `cave` encounter-table + biome rule
(interior biome derives from the mountain's region biome).
Ports/sea: port features get a dock prefab + a portal pair linking the two ports of a sea
edge (fast-travel semantics — sailing is out of scope, GDD allows the abstraction);
interaction prompt "Sail" on the dock portal (portal activation via prompt confirm rather
than step-on — extend PortalService with `requiresConfirm` flag, used by docks).

## Tests (`[test]`)

Interior determinism (feature id ⇒ identical map); spine walkability guaranteed across 50
seeded generations (property test); portal binding (west entrance ⇒ west portal ⇒ exiting
east lands at the far door); cave encounter rule fires; dock confirm-gating.

## Definition of Done

`[build]`/`[test]` green; `[run]` (user validates): walk a road into a tunnel entrance,
traverse the cave (encounters inside), exit the far side onto the continuing road; sail
between two ports. Both directions, repeatable.
