# Step 18 — Battle log → FeatBoardService evaluation → applyProgress pipeline

**Phase F · needs step 17**

## Goal

Creatures actually progress: post-battle evaluation of the event log against reachable
nodes (both outcomes, D25), completions applying rewards live, encounter enemies powered by
`completedNodes`.

## Reading

[creatures-feats.md §5](../../03-systems/creatures-feats.md) ·
[24-seq-feat-progression.puml](../../diagrams/24-seq-feat-progression.puml).

## Files

`srcs/feats/feat_board_service.hpp/.cpp` — the service per spec: subscribes
`battleResolved`; per player-side creature: filter the `BattleLog` (caster==u or
target==u≠caster), append `BattleWon{unitSurvived}` on wins, reset transient progress,
evaluate **reachable nodes only** in a loop until no new completion (newly-adjacent nodes
join the same pass), reset transient again, `featProgressUpdated(unit, completions)`,
`applyProgress`; `completeNodeDirect(unit, nodeUuid)` (bypass requirements, honor rewards —
used by encounters and tools).
`srcs/battle/end phase` — emits BattleWon into the log (already, step 10) — verify both-
outcome handling: on loss, no BattleWon, evaluation still runs.
`encounters/encounter_service.*` — enemies: pre-complete `completedNodes` via
`completeNodeDirect` then applyProgress (full power dial incl. form, D35); remove step-14's
warning log.
Remove step 17's temporary newGame debug completion.
Data: give the wild `forest-basic` table a second team entry with `completedNodes`
containing sprout's first stat node (visible tougher variant); starter creatures begin at
root only.

## Tests (`[test]`)

The full pipeline on synthetic logs: a battle log crafted to complete exactly node X on
creature A and progress-but-not-complete node Y (assert percentages); cascade unlock (X
completes → neighbour Z becomes reachable and completes from the same log); loss still
progresses (no BattleWon; winBattleCount doesn't move; dealDamage does); Game-scope
carryover across two evaluations; featProgressUpdated payload counts; enemy pre-completion
(stats + form reflect nodes).

## Definition of Done

`[build]`/`[test]` green; `[run]`: play a bush fight dealing 30+ damage with sprout →
console/overlay reports "Sprout: 1 feat completed"; next fight, Ember is castable
(rewards live without restart); losing a fight still reports partial progress. User
validates the loop feels right.
