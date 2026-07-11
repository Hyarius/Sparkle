# Ticket 037 — Make weighted prefab selection independent of checkout path

- **Status:** Ready
- **Priority:** P2 — Medium
- **Area:** Playground / deterministic content loading

## Problem

Weighted palette selection mixes the absolute source filename into its seed. Identical content therefore resolves differently after moving the repository, installing the binary elsewhere, or building on another machine. This violates reproducibility for a fixed data set.

## Evidence

- `playground/srcs/world/voxel_content_parser.cpp:39-52` hashes `p_reader.file().generic_string()`.
- `playground/CMakeLists.txt:23-28` compiles an absolute source resource path into every consumer.

## Failure scenario

Load the same prefab JSON and voxel registry from `C:/work/Sparkle` and `D:/build/Sparkle`. Weighted cells can select different voxel variants even though every input file and world seed is identical.

## Proposed direction

Seed selections from a stable logical asset identifier, such as the registry ID plus JSON path/token and authored coordinates. Keep physical filesystem paths only for diagnostics.

## Acceptance criteria

- [ ] Identical assets produce identical prefab cells regardless of absolute resource-root path.
- [ ] Distinct prefab IDs still receive independent deterministic selections.
- [ ] Moving the prefab within the resource tree has explicitly defined behavior.
- [ ] Diagnostic errors continue to show the physical source file.

## Tests

- [ ] Load the same fixture through two different temporary absolute roots and compare every resulting voxel.
- [ ] Verify different logical IDs do not collapse to the same sequence merely because their JSON content matches.
- [ ] Verify repeated loads are stable.

## Dependencies / notes

Ticket 038 owns the broader runtime resource-root strategy; agree on the logical asset identifier before implementing either ticket.
