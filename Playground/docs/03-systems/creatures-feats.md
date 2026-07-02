# System — Creatures & Feats

Species/forms/units, the attribute pipeline, the Feat Board, requirement evaluation, and
post-battle progression. **No XP levels — this system is the whole progression model.**

Diagrams: [08-class-creatures-feats.puml](../diagrams/08-class-creatures-feats.puml),
[24-seq-feat-progression.puml](../diagrams/24-seq-feat-progression.puml).
Plan: steps [14](../plan/steps/step-14-creatures-team.md),
[17](../plan/steps/step-17-feat-model.md), [18](../plan/steps/step-18-feat-progression.md).
JSON: [02-data-model.md §6/§8](../02-data-model.md).

---

## 1. Creature types (`Playground/srcs/creatures/`)

```
pg::Attributes        // the full stat block (glossary); plain struct, JSON-mapped
pg::CreatureSpecies   // immutable definition: attributes, defaultAbilities, featBoard*,
                      // defaultFormId, forms map, tamingProfile
pg::CreatureForm      { displayName; int tier; modelId; animationSetId; avatar; }
pg::CreatureUnit      // one owned instance (mutable, persisted):
    const CreatureSpecies* species;
    std::string currentFormId;                   // DERIVED (D35) — replayed changeForm
    Attributes attributes;                       // DERIVED — never authored, never saved
    std::vector<const Ability*> abilities;       // derived
    std::vector<const Status*>  permanentPassives; // derived
    FeatBoardProgress featBoardProgress;         // the ONLY persisted progression state
pg::CreatureStorage   // the PC: unbounded list + add/remove/swap-with-team
pg::PlayerData        // team[6] (nullable slots) + storage + world position
                      // addCreatureToTeamOrStorage(unit): first free slot, else storage
```

**The attribute pipeline (critical invariant):** `attributes`, `abilities`,
`permanentPassives` **and `currentFormId`** are always **recomputed** by
`FeatBoardService::applyProgress(unit)`: reset to `species.attributes` +
`defaultAbilities` + `defaultFormId`, then replay every completed node's rewards
(`completionCount` times each, in board-definition node order — the editor enforces form
nodes tier-ascending so replay order is form-correct, D35). Saves store **only**
`featBoardProgress` ([02-data-model.md §15](../02-data-model.md)); this keeps saves tiny
and makes reward changes retroactive. Never mutate `unit.attributes` or `currentFormId`
directly — add rewards or (battle-only) modify `BattleAttributes`.

## 2. Feat Board model (`Playground/srcs/feats/`)

```
pg::FeatBoard   { rootNodeUuid; std::vector<FeatNode>; node(uuid); neighbours(node); isRoot(node) }
pg::FeatNode    { spk::UUID uuid;                                   // stable identity (D34)
                  displayName; Vector2 position; FeatNodeKind kind;
                  neighbourUuids; int repeatLimit;                  // 0 = unlimited
                  std::vector<FeatRequirementEntry> requirements;   // ALL must complete
                  std::vector<unique_ptr<FeatReward>> rewards; }
pg::FeatRequirementEntry { spk::UUID uuid; unique_ptr<FeatRequirement> condition; }
pg::FeatNodeKind enum { StatsBonus, Ability, Passive, Form }
```

The feat registry also maintains a global `uuid → (board, node)` lookup so saves resolve
progress without knowing board layout (D34).

**Activation rule:** root starts completed (a fresh unit's progress records root at
completionCount 1). A node is **reachable** (accumulates progress) iff it isn't exhausted
(`completionCount < repeatLimit`, unless unlimited), any neighbour is completed, and — for
`Form` nodes — its form's tier is exactly `currentForm.tier + 1` (evolution gating; choosing
one branch is enforced by board topology: sibling form nodes neighbour the same parent, and
a completed form node's `changeForm` reward moves the tier, locking the other branch out via
the tier check — mirror of Unity `IsFormNodeLockedByCurrentTier`).

## 3. Condition evaluation — the shared `BattleCondition` core (D33)

```
pg::BattleCondition (abstract — shared by FeatRequirement AND TamingCondition)
    Scope scope;                 // Ability | Turn | Fight | Game   (D21)
    int requiredRepeatCount;     // how many scope windows must reach 100%
    virtual bool isScopeLocked() const;
    Advancement evaluateEvents(span<const BattleEvent*>, Advancement current);
    // template method: concrete types only implement
    virtual float evaluateEventProgress(const BattleEvent&) = 0;  // 0..100 contribution
pg::BattleCondition::Advancement { float progress; int completedRepeatCount; }

pg::FeatRequirement  = BattleCondition parsed via the FeatRequirement factory
pg::TamingCondition  = BattleCondition parsed via the TamingCondition factory
// One evaluation engine, two authoring catalogs: a condition type exists where its parser
// is registered (shared types register in both; specialized types in one).
```

Window semantics (implemented once in `BattleCondition::evaluateEvents`):
- **Ability**: each single event must reach 100% by itself.
- **Turn**: contributions grouped by `event.turnIndex`; a turn's sum ≥ 100% ⇒ one repeat.
- **Fight**: contributions accumulate across the battle; each 100% crossing ⇒ one repeat,
  remainder carried; leftover progress persists **within** the fight only.
- **Game**: like Fight but progress persists across battles (saved).

Concrete types = the catalog in [02-data-model.md §6.1](../02-data-model.md); each is
`BattleConditionTemplated<TEvent>` filtering its event type then scoring
(`computeLinearProgress(amount, required) = amount/required·100`). Meta `and`/`or` override
`evaluateEvents` to combine children (min/max progress; completion when all/any complete).

**Attribution rule:** an event contributes to a creature's requirements when the creature is
the event's **caster**, or its **target** (when target ≠ caster) — both roles are evaluated.
Condition types self-select the role via their progress function (e.g. `takeDamage` returns
0 unless the unit is the target).

## 4. Progress & persistence

```
pg::FeatRequirementProgress { spk::UUID requirementUuid; const FeatRequirement* condition;
                              Advancement adv;
                              persistsAcrossFights() = (scope == Game) }
pg::FeatNodeProgress        { spk::UUID nodeUuid; int completionCount;
                              vector<FeatRequirementProgress>;   // matched by uuid (D34)
                              isCompleted() (all reqs complete); isExhausted(node);
                              complete()  // ++count, reset req progress
                              resetTransientRequirementProgress() // non-Game scopes → 0 }
pg::FeatBoardProgress       { vector<FeatNodeProgress>; findProgress(nodeUuid);
                              getOrCreateProgress(node); toJson/fromJson
                              // load drops unknown uuids with a warning }
```

## 5. `pg::FeatBoardService` — the progression engine (static + one bus listener)

Post-battle flow (**both outcomes** — losing still progresses, D25; the GDD requires this
even though the Unity example only processed wins):

```
on battleResolved(ctx, winner):
    for each player-side BattleUnit u with sourceUnit:
        events(u) = battle log entries where caster==u or (target==u && target!=caster)
                    + (winner==Player ? BattleWon{unitSurvived: !u.isDefeated} : nothing)
        completions = registerFightEvents(u.sourceUnit, events(u))
            // reset transient (non-Game) progress, evaluate reachable nodes only,
            // node completes when ALL its requirements complete → complete() → node's
            // rewards apply → newly-adjacent nodes become reachable for the SAME pass
            // (loop until no new completion), reset transient progress again
        if completions > 0: EventCenter.featProgressUpdated(u.sourceUnit, completions)
    for each creature that completed nodes: applyProgress(unit)   // recompute pipeline (§1)
```

`registerFightEvents` evaluates **reachable nodes only** (progress never accrues on locked
nodes). Game-scope progress accrues across fights and is saved.

The end-of-battle **feat summary screen** (step 22) renders from the
`featProgressUpdated` payloads + a diff of node completions.

## 6. Dependencies

Uses: battle events/log, abilities/statuses registries, JSON layer.
Used by: battle (BattleUnit sources), taming (shared `BattleCondition` core, D33), save
(uuid-keyed progress serialization), HUD (team/creature cards), tools (feat-board editor),
encounters (`completedNodes` pre-completion when spawning enemies — apply via
`completeNodeDirect(unit, nodeUuid)` which bypasses requirements but honors rewards,
deriving stats *and form*, D35).

## 7. Testing

Headless: window semantics per scope (the 4 windows × carryover cases — highest-value
tests in the whole project); repeat counts; and/or combinators; reachability/adjacency
unlock; tier-gated form nodes + branch lockout; reward replay determinism (applyProgress
idempotent); full post-battle pass over a synthetic battle log; Game-scope persistence
round-trip through save JSON.
