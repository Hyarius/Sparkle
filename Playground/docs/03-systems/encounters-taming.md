# System — Encounters & Taming

How battles start (bush/interior/trainer/gym triggers, biome tables, tier scaling) and how
wild creatures join the team (taming — there is **no capture**, D24).

Diagrams: [09-class-encounters-taming.puml](../diagrams/09-class-encounters-taming.puml),
[22-seq-encounter-to-battle.puml](../diagrams/22-seq-encounter-to-battle.puml).
Plan: steps [08](../plan/steps/step-08-encounter-trigger.md) (M1 slice),
[19](../plan/steps/step-19-taming.md), [20](../plan/steps/step-20-encounters-full.md).
JSON: [02-data-model.md §11/§12](../02-data-model.md) + `tamingProfile` in §8.

---

## 1. Encounter pipeline (`Playground/srcs/encounters/`)

```
pg::EncounterSpawn      // what a resolved encounter is:
    std::vector<EncounterTeamMember> enemyTeam;   // ≤6: species/ai/completedNodes (D35)
    bool allowsTaming;                            // wild yes; trainer/gym no
    Vector2Int boardSize;                         // default from game rules (D13)
    PlacementStyle placementStyle;                // random | fixed | byLine
pg::EncounterEmitter    // ONE subscriber to EventCenter.playerMoved (exploration only):
    on playerMoved(cell):
        if mode != Exploration: return            (mode gates it; frozen in battle)
        tags = voxel tags at the player's cell (and the cell the player stands in, i.e.
               the passable voxel occupying it — bushes are passable cross-planes)
        biome = active biome (map's biome in M1; macro plan later)
        for each biome.encounterRules where trigger ∈ tags:
            EncounterResolver.tryRoll(rule, cell) → optional team → build EncounterSpawn →
            EventCenter.encounterTriggered(spawn); stop
pg::EncounterResolver
    tryRoll(rule, cell):
        if cell == lastCheckedCell: no roll        // no re-roll standing still (Unity)
        roll seeded RNG < rule.chancePerStep ?
        tier = table.tierForBadges(player badges)  // empty tier falls back downward (§data)
        weighted pick among tier.weightedTeams
pg::EncounterService    // turns a spawn into a battle:
    on encounterTriggered(spawn):
        instantiate enemy CreatureUnits (species; pre-complete completedNodes then
            applyProgress — derives stats, abilities AND form, D35), attach AI behaviours
        BoardBuilder.fromWorld(world, playerCell, spawn.boardSize)
        BattleContext{playerTeam (non-null slots), enemies, board, placement,
                      allowsTaming, returnWorldCell = playerCell}
        EventCenter.battleStarted(ctx)
```

Trainer LoS (step 20): trainer actors carry a facing + sight range; a `TrainerSightLogic`
raycasts (world DDA) trainer→player each time the player reaches a cell; hit ⇒ scripted
approach then `encounterTriggered` with the trainer's authored team, `allowsTaming=false`,
and a cleared-flag so beaten trainers never re-trigger (save-meta.md). Gym/boss encounters
(step 33) are scripted interactions on gym markers using **handcrafted boards**
(`BoardBuilder.fromGrid` on a `maps/*.json` structure) — the one non-Gamma board path.

**Interior encounters**: interiors tag their floor cells (`"caveFloor"`); rules trigger on
that tag — same pipeline, no special case.

## 2. Taming (`Playground/srcs/taming/`)

Conditions are **`TamingCondition`s** — their own catalog/factory over the shared
`BattleCondition` evaluation core (D33; the machinery is identical to feat requirements,
the authoring vocabulary is free to diverge). They are evaluated **live**, per event, so an
impressed creature leaves mid-fight:

```
pg::TamingProfile   { std::vector<unique_ptr<TamingCondition>> conditions; }   // on species
pg::TamingProgress  { per-condition Advancement; isImpressed; hasFailed;
                      evaluateEvents(events)  // all conditions complete ⇒ impressed
                      markFailed() }          // defeated before completion
pg::WildBattleUnit : BattleUnit { TamingProgress tamingProgress; }
pg::TamingService   // battle-scoped listener:
    on battleEventOccurred(e):   // only when ctx.allowsTaming
        for each enemy WildBattleUnit not impressed/failed/defeated:
            u.tamingProgress.evaluateEvents({e})
            if impressed: ctx.removeUnit(u)        // leaves the board, NOT defeated
                          EventCenter.creatureImpressed(u)
    on battleResolved(ctx, winner):
        if winner == Player:
            for each impressed unit: recruit = fresh CreatureUnit{species, form},
                applyProgress(recruit); player.addCreatureToTeamOrStorage(recruit)
                EventCenter.creatureRecruited(recruit)
        // loss ⇒ impressed creatures are forfeit (GDD)
```

Rules (GDD §7-Taming, all enforced here):
- tracked passively, no player input; wild encounters only (`allowsTaming`);
- `defeatUnit` on a wild unit marks taming failed permanently for that encounter;
- an impressed unit that left counts as neither alive nor defeated for victory conditions
  (battle continues; if it was the last enemy, the player side wins by
  "no active enemies" — `isActiveInBattle` already yields this);
- recruits **inherit the wild unit's `completedNodes` pre-completions** (then
  `applyProgress`): since form derives from completed nodes (D35), this is what keeps a
  tamed evolved creature evolved — Pokémon-consistent — and the player continues that
  board from where the wild spawn started. Requirement *progress* (partial windows) is not
  inherited, only completions.

**Which events do conditions see?** All battle events (the wild unit's conditions typically
reference the *player team's* behavior: "deal ≥80 magical damage in one hit", "end 2 turns
adjacent to the wild creature"). Condition types whose semantics are unit-relative
(`turnEndPosition`…) evaluate relative to their event's unit as usual — authors pick
condition types accordingly, and taming-specific types (e.g. conditions about the wild
creature being *witnessed* rather than acted on) live only in the TamingCondition factory
(D33). Taming conditions surface in the creature-info window only after first discovery
(HUD concern, step 22; data ships with the species regardless).

## 3. Dependencies

Uses: EventCenter, biome/encounter/species registries, FeatRequirement evaluation, board
builder, PlayerData. Used by: BattleMode (context creation), HUD (taming condition display),
save (cleared trainers/gyms), world gen (bush/flora density feeds trigger frequency).

## 4. Testing

Headless: resolver (chance gating incl. same-cell suppression, tier fallback, weighted pick
determinism under seeded RNG); spawn→context assembly (team instantiation with
completedNodes); taming progress across synthetic event streams (impressed mid-fight,
failure on defeat, forfeiture on loss, recruit freshness, team-full → storage); victory
interaction (last enemy impressed ⇒ player wins). Visual: bush trigger in the M1 world.
