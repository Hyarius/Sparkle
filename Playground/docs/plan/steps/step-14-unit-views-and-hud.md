# Implement battle unit views and the battle HUD

Turn the manually controllable battle from step 13 into a readable playable prototype.
Represent every placed creature with a cube in the active board's presentation space, whose
tint and scale come from species JSON. The same presenters must work over a live-world
overlay and the isolated handcrafted arena. Animate committed movement/effect events
cosmetically, and implement the desktop battle HUD shown by the Unity wireframes: two
six-creature team columns, turn
forecast, active-unit resources, eight ability slots, deployment controls, and End Turn.

This is an implementation task. Modify Playground presentation/widget code, add pure view
models and lifecycle tests, perform visual checks, update documentation, and keep the full
build green. Do not add final models, an asset-authoring workflow, the taming-condition
panel, Feat summaries, or the persistent result screen; later steps own those features.

---

# 1. Repository baseline and visual references

Implement after steps 01-13. Reinspect the current repository and prerequisite handoff
reports. Reuse settled names rather than creating parallel snapshots, event bridges, or
interaction controllers.

At the original `6ae3151` baseline:

* the exploration avatar is a shared 0.65-unit
  `spk::PrimitiveObject::CreateCube` rendered by `spk::TextureMeshRenderer3D`;
* `TextureMeshRenderer3D` supports a shared mesh, texture, per-renderer `spk::Color` tint,
  translucency, and cast/receive-shadow flags;
* `ActorPathLogic` uses `walkHeightAtCenter` and `interpolateWalkSegment` so cube placement
  follows cubes, slabs, slopes, and stairs;
* `GameSceneWidget` is a `spk::GameEngineWidget` and can parent normal Sparkle widgets;
* available widgets include `spk::Panel`, `spk::TextLabel`, `spk::PushButton`, layouts, and
  `spk::SliderBar`;
* `spk::PushButton` has click contracts and pressed/released styles but no disabled-state
  API;
* active child widgets receive input before their parent and may consume it.

Steps 01-04 establish:

* stable `CreatureInstanceId` and `BattleUnitId` values;
* `CreatureAttributes`, `DerivedCreatureState`, and current species form;
* a required placeholder presentation block on every v1 form:

```json
{
  "presentation": {
    "tint": [72, 184, 112, 255],
    "scalePermille": 1000
  }
}
```

The JSON channels are integers in 0..255 and `scalePermille` is 250..3000. The definition
layer stores plain integer/byte values, not `spk::Color`, meshes, textures, renderer
pointers, or animation assets.

Steps 05-13 establish:

* board-local support coordinates, `BoardData::toPresentationCell`/
  `fromPresentationCell`, plus a
  typed live-only world conversion when applicable;
* a frozen traversal graph over either a pinned live world or an owning immutable
  handcrafted arena source;
* `BattleSession` as the sole mutation facade and copied `BattleSnapshot` values;
* `BattleSession::submit` returning the exact typed
  `AcceptedCommand | RejectedCommand | AbortedCommand` result;
* value-owned events in an append-only `BattleEventLog`;
* event publication only after atomic resolution by draining complete committed batches and
  wrapping each one in a generation-guarded `BattleBatchPublication`;
* `BattleQueryService` for pure previews;
* `BattleInteractionController` as the one owner of transient targeting/placement state;
* one persistent tactical camera and a battle board overlay driven by a source-agnostic
  presentation-cell binding/AABB.

The visual layout references are:

```text
C:/Users/User/Documents/UnityProject/Erelia/UI_Design/BattleHUD.png
C:/Users/User/Documents/UnityProject/Erelia/UI_Design/TeamUIElement.png
C:/Users/User/Documents/UnityProject/Erelia/UI_Design/BattleUIElement.png
C:/Users/User/Documents/UnityProject/Erelia/UI_Design/Creature_UI_Element.png
C:/Users/User/Documents/UnityProject/Erelia/UI_Design/Action_UI_Element.png
```

Treat them as low-fidelity layout intent, not assets to copy into Playground. GDD and
runtime correctness outrank pixel matching. The central 3D board view must remain dominant.

---

# 2. Required final behavior

After this step:

1. Every placed living battle unit has exactly one cube entity on its support surface.
2. Cube tint and scale come from the unit's derived current form `presentation` JSON.
3. Player and enemy units remain visually distinguishable even when authored tints are
   similar, through a small team marker or selection outline/mask.
4. Placement creates or repositions views only after the placement command commits.
5. Committed movement follows its resolved path cosmetically across shaped terrain.
6. Damage, healing, shield change, status, and defeat have simple readable cues without
   influencing rules or delaying the scheduler.
7. Every battle-object instance has one generic board marker keyed by `BattleObjectId`;
   place/trigger/remove events create readable bounded placeholder feedback.
8. The exploration avatar renderer stays hidden during battle, but its entity and player
   streamer remain active exactly as step 12 requires.
9. The HUD is inactive outside Battle and visible during Deployment, Activation, enemy
   processing, and Terminal staging.
10. Both teams show up to six stable cards with name, HP, defeat state, active selection,
    and stamina/ready information.
11. A predicted turn strip presents deterministic scheduler order without advancing the
     real scheduler.
12. The active-unit panel presents HP, shield, AP, MP, stamina/next-ready value, statuses,
     selected ability details, and eight ability slots.
13. Ability slot 1..8 and mouse clicks call the same step-13 interaction-controller method.
14. Disabled ability slots are styled as disabled and cannot emit a selection despite
     `spk::PushButton` lacking a native disabled API.
15. End Turn calls `BattleInteractionController::endTurn()`; it never advances the
     scheduler directly.
16. Deployment shows the player's six roster cards as selectors, reflects committed
     placement, and enables Ready only when `ConfirmDeploymentCommand` would validate.
17. HUD refresh is driven by bind/committed-batch/interaction notifications, not a full
     snapshot poll every render frame.
18. Battle teardown removes unit/object entities before destroying them, releases widget
     contracts before the session, and clears all IDs/animations/view models.
19. A second battle in the same widget creates no duplicate cards, clicks, views, or event
    deliveries.
20. Unit and object placement uses only the board-to-presentation mapping and active cell
    source. A handcrafted arena requires no `VoxelWorld`, exploration origin, world-cell
    conversion, or presenter subclass.

---

# 3. Hard boundary: simulation first, presentation second

Maintain this one-way dependency:

```text
JSON registries + persistent CreatureUnit
                ↓ projection
BattleSession / BattleSnapshot / BattleEventLog
                ↓ copied data + committed event spans
BattlePresentationRuntime
├── BattlePresentationBoardBinding active source/mapping; borrowed from environment lease
├── BattleUnitPresenter         unit 3D entities, cosmetic queue
├── BattleObjectPresenter       generic trap/object markers and cues
└── BattleHudWidget             widgets and view models
                ↓ intents only
BattleInteractionController
                ↓ value command
BattleSession::submit
```

Presentation may:

* retain stable IDs;
* retain copied view models;
* retain the last consumed event sequence;
* interpolate transforms and colors using frame delta;
* disable input until a cosmetic event queue reaches the authoritative snapshot;
* format definition data as text.

Presentation may not:

* own or mutate `BattleUnit`;
* infer damage, range, cost, or legality;
* decrement AP/MP/HP/stamina locally;
* advance `BattleTime`;
* consume battle RNG;
* append gameplay events;
* hold pointers/references into a temporary `BattleSnapshot`;
* make AI or outcome wait for a tween;
* use render completion as a rule event.

When the headless state moves immediately and a cube is still animating, the snapshot is
authoritative. The renderer is merely catching up.

---

# 4. Battle unit presentation data

Resolve presentation from the same derived form used to build the battle projection:

```cpp
struct BattleUnitVisualSpec
{
    std::array<std::uint8_t, 4> tint;
    std::uint16_t scalePermille = 1000;
};
```

Do not duplicate this structure if step 04 gave it the settled `PlaceholderVisual` name.
Convert it to engine types only at the presentation boundary:

```cpp
spk::Color toSpkColor(const PlaceholderVisual& visual);
float toWorldScale(const PlaceholderVisual& visual);
```

The conversion must divide integer channels by 255.0 and derive a finite positive scale
from `scalePermille`. Parser validation already rejects invalid v1 content.

If a legacy/test definition somehow lacks presentation, emit a diagnostic and choose a
stable fallback from a hash of semantic species/form ID. Never use pointer hashes,
`std::hash` implementation-dependent color output, frame time, or battle RNG. New v1 JSON
must never rely on the fallback.

Use a shared cube mesh and one small neutral/white texture for all battle units. Apply the
form tint with `TextureMeshRenderer3D::setTint` and scale with `Transform3D::setScale`.
Do not allocate a separate cube mesh or texture per unit.

Recommended prototype dimensions:

```text
base cube edge:       0.65 world unit
authored scale:       base * scalePermille / 1000
maximum footprint:    still visually centered in one tactical cell
foot clearance:       0.01 world unit above the walk surface
```

The JSON maximum of 3000 can exceed one cell. Preserve the authored scale for this
prototype, but camera framing and unit tests must handle it. Scale remains cosmetic and
does not change occupancy, range, or LoS.

---

# 5. Presentation-space placement on shaped support cells

All tactical coordinates identify solid support voxels. Never place a unit at
`presentationCell.y + 1` by assumption.

Create one pure helper equivalent to:

```cpp
[[nodiscard]] spk::Vector3 battleUnitCenterPosition(
    const BoardData& board,
    const BoardCell& localSupport,
    float renderedHeight);
```

For every source kind:

```text
presentSupport = board.toPresentationCell(localSupport)
localFoot      = walk-surface point from board.cells() at localSupport center
translation    = presentSupport - localSupport
presentFoot    = localFoot + translation
center         = presentFoot + (0, renderedHeight / 2 + 0.01, 0)
```

For a live board, `toPresentationCell` returns the corresponding exploration-world support
cell. For a handcrafted board, it returns the corresponding cell in the isolated arena
renderer, normally at the definition's zero-origin local grid. This mapping is the only
source-specific fact the presenter needs.

Use the board's frozen source/height contract if step 05 stored surface heights. Otherwise
read through `board.cells()`: its live implementation borrows the pinned `WorldCellSource`
while fluid is paused/revisions are valid, and its handcrafted implementation owns an
immutable grid for the environment-lease lifetime. Never query global `VoxelWorld` from a
unit/object presenter.

`board.cells()` is board-local; passing a presentation coordinate to it would apply the
mapping twice. For path interpolation, call `interpolateWalkSegment` with
`board.cells()` and board-local endpoints so shaped surface sampling is correct, then apply
the single board presentation translation derived from `toPresentationCell`. Validate at
attach that the mapping is one-to-one and translational across every presentable cell; a v1
board never rotates or non-uniformly scales tactical geometry for presentation. The helper
accounts for oriented/flipped slopes and step-edge heights. The cube center adds half its
rendered height after calculating the foot point. Do not linearly interpolate integer Y
values.

Headless `GridCellSource` fixtures without a presentation binding need no 3D presenter, but
the position helper remains testable by supplying the same identity binding used by an
authored arena. Production handcrafted boards are rendered and must use this exact path;
they are not skipped as synthetic fixtures.

---

# 6. BattleUnitView and presenter ownership

Create the equivalent of:

```cpp
struct UnitCosmeticTrack
{
    BattleActionId action;
    std::vector<BoardCell> path;
    std::size_t segment = 0;
    float segmentProgress = 0.0f;
    float worldUnitsPerSecond = 5.0f;
};

struct BattleUnitView
{
    BattleUnitId id;
    std::unique_ptr<spk::Entity3D> entity;
    spk::Transform3D* transform = nullptr;
    spk::TextureMeshRenderer3D* renderer = nullptr;
    BoardCell authoritativeCell;
    std::optional<UnitCosmeticTrack> cosmetic;
};

class BattleUnitPresenter
{
public:
    void attach(
        spk::GameEngine& engine,
        const BattlePresentationBoardBinding& boardPresentation,
        const Registries& registries,
        const BattleSnapshot& snapshot,
        BattleEventSequence nextSequence);

    void consume(
        std::span<const BattleEvent> events,
        const BattleSnapshot& afterBatch);

    void update(const spk::UpdateContext& tick);
    void fastForward();
    void detach() noexcept;
};
```

`BattleUnitPresenter` is source-agnostic. It accepts the active `BoardData`/presentation
binding established in step 13 and may branch only on semantic presentation data such as
unit side or form visual—not on `BoardSourceKind`. A single implementation creates,
repositions, interpolates, reconciles, and tears down entities for both source kinds.

Prefer a sorted vector/map with explicit canonical iteration. An unordered map may be used
for lookup only; never use its order for creation, rendering cues, or tests.

## 6.1 Attach

Some enemy/player placement may already be committed before presentation binds. Therefore:

1. copy the current snapshot;
2. iterate placed units in canonical `BattleUnitId` order;
3. create exactly one view per living or defeated-but-still-shown unit;
4. set its transform directly to authoritative placement;
5. record the log's next sequence as the consumption cursor;
6. register every entity with the engine only after it is fully configured.

Do not replay the entire setup log and also reconcile the snapshot, which would create
duplicates.

## 6.2 Reconcile after a batch

Consume `publication.batch.events` in sequence order for every committed batch publication,
then reconcile to that batch's `after` snapshot:

* a newly placed unit creates or reveals a view;
* another `PlaceUnitCommand` moves the same existing view to the new committed cell;
* a committed move creates a cosmetic path track;
* defeat marks/removes the view according to section 8;
* if an event was skipped or a cosmetic track cannot be built, snap to the snapshot;
* if a view exists for no snapshot unit, remove it with a diagnostic;
* advance the cursor exactly once.

Duplicate delivery of an already consumed sequence must be ignored. A sequence gap must
diagnose and perform full snapshot reconciliation rather than applying a partial animation.

## 6.3 Detach order

For every view:

1. clear cosmetic callbacks/tracks;
2. call `gameEngine.removeEntity(view.entity.get())`;
3. null cached component pointers;
4. destroy the entity.

Then clear shared presentation references and reset the event cursor. `detach()` is
idempotent and noexcept. `BattlePresentationRuntime` calls it before `BattleSession` and
`BoardData` destruction.

---

# 7. Committed movement animation

The resolver, not the view, chooses the path. Step 08 emits ordered
`UnitMovementStep{unit, from, to, enteredCellCost, Voluntary}` events followed by exactly
one aggregate `UnitMoved` for the accepted action. Collect those committed steps by
action/unit to obtain the presentation path and validate their summed cost/endpoints against
the aggregate. Do not reconstruct A* after occupancy has changed and do not require a
caller-supplied path in `MoveCommand`.

Build one section-6 `UnitCosmeticTrack` from the event. It is declared before
`BattleUnitView` because the view's `std::optional<UnitCosmeticTrack>` owns a complete value;
do not rely on a forward declaration there.

Rules:

* validate that the step chain is contiguous and its front/back/cost match `UnitMoved`;
* animate through every segment with `interpolateWalkSegment`;
* face the X/Z direction when practical;
* use render delta only for cosmetic progress;
* do not emit `UnitMoved` or change board occupancy;
* a zero-length/reposition event snaps;
* multiple tracks for one unit are queued in event order;
* if the queue exceeds a conservative cap during AI autoplay, collapse older cosmetics to
  the latest authoritative snapshot and diagnose once;
* battle exit fast-forwards/clears all tracks immediately.

The step-13 interaction controller may report `InteractionBusy` while pending cosmetics
are being presented, so the player does not issue a command against a visually stale
board. This is a presentation input gate only. The scheduler, AI pump, event log, outcome,
and deterministic digest have already advanced.

---

# 8. Minimal combat cues

The prototype needs readable feedback, not a VFX framework. Consume committed events:

* health damage: briefly tint toward red and pulse scale;
* healing: briefly tint toward green;
* shield gain/loss: brief blue/white pulse;
* status apply/remove: update HUD immediately; optional short tint pulse;
* displacement/teleport: use the committed position path if provided, otherwise snap;
* defeat/removal: desaturate/shrink over a short duration, then hide or remove;

Never derive a number from the visual cue. The HUD reads exact snapshot resources.
Overlapping cues use deterministic event order and bounded queues. Animation duration and
color blends are constants in presentation code, not combat JSON.

Floating numbers, particles, sound, camera shake, skeletal animation, and polished death
effects are non-goals.

## 8.1 Generic battle-object markers

Step 10 intentionally deferred scene visuals, but the playable pack contains a nonblocking
Thorn Snare that must not be invisible. Add a generic presenter; do not branch on
`definitionId` or add final art:

```cpp
struct BattleObjectView
{
    BattleObjectId id;
    std::unique_ptr<spk::Entity3D> entity;
    spk::Transform3D* transform = nullptr;
    spk::TextureMeshRenderer3D* renderer = nullptr;
    BoardCell authoritativeCell;
    std::optional<BattleSide> creatorSide;
    std::optional<ObjectCosmeticCue> cosmetic;
};

class BattleObjectPresenter
{
public:
    void attach(
        spk::GameEngine&,
        const BattlePresentationBoardBinding&,
        const BattleSnapshot&);
    void consume(std::span<const BattleEvent>, const BattleSnapshot& afterBatch);
    void update(const spk::UpdateContext&);
    void fastForward();
    void detach() noexcept;
};
```

Own a stable `BattleObjectId -> BattleObjectView` map and create views in ascending ID order.
Use one small generic prism/cube marker positioned with the same board-local
support-to-presentation walk-height helper as units. Offset it toward a fixed cell corner
and slightly above the surface so a unit may stand on a nonblocking object without hiding
it. If several legal
nonblocking objects share a cell, assign deterministic small vertical slots by ascending
object ID; recompute those cosmetic slots after reconciliation. Creator side chooses only a
generic Player/Enemy/neutral marker color. Blocking state may choose a generic solid versus
outline scale. Definition IDs never select C++ colors, meshes, or behavior.

This presenter is equally source-agnostic: it does not query `VoxelWorld`, inspect
`BoardSourceKind`, add a world anchor, or own arena geometry. The environment lease owns the
surface renderer/grid; the object presenter owns only object marker entities and removes
them before that lease is released.

On attach, reconcile every object snapshot already present. On committed
`BattleObjectPlaced`, create/reveal one marker only after commit. On
`BattleObjectTriggered`, pulse the existing marker once per event in sequence order. On
`BattleObjectRemoved`, queue a short shrink/fade and remove the entity; if trigger and
removal occur in the same batch, retain a bounded transient copy long enough to show the
trigger cue, then destroy it. Reconcile against `afterBatch.objects` after consuming the
span: create a missing authoritative object, snap a wrong cell, and remove a stale view with
a diagnostic. Cosmetic cue duration never delays commands, scheduler, result finalization,
or Continue.

No marker participates in picking, occupancy, LoS, or movement. Board input still resolves
support cells and the headless query service supplies legality. `detach()` removes every
registered marker/transient entity from the engine before destruction and is idempotent.

---

# 9. Team distinction and selection

Species tint alone is not a team marker. Add a low-cost readable cue:

* a small mask ring under the selected/active unit using the step-13 overlay model; and
* player/enemy color on the corresponding HUD card.

Prefer the overlay ring because it already conforms to shaped surfaces. Extend
presentation with a separate non-rule ring entity/material, or reuse the existing `Hovered`
mask only when no higher-priority interaction overlay owns that cell. Do not extend the
locked eight-value `BattleMaskKind`/atlas contract. Selected action/invalid/hover masks must
still win over the passive active-unit cue.

Do not recolor the whole species cube by team; authored form tint should remain visible.
Do not add a collider or entity ray-picker. Targeting still uses board support cells and
stable IDs through `BattleQueryService`.

---

# 10. Pure HUD view-model layer

Widgets must not traverse mutable battle state. Create a pure builder that receives copied
state and immutable definitions:

```cpp
struct ResourceViewModel
{
    std::int64_t current = 0;
    std::int64_t maximum = 0;
    std::string label;
};

struct CreatureCardViewModel
{
    BattleUnitId unit;
    std::optional<CreatureInstanceId> creature; // player persistent participant only
    BattleSide side;
    std::string displayName;
    std::array<std::uint8_t, 4> tint;
    ResourceViewModel health;
    std::int64_t shield = 0;
    BattleTime readyAt;
    BattleTime readyIn;
    bool placed = false;
    bool defeated = false;
    bool active = false;
    bool selectedForPlacement = false;
};

struct AbilitySlotViewModel
{
    std::size_t shortcut = 0; // 1..8
    std::optional<std::string> abilityId;
    std::string name;
    std::string costText;
    std::string rangeText;
    bool selected = false;
    bool enabled = false;
    std::optional<CommandRejection> disabledReason;
};

struct TurnForecastEntryViewModel
{
    BattleUnitId unit;
    std::string shortName;
    BattleSide side;
    BattleTime readyAt;
    BattleTime offsetFromNow;
};

struct BattleHudViewModel
{
    BattlePhase phase;
    std::vector<CreatureCardViewModel> playerTeam;
    std::vector<CreatureCardViewModel> opponentTeam;
    std::vector<TurnForecastEntryViewModel> forecast;
    std::optional<ActiveUnitViewModel> activeUnit;
    std::array<AbilitySlotViewModel, 8> abilities;
    DeploymentViewModel deployment;
    std::string statusLine;
};
```

Use the exact resource scalar types established by combat; the example uses integers only
to show ownership. HP/AP/MP are never formatted from floats.

`BattleHudViewModelBuilder::build` receives:

* `BattleSnapshot`;
* immutable registries;
* pure `BattleQueryService`;
* copied `BattleInteractionState`;
* a pure scheduler forecast;
* optional last typed rejection/status message.

It returns owned strings/vectors and holds no source reference. Sort team cards by authored
roster/spawn order, not current entity order. Keep empty slots so each team column visibly
supports six.

Add equality to view models. `BattleHudWidget::render(model)` updates only fields whose
values changed; geometry refresh is separate.

---

# 11. Scheduler forecast and stamina display

The GDD requires the player to see turn-bar/stamina progression. Battle uses fixed
`BattleTime` milliseconds; do not decrement a countdown from wall-clock frames.

Add or use a pure scheduler query equivalent to:

```cpp
[[nodiscard]] std::vector<TurnForecastEntry> forecastActivations(
    const BattleSnapshot& snapshot,
    std::size_t maximumEntries);
```

It must:

* operate on a copy/value projection;
* use the exact readiness/stamina formula and canonical tie-breaker from step 07;
* not mutate the session heap, generation counters, current time, log, or RNG;
* omit defeated/removed units;
* return a bounded list, for example the next 10 activations;
* handle a currently active unit explicitly rather than scheduling it twice;
* be deterministic for equal-ready-time units.

Display `readyIn = max(0, readyAt - snapshot.elapsed)` on team cards. During player input,
battle time is stopped, so the number does not tick with real seconds. Refresh when the
scheduler advances or state changes, not every frame.

This is explicitly a **bar-only projection**, not a cloned future battle simulation. Copy
current fill/effective stamina/tie metadata; treat currently paused/stunned units as
unavailable for the forecast horizon; repeatedly advance the copied unpaused bars to the
next ready boundary, append the canonical winner, and reset only that copied winner's bar
to zero. If an activation is currently in progress, assume it ends now for projection,
reset that actor's copied bar, and do not list the already-running activation again. Do not
run activation hooks, object triggers, AP/MP refill, owner-duration decrements, timeline
expiry, DOT/HOT, modifier changes, commands, defeat, or outcome.

The strip may show repeated fast units under those assumptions and may return fewer than
the requested entries when all remaining units are paused. Label/document it as guidance.
It is expected to match the real scheduler only in a fixture with no status/object/timeline
changes and immediate no-op EndTurn activations. After every real committed batch it is
rebuilt from the new snapshot. It is never a reserved schedule or rule input.

---

# 12. Widget tree

Implement a persistent `BattleHudWidget` as a child of `GameSceneWidget`. Construct it once,
deactivate it in Exploration, bind/activate it only after battle presentation has attached,
and unbind/deactivate it before session destruction.

Suggested tree:

```text
BattleHudWidget
├── PlayerTeamPanel
│   └── 6 × CreatureCardWidget
├── TurnForecastWidget
│   └── up to 10 compact TurnEntryWidget
├── OpponentTeamPanel
│   └── 6 × CreatureCardWidget
├── ActiveUnitPanel
│   ├── name + phase/status line
│   ├── HP ResourceBarWidget + shield label
│   ├── AP ResourceBarWidget
│   ├── MP ResourceBarWidget
│   ├── stamina/ready label
│   ├── compact status list
│   ├── selected ability summary
│   ├── AbilityBarWidget
│   │   └── 8 × AbilitySlotWidget
│   └── EndTurn PushButton
└── DeploymentPanel
    ├── selected creature/instruction label
    └── Ready PushButton
```

The Team and Battle UI wireframes define the grouping. Use Sparkle layouts where they make
geometry simpler, but explicit `_onGeometryChange` calculations are acceptable.

Create a read-only `ResourceBarWidget` rather than abusing interactive `SliderBar`:

* background `Panel`;
* fill `Panel` clipped/sized to a clamped ratio;
* centered `TextLabel` with exact current/maximum;
* zero maximum yields ratio zero, not divide-by-zero;
* negative/over-max values are visibly clamped but text may expose the authoritative value
  with a diagnostic.

Widgets own every `subscribeToClick` contract as a member. Dropping the widget/binding drops
the contract.

---

# 13. Responsive layout

At the 800x600 minimum:

* player team column hugs the left edge;
* opponent team column hugs the right edge;
* forecast is a narrow strip inside/adjacent to the left column without covering center;
* active panel is centered at the bottom;
* deployment instructions/Ready occupy the active-panel region;
* the tactical board remains visible in the center;
* the F7 overlay may overlap in debug mode, but normal HUD panels must not overlap each
  other.

Use margins and clamp sizes rather than assuming the 1103x758 wireframe dimensions.
Recommended proportional caps:

```text
team panel width:     clamp(viewport * 0.18, 150, 240)
active panel width:   clamp(viewport * 0.55, 480, 820)
active panel height:  150..210
edge margin:          8..16
ability slots:        eight equal cells with 4..8 spacing
```

For narrower-than-supported debug windows, retain controls by reducing spacing/text before
letting panels overlap. Document 800x600 as the supported prototype minimum.

The HUD must receive geometry updates when `GameSceneWidget::_onGeometryChange` runs. Mouse
hit regions and visual rectangles must remain aligned after resize.

Ensure widget child ordering routes pointer input to HUD buttons before step-13 board input.
A click on an ability/Ready/End Turn button must be consumed and must not also select a
board cell underneath it.

---

# 14. Creature team cards

Each column always contains six reusable `CreatureCardWidget` instances. Render:

* tinted species/form swatch (an icon placeholder);
* display name;
* HP current/maximum and fill;
* compact shield value when nonzero;
* ready-in/stamina text;
* active border/state;
* defeated/empty state;
* placed marker during Deployment.

During Deployment, player cards with a persistent creature ID are selectable. A click
calls:

```cpp
interaction.selectPlacementCreature(creatureInstanceId);
```

It does not submit a placement itself; the next legal board click in step 13 submits
`PlaceUnitCommand`. Clicking an already placed creature selects it for repositioning
through another `PlaceUnitCommand`. The card becomes selected only from controller state,
not optimistically from button state.

Opponent cards are informational. Do not expose hidden future abilities if battle rules
later add information hiding; this prototype may show species/name/HP/stamina because the
GDD has no hidden-information rule.

Rich grow-on-hover panels from `Creature_UI_Element.png`, draggable creature windows,
passive tooltips, and complete stat sheets are deferred. Preserve room for them without
implementing them now.

---

# 15. Active-unit panel and ability bar

Render the active unit only when `BattlePhase::Activation` has an active unit. During enemy
cosmetics, show the enemy active unit but disable all command controls.

The panel shows:

* display name and side;
* HP/max HP and shield;
* AP/max AP;
* MP/max MP;
* stamina or next-ready metadata;
* active statuses with remaining duration/stack count, capped with `+N` overflow;
* selected ability name, cost, range/area, and LoS requirement;
* eight fixed ability slots;
* End Turn.

Do not display obsolete Unity terminology `PA`/`PM`; Playground uses `AP`/`MP`.

Ability ordering comes from `DerivedCreatureState`/battle snapshot in the stable order
established by data parsing and Feat reward replay. The first eight map to shortcuts 1..8.
Paging beyond eight is not needed because the prototype combat contract caps the equipped
visible set at eight; if prerequisite data allows more, diagnose and show the first eight
without silently changing battle data.

Use `BattleQueryService` to determine each slot's current usability and typed rejection.
Typical disabled reasons include insufficient AP/MP, status restriction, wrong
phase/owner, or no legal anchor. Do not locally compare only resource numbers.

## 15.1 Disabled button wrapper

`spk::PushButton` has no disabled state. Implement it in `AbilitySlotWidget`:

```cpp
class AbilitySlotWidget : public spk::Widget
{
private:
    spk::PushButton _button;
    spk::PushButton::Contract _clickContract;
    bool _enabled = false;
    std::optional<std::string> _abilityId;
    std::function<void()> _onEnabledClick;

public:
    void render(const AbilitySlotViewModel&);
    [[nodiscard]] bool enabled() const noexcept;
};
```

The stored callback checks `_enabled` and `_ability` before forwarding. Disabled styling is
visually distinct on both released and pressed subwidgets. Do not merely dim it while still
allowing `selectAbility`.

The slot click calls `BattleInteractionController::selectAbility(shortcut - 1)`. Keyboard
1..8 already calls that exact method. Selection highlights only after the controller
accepts and publishes its new interaction state.

End Turn uses the same guarded wrapper policy and calls `interaction.endTurn()`. It is
enabled only for a player-owned active unit in an accepting interaction state. Space and
click therefore share the same command path.

## 15.2 Ability summary

Use a pure formatter over `AbilityDefinition` and its normalized effect definitions to show
short prototype text:

```text
Ember Arc — 2 AP
Range 1–4 · radius 1 · LoS
Magic damage; applies Burn (2 turns)
```

Never evaluate formulas or roll values while formatting. Generated rich colored markup and
icons from `Action_UI_Element.png` are later polish; plain text is sufficient here.

---

# 16. Deployment HUD

When `BattlePhase::Deployment`:

* activate the deployment panel;
* keep player/opponent team cards visible;
* hide/deactivate the active ability panel and End Turn;
* display `Select a creature, then click a highlighted support cell.`;
* show placed count and any typed validation message;
* enable Ready only if a pure preview of `ConfirmDeploymentCommand` is currently accepted;
* clicking Ready calls `interaction.confirmDeployment()`;
* Enter calls that same method from step 13.

Do not auto-select the first roster member. Do not auto-ready after the last placement.
Do not modify occupancy in a card or button callback.

After any accepted `PlaceUnitCommand` event span:

1. reconcile cube views from the resulting snapshot;
2. rebuild the HUD model;
3. rebuild deployment overlay from the controller/query;
4. only then accept another click.

A rejected placement leaves prior views/cards intact and shows the typed reason plus the
short invalid mask from step 13.

---

# 17. Binding and refresh model

The HUD is event-driven at semantic boundaries, not frame-polled.

Reuse `BattleSnapshotProvider` from step 13; do not redeclare a look-alike alias. Create a
binding facade equivalent to:

```cpp
class BattleHudBinding
{
public:
    void attach(
        BattleSnapshotProvider snapshots,
        const Registries& registries,
        const BattleQueryService& queries,
        BattleInteractionController& interaction);
    void onCommittedBatch(const BattleBatchPublication& publication);
    void onInteractionChanged(const BattleInteractionState&);
    void detach() noexcept;
};
```

The binding stores its attached `{BattleRuntimeGeneration, BattleId}` and ignores a
publication unless both values match before reading its batch or snapshot provider. Every
deferred animation/timer callback repeats that check; `detach()` invalidates the stored
generation before releasing widgets/entities/contracts.

Refresh on:

* initial attach;
* one completed command/event batch;
* phase/active-unit/scheduler advance;
* interaction selection/hover rejection that changes HUD text or selected slot;
* cosmetic queue busy/idle if it affects enabled state;
* terminal staging.

Mouse hover that only changes a board mask need not rebuild all team cards. Coalesce
multiple notifications raised inside one gateway submit and build once from the final
copied snapshot.

The binding must never cache a raw `BattleSession*`. Use the safe provider/callback facade
from step 13, clear it in `beginExit`, drop all contracts, and deactivate the tree.
`render(model)` must be callable with a synthetic view model in widget tests without a
battle session.

---

# 18. Terminal and busy presentation

While enemy AI or a committed cosmetic queue is active:

* leave informational cards/forecast/resources visible;
* disable ability, placement, Ready, and End Turn buttons;
* show `Enemy acting…` or `Resolving…` in the status line;
* do not run a second AI pump from the widget.

At Terminal, show a small `Victory`, `Defeat`, `Draw`, or `Technical error` status line; the
last case uses the typed abort reason's short mapping, never exception text. This is only a
temporary banner. Do not add a modal result screen or auto-exit here: step 18 supersedes the
step-12 temporary transition and owns the persistent result/commit/Continue flow.

If teardown occurs before a cosmetic animation completes, fast-forward/clear it and detach.
No animation may keep the mode alive.

---

# 19. Expected files

Adapt names to the actual post-step-13 tree, but expect:

```text
Playground/srcs/battle/presentation/
├── battle_unit_position.hpp/.cpp
├── battle_unit_presenter.hpp/.cpp
├── battle_unit_view.hpp
├── battle_object_presenter.hpp/.cpp
├── battle_object_view.hpp
├── battle_hud_view_model.hpp
├── battle_hud_view_model_builder.hpp/.cpp
└── ability_summary_formatter.hpp/.cpp

Playground/srcs/widgets/battle/
├── battle_hud_widget.hpp/.cpp
├── creature_card_widget.hpp/.cpp
├── team_panel_widget.hpp/.cpp
├── turn_forecast_widget.hpp/.cpp
├── resource_bar_widget.hpp/.cpp
├── ability_slot_widget.hpp/.cpp
├── ability_bar_widget.hpp/.cpp
├── active_unit_panel.hpp/.cpp
└── deployment_panel.hpp/.cpp

Playground/tests/
├── battle_unit_position_tests.cpp
├── battle_unit_presenter_tests.cpp
├── battle_object_presenter_tests.cpp
├── battle_hud_view_model_tests.cpp
├── battle_hud_binding_tests.cpp
├── battle_widget_logic_tests.cpp
└── turn_forecast_tests.cpp
```

Wire these into existing CMake targets. Keep Sparkle-generic widget changes out of scope;
`AbilitySlotWidget` and `ResourceBarWidget` are Playground components.

The presentation file set also includes
`battle_object_view.hpp`, `battle_object_presenter.hpp/.cpp`, and
`battle_object_presenter_tests.cpp`. `BattlePresentationRuntime` owns and detaches both the
unit presenter and object presenter before session/board teardown.

---

# 20. Automated verification

Use deterministic synthetic definitions/snapshots and a fake command gateway.

## 20.1 Position and visual conversion

Prove:

* cube, slab, oriented/flipped slope, and stair positions use correct walk height;
* a translated live-board origin and a zero-origin handcrafted arena both use
  `toPresentationCell`, never raw coordinate equality or a required `toLiveWorldCell`;
* rendered half-height and clearance are applied once;
* segment interpolation follows the same surface helper as exploration;
* tint bytes map exactly to normalized engine color;
* scalePermille 250, 1000, and 3000 produce exact finite scales;
* fallback color is stable by semantic ID and battle-RNG independent.

Run cube/slab/slope/stair and segment-interpolation cases through both presentation-source
bindings. Assert equivalent local geometry yields the same relative placement, a
handcrafted presenter never touches a `VoxelWorld`/world-anchor fake, and malformed
non-translational or duplicate mappings fail attach transactionally before any entity is
registered.

## 20.2 Presenter

With a fake engine/entity registration seam, prove:

* initial reconciliation creates one view for each already placed snapshot unit;
* duplicate placement/events cannot create a second view;
* reposition reuses the entity;
* movement consumes committed `UnitMovementStep` events rather than recomputing a path;
* duplicate sequences are ignored;
* a sequence gap snaps/reconciles and diagnoses;
* damage/heal/defeat cues never mutate the snapshot or event log;
* fast-forward reaches the snapshot cell;
* detach removes every registered entity before destruction and is idempotent.

For the battle-object presenter, prove attach/reconciliation from an already populated
snapshot, stable same-cell slot order, one view per ID, place/trigger/remove event handling,
same-batch trigger-then-remove transient feedback, gap recovery from the snapshot, no
content-ID or board-source branch, correct live and handcrafted presentation placement, and
complete idempotent engine removal on teardown. Assert its update and
fast-forward paths leave the battle state/event/RNG digest unchanged.

## 20.3 View model

Prove:

* teams stay in authored order with six slots;
* current form supplies name/tint/scale metadata;
* HP/AP/MP/shield/status text matches the copied snapshot;
* bar-only forecast uses step-07 fill/tie math, resets projected winners, may repeat fast
  units, omits paused units, and does not list the current activation twice;
* a no-status/no-object immediate-EndTurn fixture matches the real scheduler, while a
  timed-stun/hook fixture is explicitly not claimed as an exact future schedule;
* forecast leaves session/log/RNG digests unchanged;
* ability ordering is stable and capped at eight visible slots;
* enabled state is exactly the query service result;
* deployment Ready is exactly confirm-command preview;
* equal inputs produce equal models.

## 20.4 Widget logic

Test without GPU pixels:

* resource ratio clamps and zero maximum is safe;
* disabled `AbilitySlotWidget` does not forward a click;
* enabled click and keyboard-facing call reach the same interaction method/slot;
* Ready and Enter-facing call reach the same confirm method;
* End Turn and Space-facing call reach the same method;
* model equality avoids redundant updates;
* deactivated HUD consumes no battle input;
* a HUD click is consumed before board input;
* geometry at 800x600 and wide aspect keeps rectangles inside the viewport and nonoverlap.

## 20.5 Binding and lifecycle

Run:

```text
Exploration
  -> Battle Deployment (place/reposition/Ready)
  -> player activation
  -> enemy batch/cosmetics
  -> Terminal
  -> Exploration
  -> repeat once
```

Assert:

* one HUD binding and one click contract per button;
* no snapshot poll on ordinary frame update;
* one coalesced model build per committed batch;
* player renderer hidden but player entity/streamer active in battle;
* fluid plus source-specific streaming/render-visibility invariants from step 12 remain
  true for live and handcrafted runs;
* no unit entity/contract remains after exit;
* second battle starts with empty transient animation/selection/model state;
* destroying the widget during movement animation is safe;
* unit/object entities detach before the handcrafted arena renderer/grid and before the
  engine, and generated-world visibility restores exactly.

Run the complete Playground test suite with the corrected step-01 preset.

---

# 21. Visual verification

Run the real executable:

1. Enter a debug battle and confirm both six-slot team columns and bottom panel leave the
   central board visible.
2. Select each player card, place/reposition its cube on cubes, a slope, and stairs, then
   press Ready.
3. Verify authored form tints/scales, player/enemy distinction, shadows, and support-surface
   placement.
4. Compare HP/AP/MP/stamina/card values with F7 snapshot diagnostics.
5. Verify the turn forecast changes only when simulated battle time/scheduler changes.
6. Use ability buttons and 1..8; selected mask/slot and enabled state must agree.
7. Verify a disabled ability cannot be fired by mouse or keyboard.
8. Move across shaped terrain; the cube follows the committed path without rule delay.
9. Observe simple damage, heal, shield, status, and defeat feedback.
10. Click HUD controls over the board and verify no accidental board target.
11. Resize to 800x600 and a wide aspect; controls remain readable and clickable.
12. Exit/re-enter battle; confirm no duplicate cube, card, click, or stale animation.
13. Return to exploration and verify its player cube, camera, hover, movement, fluid, and
    portals resume.
14. Repeat deployment, movement, an object cue, and teardown in the authored handcrafted
    arena. Verify unit/object feet follow its shaped surfaces, only the arena is visible,
    and exploration rendering returns exactly on exit.

The monochrome Unity wireframes are grouping references. Do not block this step on final
palette, typography, icon art, or pixel-perfect fidelity.

---

# 22. Documentation

Update:

* `Playground/docs/03-systems/ui-hud.md` to replace the obsolete “M1 ships no HUD” plan
  with the implemented prototype tree, data flow, controls, and deferred rich panels;
* the battle presentation page from step 13 with unit-view lifecycle and cosmetic-event
  rules;
* `Playground/README.md` with deployment/ability/End Turn controls and the 800x600 minimum;
* relevant architecture/diagram documentation with snapshot -> view model -> widget and
  intent -> interaction controller -> gateway arrows.

State explicitly:

* JSON placeholder visuals are presentation-only;
* unit scale does not change tactical footprint;
* widgets subscribe/refresh at semantic boundaries and never mutate rules;
* movement animation consumes committed path events;
* `Actor.cell` and battle positions are solid support cells;
* battle views use board-to-presentation coordinates; `toLiveWorldCell` is live-only and neither
  the HUD nor unit/object presenter depends on it.

---

# 23. Non-goals

Do not implement:

* final creature models, sprites, animation rigs, particles, audio, or art pipeline;
* rich text markup or final icons;
* full grow-on-hover creature sheets/passive cards;
* a runtime content/roster editor;
* ability paging beyond the prototype equipped set;
* taming conditions or capture controls;
* Feat progress/evolution panels;
* modal result/reward/save UI;
* exploration HUD/main menu;
* damage simulation or random preview;
* world-space health bars/floating combat text;
* entity/collider target picking;
* alternate commands from button callbacks;
* a scheduler that waits for animations.

---

# 24. Acceptance criteria

This step is complete only when:

* every placed unit has one correctly positioned cube view using current-form JSON tint and
  scale for either board source;
* cube/slab/slope/stair position tests and visual checks pass through live-world and
  handcrafted presentation bindings;
* view creation/reposition/movement/defeat/detach follow committed snapshots/events;
* every battle object has one generic correctly positioned marker, and trigger/removal cues
  are observable, bounded, snapshot-reconciled, and simulation-neutral;
* cosmetic queues cannot mutate or delay headless battle state;
* HUD matches the wireframe grouping with two six-slot columns, forecast, active resources,
  eight abilities, deployment Ready, and End Turn;
* manual placement selects by `CreatureInstanceId` and submits only through the step-13
  controller/gateway;
* ability/Ready/End Turn mouse and keyboard paths converge on the same controller methods;
* disabled buttons cannot forward clicks;
* forecast and view-model building are deterministic and read-only;
* HUD updates are semantic-event driven and coalesced;
* HUD input consumes clicks before the board picker;
* 800x600 and wide layouts pass geometry/visual checks;
* player entity/streamer, fluid pause, pinned chunks, and queued transitions retain step-12
  live-world behavior, while handcrafted runs retain isolated-arena ownership and exact
  generated-world visibility restoration;
* two battle round trips and mid-animation destruction leave no stale entity or contract;
* full builds/tests pass and documentation reflects the implemented state.

---

# 25. Handoff to step 15

In the completion report, provide:

* exact snapshot and event payloads used by the presenter;
* how `UnitMovementStep` events are grouped/validated into the cosmetic path and effective
  movement cost;
* the last-consumed event sequence/idempotency behavior;
* the pure HUD/forecast/query APIs;
* widget bind/unbind and unit-entity detach order;
* the source-agnostic `toPresentationCell`/surface-height path shared by unit and object
  presenters and its handcrafted-arena teardown dependency;
* final controller methods called by buttons;
* current-form `PlaceholderVisual` conversion and fallback;
* test commands and remaining presentation limitations.

Step 15 will consume the same typed event log for cross-cutting condition evaluation. Do
not make it scrape HUD text, cosmetic queues, or entities.
