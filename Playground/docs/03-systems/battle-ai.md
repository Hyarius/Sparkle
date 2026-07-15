# Rule-driven battle AI

Step 11 adds a headless deterministic controller for the ordered `AIBehaviourDefinition`
rules loaded from `data/ai`. `BattleAIPlanner` is pure: it copies a `BattleSnapshot`, reads
immutable registries and shared legal plans, and returns one ordinary `BattleCommand` plus an
owned rule trace. It neither mutates a unit nor draws `BattleRng`.

`BattleAIDriver` owns only activation-local guard scratch and immediately submits that command
through `BattleSession::submit`. Enemy commands use `EnemyAi`; optional development player
autoplay uses `DebugAutoplay` and an explicitly supplied profile. There is no AI-only effect,
movement, target, resource, or end-activation path.

## Planner rules

Every selector starts with active, placed combatants (`health > 0` and no removal reason).
`self` is the actor; ally selectors exclude self. Nearest selectors use x/z Manhattan
distance; low/high-health selectors use `health * 1000 / maxHealth`, truncated to integer
permille. Ties are roster order then battle-unit ID.

Conditions are ANDed in authored order and stop at the first false result. Health and distance
comparisons are inclusive, resource conditions inspect only the actor's current AP/MP, status
conditions use runtime status/tag queries, and `abilityAffordable` checks ownership plus costs
only. A missing selector is false, including `present: false` status checks.

Cast decisions use `BattleSession::planCast` or its shared legal-anchor enumeration. Unit and
self anchors still run the complete validator. Best-area maximizes preferred active affected
units, minimizes the other side, then uses canonical board-cell order. Nearest-legal-cell uses
Manhattan distance then canonical order. Movement considers only legal, non-origin plans inside
the current-MP/author cap and requiring strict toward/away improvement. Its ties are objective
distance, weighted MP cost, path length, then canonical destination. The command contains only
the chosen destination or anchor.

A matching decision with no legal command falls through. After every accepted move or cast the
next driver operation reevaluates from rule zero against a fresh snapshot. `endTurn` is also an
ordinary submitted command.

## Guards, diagnostics, and pumping

For an activation, the driver records accepted non-end commands and insertion-ordered material
progress digests. The AI command cap requests `EndTurn{AiCommandCap}`. Re-observing a progress
digest requests `EndTurn{AiRepeatedStateGuard}`, so zero-cost no-op commands and longer cycles
cannot spin. `gameplayProgressDigest` deliberately excludes event/action counters and guard
scratch; `BattleSession::authoritativeBattleStateDigest` adds the generic command counter, and
`BattleAIDriver::authoritativeStateDigest` additionally folds its guard state.

A rejected planned command creates an owned `AICommandDiagnostic`, never a gameplay event. A
changed-state rejection may replan once; otherwise the driver requests
`EndTurn{AiInvalidPlannedCommand}`. A missing/corrupt profile or no produced rule safely requests
`AiNoRuleProducedCommand`.

`BattlePump::advanceUntilPlayerInput` consumes bounded logical operations only: a scheduler
boundary or one AI submission. It reports deployment, player wait, AI wait, progress, or terminal
state and never receives frame time. Splitting a budget therefore leaves the same simulation as a
single larger call. Step 12 will call this seam from battle lifecycle/update code; this step does
not start a battle from exploration or add battle presentation.
