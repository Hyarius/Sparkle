# Implement the battle overlay, tactical camera, and player input

Make both battle presentation sources created in step 12 directly playable with the existing
Sparkle camera, mouse, keyboard, and mask texture. Ordinary battles overlay the frozen live
world; authored Gym/Special boards render through the isolated arena binding in the same
`GameSceneWidget`. The player must be able to deploy creatures, inspect legal cells, choose
movement or an ability, preview the exact result accepted by
the headless validator, and submit commands through the same gateway used by AI and tests.

This is an implementation task. Modify Playground presentation/input code, extract the
existing private walk-surface mask builder into a reusable component, add focused tests,
update the implemented-system documentation, and keep the complete build green. Do not add
the final creature render presenter or HUD in this step; those are step 14.

---

# 1. Repository baseline

Implement against the repository after steps 01-12. Reinspect the named files and use the
actual prerequisite APIs from their handoff reports. If a type below has an equivalent
settled name, extend that type instead of creating a second abstraction.

At the original `6ae3151` baseline:

* `GameSceneWidget` owns one `spk::Camera3D` and the exploration player, hover, and camera
  components;
* `ExplorationInputLogic` derives a ray with
  `Camera3D::rayFromViewport(viewport, mouse)`, calls `MousePicker::pickStandable`, and
  sends `actorMoveRequested`;
* `MousePicker` calls `spk::VoxelRayCast::cast` against solid world voxels and then searches
  downward for a `WorldNavigation` node;
* `CameraControllerLogic` follows the exploration actor, rotates on right drag, zooms on
  mouse wheel, and is gated by `world.explorationActive`;
* the exploration hover mesh is rebuilt from private functions in the anonymous namespace
  of `Playground/srcs/logics/exploration_input_logic.cpp`:
  `maskUV`, `appendWalkSurface`, and `buildHoverMesh`;
* that mesh includes every transformed polygon whose normal has positive Y, so cubes,
  slabs, slopes, and stair treads receive a shape-conforming overlay;
* the mask atlas is `resources/textures/mask.png`, currently treated as four columns by
  four rows, and the overlay is lifted by 0.02 world units to prevent z-fighting;
* `spk::Widget` propagates input to active children before the parent and stops once an
  event is consumed.

After step 12, `GameSceneWidget` must additionally own a `ModeManager` and a nullable
`BattleModeRuntime`. A battle runtime owns a headless `BattleSession` built over frozen
`BoardData`, one `LiveWorldBattleLease | HandcraftedArenaLease`, and a source-agnostic
presentation binding. It exposes a read-only snapshot, accepts every action through the
common command gateway, and remains alive until a queued post-update exit is committed.
Fluid simulation is paused during battle. Live-world rendering/streaming remain active for
live boards; generated-world rendering is hidden and an immutable isolated arena is active
for handcrafted boards. Do not weaken those lifecycle rules here.

The headless contracts from steps 05-11 are authoritative:

* a board cell identifies the solid support voxel, not the air space above it;
* occupancy, paths, deployment zones, movement cost, range, line of sight, and area shapes
  come from `BoardData` and battle services;
* placement, move, ability, and end-turn actions are value commands;
* validation/preview is read-only and command execution is atomic;
* rejected commands do not mutate state, consume RNG, allocate semantic window IDs, or
  append gameplay events;
* AI, player input, debug autoplay, and tests all submit through the same public gateway.

If those seams do not exist, stop and finish their earlier steps. Never implement a second
set of tactical rules in presentation code.

---

# 2. Required final behavior

After this step:

1. Exploration still behaves and renders exactly as before when no battle exists.
2. Entering battle blends the existing camera to a tactical framing of the frozen board.
3. Right drag orbits the tactical view and the mouse wheel zooms it. The exploration
   follow controller cannot compete for the camera.
4. Hovering a board cell shows one shape-conforming mask on the active presentation source's
   voxel support surface.
5. Legal deployment, reachable movement, hovered path, ability range, LoS-blocked aims,
   area-of-effect preview, hover/selection, and invalid feedback use the eight exact
   deterministic mask categories from step 01.
6. The cells displayed as legal are derived from the same read-only preview/validator that
   will accept the eventual command.
7. Board picking cannot select a world-navigation node outside the frozen battle footprint,
   a different height in the same column, or mutable terrain discovered after entry.
8. Production battle placement is manual. The player selects a roster creature, clicks a
   legal deployment cell, may reposition it, and explicitly confirms placement.
9. During an activation, the player can select movement or one of up to eight abilities,
   cancel selection, inspect a preview, confirm a target, and end the turn.
10. Keyboard and mouse use the same controller methods and produce the same commands.
11. UI/input never edits `BattleUnit`, board occupancy, the scheduler, AP/MP/stamina, or
    effects directly.
12. Input is ignored while the battle is resolving a batch, AI owns the activation, a
    mode transition is queued, or the session is terminal.
13. Leaving battle destroys all overlay entities and contracts before the battle session,
    restores the exploration camera smoothly, clears stale hover state, and does not move
    or mutate either cell source.
14. Fluid remains paused for the entire battle. Live streaming remains active for a
    live-world board; a handcrafted arena never requests/pins world chunks and stays
    presentable from its owning immutable grid.
15. Camera framing, overlay building, and picking use `BoardData` presentation mapping/AABB
    and the active presentation cell source. None of them require a `VoxelWorld` or assume a
    board is live-world based.

The result is an input-complete, manually controllable battle checkpoint. Step 14 makes it
the first readable playable battle with creature cubes and a HUD; nothing in this step may
depend on that UI.

---

# 3. Ownership and presentation boundary

Keep presentation lifetime subordinate to `BattleModeRuntime` and keep the headless session
independent of Sparkle objects:

```text
GameSceneWidget
├── Camera3D                         persistent across modes
├── exploration controllers/entities persistent but inactive/hidden in battle
├── ModeManager                     authoritative mode and queued transitions
└── BattleModeRuntime               only while Battle is committed
    ├── BattleSession               headless authority
    ├── BattlePresentationBoardBinding
    │   └── live-world adapter | handcrafted-arena adapter
    ├── BattlePresentationRuntime
    │   ├── TacticalCameraController
    │   ├── BattleBoardPicker
    │   ├── BattleOverlayPresenter
    │   └── BattleInteractionController
    └── contracts / transient input state
```

The exact owner may be a field of `BattleModeRuntime` or a widget-owned object explicitly
bound/unbound with that runtime. The following rules are mandatory:

* no presentation object outlives the session/snapshot provider it observes;
* callbacks capture a stable presentation owner, never a raw session pointer whose
  destruction can precede the contract;
* every callback/queued batch captures the runtime's `BattleRuntimeGeneration` and
  `BattleId`; it returns without touching state unless both still match the active
  presentation runtime;
* all event contracts are released before `BattleSession` is destroyed;
* render entities are removed from the game engine before their C++ owner is destroyed;
* entering and leaving battle twice must not duplicate input subscriptions or renderers;
* a failed transactional entry creates no active tactical controller or overlay;
* the presentation runtime borrows one immutable source binding from the active environment
  lease and never switches source kind during a session;
* `BattleSession`, `BoardData`, battle definitions, and commands contain no
  `spk::Entity3D`, mesh, widget, mouse, keyboard, or camera type.

`beginExit()` first invalidates the presentation runtime's generation, then disables input,
releases contracts, and removes entities. Do not use `BattleId` alone as a lifetime token:
its deterministic 32-bit fold may collide across separate sessions.

Define the small presentation-facing value/callback contracts once in this step and reuse
them from step 14:

```cpp
using BattleSnapshotProvider = std::function<BattleSnapshot()>;

struct BattlePresentationCallbacks
{
    std::function<void()> presentationStateChanged;
    std::function<void(const CommandResult&)> commandCompleted;
};
```

`commandCompleted` receives the exact `AcceptedCommand | RejectedCommand | AbortedCommand`
returned by `BattleSession::submit`; it does not synthesize a presentation-only result.
Both callbacks are cleared by `beginExit()` and every bound callable performs the generation
and battle-ID guard above before touching its owner. They never capture a raw session
pointer.

`BattlePresentationRuntime` should expose explicit lifecycle:

```cpp
struct BattlePresentationBoardBinding;

class BattlePresentationRuntime
{
public:
    BattlePresentationRuntime(
        spk::GameEngineWidget& engine,
        spk::Camera3D& camera,
        BattleSession& commands,
        BattleSnapshotProvider snapshots,
        BattlePresentationCallbacks callbacks);

    void enter(BattlePresentationBoardBinding boardPresentation);
    void update(const spk::UpdateContext& tick);
    void beginExit();
    void detach() noexcept;
};
```

Treat this signature as a responsibility contract, not permission to duplicate a settled
step-12 type. `detach()` must be idempotent and safe during exception unwinding.

The binding is a non-owning, source-agnostic view whose lifetime is guarded by the runtime
generation. It provides at least:

```cpp
struct BattlePresentationBoardBinding
{
    const BoardData& board;
    const IBattlePresentationCellSource& cells;
    PresentationAabb bounds;
    BoardSourceDescriptor source;
};
```

`PresentationAabb` is the finite float-space union of every presentable support shape after
board-to-presentation translation, including its actual top surface rather than only integer
cell centers. It is precomputed once at successful attach, rejects an empty/non-finite box,
and is presentation-only; it is not stored in battle state or a deterministic simulation
digest.

`IBattlePresentationCellSource` accepts **presentation-space** integer cell coordinates and
returns the voxel/render-shape data needed by the mask builder and picker. The live adapter
queries `VoxelWorld`; the handcrafted adapter queries the arena's owning immutable grid.
This is a presentation adapter, not another traversal/LoS authority: gameplay continues to
use `BoardData::cells()` and frozen board-local coordinates. Reuse an equivalent settled
step-12 name instead of creating a duplicate interface.

---

# 4. Extract the reusable walk-surface mask mesh builder

The current battle overlay cannot call the hover builder because it is private to
`exploration_input_logic.cpp`. Extract it; do not copy it.

Create the equivalent of:

```cpp
namespace pg
{
    struct WalkSurfaceMaskInstance
    {
        spk::Vector3Int supportCell;
        spk::AtlasCell atlasCell;
        float lift = 0.02f;
    };

    class WalkSurfaceMaskMeshBuilder
    {
    public:
        [[nodiscard]] static spk::TextureMesh3D build(
            const IBattlePresentationCellSource& cells,
            std::span<const WalkSurfaceMaskInstance> instances);

        [[nodiscard]] static spk::TextureMesh3D buildOne(
            const IBattlePresentationCellSource& cells,
            const spk::Vector3Int& supportCell,
            const spk::AtlasCell& atlasCell,
            float lift = 0.02f);
    };
}
```

Place it in `Playground/srcs/rendering/walk_surface_mask_mesh_builder.hpp/.cpp` or the
project's established equivalent. Move, rather than reinterpret, these semantics:

* resolve the actual `spk::VoxelCell` from the support coordinate;
* return no geometry for an empty or unknown support voxel;
* get its registered render shape;
* transform inner and outer faces using voxel orientation and flip;
* emit only polygons with at least three vertices and transformed normal Y above 0.0001;
* normalize each polygon's original UV bounding box back to 0..1;
* map normalized UVs into the requested mask-atlas cell;
* lift vertices vertically to prevent z-fighting;
* bake a new immutable mesh and swap it through a `shared_ptr`, because the render thread
  may still own the prior mesh.

Here `supportCell` is explicitly a presentation-space support coordinate. Exploration wraps
its `VoxelWorld` in the same small adapter (or an equivalent generic cell/render-shape
source), while battle passes the step-12 binding. Do not downcast the interface to
`VoxelWorld`; an authored arena must produce identical cube/slope/stair mask geometry from
its immutable local source.

Do not hard-code the atlas dimensions in two callers. Give the builder an explicit atlas
layout or one shared `MaskAtlasLayout{4, 4}` constant. Reject/outcome-test an atlas cell
outside that layout in debug/test builds. Preserve the exact visual output of the
exploration hover.

Refactor `ExplorationInputLogic::_rebuildHover` to call `buildOne`. This extraction is
complete only when the old anonymous `maskUV`, `appendWalkSurface`, and `buildHoverMesh`
implementations no longer exist.

For battle, build all currently visible cells into one mesh per overlay revision, not one
entity or draw call per cell. A single mesh containing instances with different atlas UVs
is sufficient.

`BattleOverlayPresenter` owns one engine-registered `spk::Entity3D` with
`TextureMeshRenderer3D`. Bind `_maskTexture`, set a readable translucent tint, disable
shadow casting/receiving for the mask, and swap immutable meshes. Do not reuse
`_hoverEntity`: exploration logic owns and clears that renderer, while battle teardown
needs an independent entity lifetime. Remove the battle entity from the engine before
destroying it.

---

# 5. Overlay model: categories, precedence, and revisions

Create a render-independent overlay model. It is a value snapshot of what should be drawn,
not a cache of game rules:

```cpp
enum class BattleMaskKind : std::uint8_t
{
    Deployment,
    Reachable,
    Path,
    AbilityRange,
    AreaPreview,
    LineOfSightBlocked,
    Hovered,
    Invalid
};

struct BattleMaskCell
{
    BoardCell cell;
    BattleMaskKind kind;

    auto operator<=>(const BattleMaskCell&) const = default;
};

struct BattleOverlayModel
{
    std::vector<BattleMaskCell> cells; // sorted by canonical BoardCell order
    std::uint64_t revision = 0;
};
```

Use this exact enum-to-`GameRules::overlayMasks` key mapping:

| Kind | Key |
|---|---|
| `Deployment` | `deployment` |
| `Reachable` | `movement` |
| `Path` | `path` |
| `AbilityRange` | `abilityRange` |
| `AreaPreview` | `areaOfEffect` |
| `LineOfSightBlocked` | `losBlocked` |
| `Hovered` | `hovered` |
| `Invalid` | `invalid` |

Selection and mouse hover intentionally share `Hovered`; add the selected cell and current
picked cell to that semantic category instead of inventing a ninth atlas key. Do not put
atlas coordinates in combat JSON or add content-only aliases.

Only one mask may win for one support cell. Apply this default priority from highest to
lowest:

```text
Invalid
Hovered
LineOfSightBlocked
AreaPreview
Path
AbilityRange
Deployment
Reachable
```

If `Hovered` would hide invalidity, show `Invalid`. If a hovered valid target needs both
meanings, the high-priority mask is sufficient and the interaction state remains visible
elsewhere in diagnostics. Do not layer coplanar masks.

Build the model from:

* committed battle phase and active unit;
* the controller's current selection state;
* read-only placement/movement/ability preview results;
* the board cell currently picked;
* the most recent rejected target and its short presentation-only timeout.

Increment `revision` only when the sorted effective `(cell, kind)` set changes. The
presenter rebuilds and swaps the mesh only when that revision changes. Mouse movement
inside the same support cell, a render frame, or an unrelated cosmetic event must not
allocate a new mesh.

The presenter converts each board-local support cell with
`BoardData::toPresentationCell`. It must not assume `boardCell == presentationCell`: a live
board maps to its world support cell, while a handcrafted board maps to the isolated arena's
presentation grid (whose origin is defined by the handcrafted builder, normally zero).
Draw only cells for which the active binding exposes a presentable support coordinate. Use
the matching `BoardData::fromPresentationCell` inverse for picking. Keep
`toLiveWorldCell` as a typed live-only conversion for systems that truly need exploration-world
coordinates; overlays, camera, unit views, and input do not.

---

# 6. Read-only tactical previews

Presentation must consume one canonical preview seam from step 08. If step 08 has separate
validators, wrap them behind a read-only facade without reimplementing them:

```cpp
struct PlacementPreview
{
    std::vector<BoardCell> legalCells;
    std::optional<CommandRejection> rejection;
};

struct MovementPreview
{
    std::vector<BoardCell> reachableCells;
    std::optional<MovePlan> hoveredPlan;
    std::optional<MoveCommand> command; // unit + destination only
    std::optional<CommandRejection> rejection;
};

struct AbilityPreview
{
    std::vector<BoardCell> inRangeAimCells;
    std::vector<BoardCell> validAimCells;
    std::vector<BoardCell> lineOfSightBlockedAimCells;
    std::vector<BoardCell> affectedCells;
    std::vector<BattleUnitId> affectedUnits;
    std::optional<CastAbilityCommand> command; // unit + abilityId + one anchor
    std::optional<CommandRejection> rejection;
};
```

This document does not require those exact return types. It requires these guarantees:

* preview and execution share target, path, range, LoS, AoE, occupancy, AP/MP, status, and
  phase logic;
* preview receives a const snapshot/state view;
* preview does not advance `BattleTime`, consume RNG, increment IDs, fire passives, execute
  effects, append events, or mutate caches visible in a state digest;
* preview collections are sorted canonically;
* a hovered movement plan exposes its canonical committed path as `Path`; ability cells
  that pass geometric range/anchor-domain checks but fail LoS are exposed separately as
  `LineOfSightBlocked`; valid anchors use `AbilityRange`, and captured AoE cells use
  `AreaPreview`;
* the eventual command is constructed from stable IDs and board cells, never a pointer to
  a rendered entity;
* the UI does not infer legality from mask color.

Add a regression test that takes a state digest and RNG-state digest before and after every
preview flavor and proves both are unchanged. For representative valid previews, submit
the returned command and prove validation accepts it. For a stale preview, allow the
gateway to reject safely; never bypass revalidation.

---

# 7. Board-aware mouse picking

Do not reuse `MousePicker::pickStandable` unchanged. It consults mutable
`WorldNavigation` and searches downward in an entire column, which is appropriate for
exploration but not for a frozen, possibly multi-level tactical board.

Create the equivalent of:

```cpp
struct BattlePick
{
    BoardCell boardCell;
    spk::Vector3Int presentationSupportCell;
    spk::Vector3 cellEntryPosition;
    float rayDistance = 0.0f;
};

class BattleBoardPicker
{
public:
    [[nodiscard]] std::optional<BattlePick> pick(
        const BoardData& board,
        const IBattlePresentationCellSource& presentationCells,
        const spk::Ray3D& ray,
        float maxDistance = 1000.0f) const;
};
```

The picker must use the board snapshot as the whitelist:

1. Build a lookup from `BoardData::toPresentationCell(boardCell)` to `BoardCell` when
   presentation attaches. Validate it is one-to-one, canonical, entirely within the
   binding's presentation AABB, and consistent with the inverse mapping.
2. Cast through `IBattlePresentationCellSource` with the existing voxel DDA semantics using
   normal solid/opaque occlusion. The nearest physical surface blocks the ray whether it
   belongs to the board or not. The live adapter includes live-world obstacles; the authored
   adapter includes every prefab voxel in the isolated arena.
3. Return a board cell only when that nearest hit is the exact solid support coordinate in
   the whitelist. A wall, prop, ceiling, higher support, or other non-board solid in front
   returns no pick; never click through it.
4. Return the exact mapped `BoardCell`. Never search downward for a different board node.
5. Reject a hit outside the presentation AABB/board footprint, a board hole, a coordinate
   absent from the active presentation source, or (for live boards) a cell removed after
   entry. A successfully constructed handcrafted source is immutable and cannot disappear.
6. Resolve ties deterministically by ray distance and then canonical cell order.

The current `spk::VoxelRayCast` deliberately traverses unit cell boundaries rather than
intersecting render-shape polygons. Keep that solid-cell occlusion contract for the
prototype and name the returned position as a cell-entry hit. If exact slope-polygon
intersection is added later, it may refine the hit within the nearest solid cell; it must
never skip that cell to select terrain behind it.

Use:

```cpp
const spk::Ray3D ray =
    camera.rayFromViewport(spk::Vector2(viewportSize), spk::Vector2(mousePosition));
```

The actual viewport size must come from the active game widget geometry, including resize.
Do not use the full monitor size or cache the initial window dimensions.

Because step 12 pauses fluid before freezing a live board and keeps it paused until
teardown, normal live terrain cannot change. The rejection in item 5 is still required as
defensive behavior and for lifecycle/failure tests. A handcrafted board ray-casts against
the immutable arena grid and is fully pickable; it is not treated as an unrendered synthetic
test source.

Pure headless `GridCellSource` fixtures may omit a presentation binding and then have no
picker. A production handcrafted board always has the renderer/source binding created by
`HandcraftedArenaLease`. This is still the same scene widget and camera, not a second scene
or second battle runtime.

---

# 8. Tactical camera controller

Reuse the existing `spk::Camera3D`. Both the live overlay and isolated authored arena are
rendered by the existing `GameSceneWidget`; a second camera or scene would split lifecycle,
input, and teardown authority.

Create a controller with explicit binding:

```cpp
struct TacticalCameraState
{
    spk::Vector3 target;
    float yawDegrees = 35.0f;
    float pitchDegrees = 48.0f;
    float distance = 16.0f;
};

class TacticalCameraController
{
public:
    void enter(const PresentationAabb& boardBounds, const spk::Camera3D& camera);
    void update(const spk::UpdateContext& tick, spk::Camera3D& camera);
    void onRightDrag(const spk::Vector2Int& delta);
    void onWheel(float delta);
    void beginExit(const spk::Vector3& explorationTarget);
    void reset() noexcept;
};
```

At entry:

* consume the validated presentation-space AABB from the active board binding (or recompute
  it exclusively from `toPresentationCell` plus presentable support surfaces in a test);
* target its X/Z center and the midpoint of its actual walking heights;
* derive a distance that fits the longer horizontal extent at the current field of view;
* clamp pitch to 25..75 degrees and distance to a board-aware minimum/maximum;
* seed interpolation from the camera's current position and target so entry does not snap.

Use frame delta only for cosmetic exponential interpolation, for example
`1 - exp(-8 * seconds)`. Camera motion must never enter a battle state digest or influence
ray-independent AI.

Controls:

* right-button drag changes yaw/pitch;
* wheel changes distance;
* optional WASD edge/pan may move the target only inside the presentation AABB plus a small
  margin;
* no key used for battle commands may simultaneously rotate the camera;
* clamp all values and guard a zero-size board.

The authoritative mode/control context from step 12 must make exactly one camera controller
active. On battle entry the exploration camera logic sees itself inactive before tactical
updates begin. On exit, tactical control stops first; exploration resumes from the current
camera state and its existing `_wasActive` path performs a smooth reacquisition of the
player. Do not restore a stale saved camera transform in one frame.

Unit-test the AABB/target/distance math without a GPU. Visually verify entry, orbit, zoom,
window resize, and return to exploration.

---

# 9. Battle interaction state machine

Centralize mouse, keyboard, and later HUD actions in one controller. Do not spread selection
state between `GameSceneWidget`, the overlay presenter, and step-14 buttons.

Use a closed state variant equivalent to:

```cpp
struct InteractionInactive {};
struct InteractionBusy {};
struct PlacementSelecting { std::optional<CreatureInstanceId> selected; };
struct PlacementReady {};
struct AwaitingAction { BattleUnitId unit; };
struct SelectingMovement { BattleUnitId unit; };
struct SelectingAbility { BattleUnitId unit; std::string abilityId; };

using BattleInteractionState = std::variant<
    InteractionInactive,
    InteractionBusy,
    PlacementSelecting,
    PlacementReady,
    AwaitingAction,
    SelectingMovement,
    SelectingAbility>;
```

`BattleInteractionController` owns:

* this transient state;
* current picked cell;
* current read-only preview;
* optional short-lived rejected-target feedback;
* references/callables for current snapshot, preview, and command submission;
* presentation-only selection notifications for step 14.

It does not own `BattleSession`, roster data, occupancy, command queues, or a second phase
enum.

Every public intent method returns this exact small value:

```cpp
enum class InputHandlingDisposition
{
    Ignored,
    Handled,
    Submitted
};

struct InputHandling
{
    InputHandlingDisposition disposition = InputHandlingDisposition::Ignored;
    std::optional<CommandResult> submission;
};

void syncFromSnapshot(const BattleSnapshot&);
void hover(std::optional<BoardCell>);
InputHandling selectPlacementCreature(CreatureInstanceId);
InputHandling clickBoardCell(BoardCell);
InputHandling selectMove();
InputHandling selectAbility(std::size_t visibleSlot);
InputHandling cancel();
InputHandling confirmDeployment();
InputHandling endTurn();
```

`submission` is present if and only if the disposition is `Submitted`, and contains the
exact accepted/rejected/aborted value returned by `BattleSession::submit`. `Handled` means
only transient interaction state changed (selection, cancellation, or feedback) without an
authoritative command. `Ignored` changes neither interaction nor simulation state. This
single return contract lets mouse, keyboard, and step-14 buttons share behavior without a
second `SubmissionResult` hierarchy.

Step 14 must call these same methods from buttons. Keyboard handlers call them too. Only
this controller translates an intent into a command.

## 9.1 Synchronization rules

On each committed gameplay event batch or phase notification:

1. obtain a fresh snapshot;
2. discard selections whose unit/ability/phase no longer exists;
3. enter `InteractionBusy` while a submitted command batch or queued AI action is resolving;
4. select placement state while player placement is open;
5. use `AwaitingAction` only when a living player-controlled unit is active;
6. use `InteractionInactive` for enemy activation, terminal state, or pending mode exit;
7. rebuild the overlay model only after this synchronization.

Never retain a pointer/reference into a prior snapshot. Stable IDs are safe.

## 9.2 Re-entrancy

Command submission may synchronously produce events. Guard the controller:

* mark itself busy before calling the gateway;
* do not process another click/key while that call is active;
* queue or coalesce snapshot refresh notifications received during the call;
* after return, perform one refresh from authoritative state;
* show rejection feedback from the returned typed reason;
* do not assume a successful command leaves the same unit active.

This prevents double-clicks from spending twice and callbacks from recursively submitting.

---

# 10. Manual deployment flow

Manual placement is a production rule, not a debug convenience.

The session begins in the placement phase established by step 06. The interaction controller
must support:

1. Choose one unplaced player-team `CreatureInstanceId`.
2. Ask the placement preview for legal cells for that creature/side.
3. Hover/click a legal support cell.
4. Submit the canonical `PlaceUnitCommand` through the common gateway.
5. If the same creature is already placed, submit another `PlaceUnitCommand`; step 06
   defines it as the canonical placement/reposition operation and keeps occupancy atomic.
6. Select another creature and repeat.
7. Confirm only when the headless placement validator says the required team is legal.
8. Submit `ConfirmDeploymentCommand`. The scheduler, not the UI, starts the battle.

For this step, number keys 1..6 select roster slots and Enter confirms. A later HUD card is
only another caller. Clicking a cell without a selected creature is ignored and does not
place the first creature implicitly.

Deployment overlays come from the validator for the selected creature. Occupied cells,
team-zone restrictions, failure to place exactly every non-empty roster member, duplicate
creature IDs, and unavailable creatures are all rejected by headless rules. Presentation
may display a short invalid mask but must not invent a reason.

The optional step-12 debug autoplay remains available, but it submits the same placement
commands. It must be disabled by default for a normal player battle once this step lands.

---

# 11. Activation controls

Use a compact, documented control scheme that does not conflict with the current French
AZERTY-friendly exploration camera bindings:

```text
M                 enter/cancel movement targeting
1..8              choose visible ability slot
left click         confirm hovered board cell for the selected action
Escape/right click cancel current targeting mode
Space              submit EndTurn
right drag         orbit camera
mouse wheel        zoom camera
```

Do not use E/Enter for end turn; Enter remains placement confirmation and the current
exploration camera already uses E. Key symbols, not text characters, should be mapped
consistently with the existing Sparkle input API.

Right button has two gestures. Record its press position; crossing a small pixel threshold
(for example 3 px) starts camera drag and suppresses cancel, while release below the
threshold performs one cancel. Do not cancel targeting merely because the player began to
orbit. Clear this gesture state on mouse leave, mode detach, or focus loss.

## 11.1 Movement

When `selectMove()` is invoked:

* require a player-controlled active unit in an action-accepting phase;
* query reachable cells from the canonical movement preview;
* show the selected origin and reachable cells;
* hover requests the canonical path/cost preview;
* click constructs/submits the exact `MoveCommand` returned by preview;
* clicking origin or Escape cancels without a command;
* rejected/stale commands retain or cancel selection according to a single documented
  policy; prefer retaining it if the active unit is unchanged.

Do not animate or move a renderer here. Step 14 consumes committed movement events.

## 11.2 Ability

When `selectAbility(slot)` is invoked:

* map the visible slot through the active unit's derived ordered ability list;
* require that the ability is currently usable according to the validator;
* show legal aim cells;
* hovering an aim cell asks the canonical target/AoE preview;
* show affected support cells without claiming that random damage has already occurred;
* click submits `CastAbilityCommand{unit, abilityId, anchor}` from the preview;
* never choose an entity by ray-cast pointer;
* Escape/right click returns to `AwaitingAction`.

Every cast still carries one anchor. When ability selection produces exactly one legal
anchor and it is the active source cell (the v1 self/no-target case), submit
`CastAbilityCommand{unit, abilityId, sourceCell}` immediately through the guarded gateway;
do not enter `SelectingTarget` or add a confirmation dialog. Zero legal anchors leaves
`AwaitingAction` and shows the canonical rejection/unavailable reason; multiple legal
anchors enters `SelectingTarget`. Because immediate submission is the user's committed
button action, Escape/right-click has nothing to cancel afterward. A stale snapshot is
handled by ordinary gateway revalidation and one authoritative refresh.

## 11.3 End turn

`Space` calls `endTurn()`. It submits
`EndTurnCommand{activeUnit, EndTurnRequestCause::Explicit}` with
`CommandIssuer{CommandController::Player}` through the gateway; UI cannot select an AI
safety cause.
The controller does not advance the scheduler directly. It is ignored during placement,
enemy control, busy resolution, or terminal state. If rules allow ending with remaining
resources, no UI confirmation is required for this prototype.

---

# 12. Event propagation and mode gating

The preferred integration is a dedicated active child widget for battle input, because
Sparkle routes active children before `GameSceneWidget`. A component-logic integration is
also acceptable if it obeys the same ordering and consumption rules.

For every event:

* first check the authoritative `ControlContext`/`GameModeKind` from step 12;
* check that a bound battle runtime exists and no transition is pending;
* allow step-14 child buttons to consume pointer events before board picking;
* consume a handled battle click/key so exploration and debug bindings cannot also react;
* do not consume an ignored event outside battle;
* always clear hover on mouse-leave, battle detach, and geometry loss;
* release any captured/focused mouse state during detach.

The battle input widget must not hold a raw reference to `BattleModeRuntime` across the
post-update transition boundary. Bind it with an explicit nullable handle/callback facade
that `beginExit()` clears before destruction.

`GameSceneWidget::_onUpdate` must preserve the step-12 sequence:

```text
normal engine/world update
    -> resolve deferred exploration interaction
    -> apply at most one already queued mode transition at the safe boundary
    -> update/pump the now-active battle
    -> synchronize committed event batches for presentation
    -> leave any transition requested by that pump queued for the next frame boundary
```

No input callback may call `ModeManager::commitPending()` or destroy the battle.

---

# 13. Diagnostics

Extend the existing F7 debug overlay with compact battle-only rows:

* mode and committed/pending transition;
* battle seed/encounter ID;
* phase and active unit ID;
* interaction-state name;
* board source plus hovered `BoardCell` and mapped presentation support cell;
* overlay cell count/revision;
* last submitted command kind and rejection code;
* camera target/yaw/pitch/distance;
* fluid state and exploration/battle streamer loaded-chunk counts.

Diagnostics must be read-only. Do not refresh a mesh or submit a preview merely because F7
is visible.

---

# 14. Expected files

Adapt exact locations to the post-step-12 tree, but expect the equivalent of:

```text
Playground/srcs/
├── battle/presentation/
│   ├── battle_board_picker.hpp/.cpp
│   ├── battle_presentation_board_binding.hpp # reuse step-12 value/view
│   ├── battle_presentation_cell_source.hpp/.cpp # extend/reuse step-12 binding
│   ├── battle_interaction_controller.hpp/.cpp
│   ├── battle_overlay_model.hpp/.cpp
│   ├── battle_overlay_presenter.hpp/.cpp
│   ├── battle_presentation_runtime.hpp/.cpp
│   └── tactical_camera_controller.hpp/.cpp
├── rendering/
│   └── walk_surface_mask_mesh_builder.hpp/.cpp
├── logics/
│   └── exploration_input_logic.cpp        # now reuses extracted builder
└── game_scene_widget.hpp/.cpp              # lifecycle/input binding only

Playground/tests/
├── battle_board_picker_tests.cpp
├── battle_interaction_controller_tests.cpp
├── battle_overlay_model_tests.cpp
├── tactical_camera_controller_tests.cpp
└── walk_surface_mask_mesh_builder_tests.cpp
```

Register sources/tests in the existing CMake lists. Do not create a second executable for
the tactical UI.

---

# 15. Automated verification

Add tests with small deterministic fixtures.

## 15.1 Mesh extraction

Prove:

* a cube emits only its top;
* a rotated/flipped slope emits its oriented upward surface;
* stairs emit all upward treads and no risers;
* empty/unknown support emits no geometry;
* multiple cells share one baked mesh with correct per-cell atlas UV bounds;
* atlas-cell bounds and lift are correct;
* live-world and handcrafted presentation-source adapters produce byte/equivalent geometry
  for identical voxel cells/shapes without a `VoxelWorld` downcast;
* exploration's old one-cell hover output is equivalent in vertex/index content after
  extraction.

These tests may inspect baked mesh data; they must not require pixel screenshots.

## 15.2 Overlay model

Prove:

* cells are canonical and duplicate-free;
* priority resolves overlapping categories exactly once;
* all eight enum values map to the exact step-01 atlas keys;
* invalid outranks hover, LoS-blocked outranks AoE, path outranks ordinary ability range,
  and selected/current-hover cells intentionally use the same `Hovered` kind;
* revisions stay stable when effective output is unchanged;
* preview changes increment once;
* a board-to-presentation mapping, not raw coordinate equality, drives instances for both
  a translated live board and a zero-origin handcrafted arena.

## 15.3 Picker

Build a multi-level fixture and prove:

* nearest whitelisted board surface wins;
* a solid non-board voxel in front occludes the board and yields no pick;
* a world-navigation node outside the board is never returned;
* no downward search changes the hit's level;
* holes and out-of-footprint hits return no value;
* resize-derived ray input uses the current viewport.

Run the same picker cases once through a live `VoxelWorld` adapter and once through an owning
handcrafted arena adapter. Add a live obstacle and an authored-prefab obstacle in front of a
legal cell and prove each nearest surface occludes identically. Prove a production
handcrafted board is pickable, its returned field is `presentationSupportCell`, no world
coordinate conversion is called, and a headless fixture with no presentation binding fails
cleanly at attach rather than dereferencing a `VoxelWorld`.

## 15.4 Interaction state machine

Prove:

* placement requires explicit selection and confirmation;
* placement/reposition/confirm all traverse the same fake gateway;
* no controller method edits fake authoritative state directly;
* movement and ability overlays are exactly the fake reachable/path/range/LoS/AoE preview
  sets;
* valid clicks submit the preview command;
* rejection produces feedback without mutation;
* re-entrant event notification cannot double-submit;
* enemy/busy/terminal/pending-exit states ignore actions;
* keyboard and button-facing methods produce identical gateway calls;
* a phase/active-unit change clears stale selection.
* right click cancels once while right drag only orbits.

## 15.5 Preview purity

For deployment, movement, and at least one AoE ability:

* capture state, event-log, next-ID, and RNG digests;
* call preview repeatedly and in different hover orders;
* prove every digest is unchanged;
* prove sorted output is identical;
* submit a returned command and prove normal validation accepts it.

## 15.6 Camera/lifecycle

Prove camera framing math for single-cell, rectangular, elevated, and sloped presentation
AABBs, including one far-translated live board and one zero-origin handcrafted board. Run an
integration fixture through both source variants:

```text
Exploration -> Battle -> Exploration -> Battle -> Exploration
```

and assert:

* one active input owner and camera controller;
* no duplicate contract or overlay entity;
* fluid paused only in battle;
* live world/player/battle streaming and handcrafted arena visibility remain exactly as
  step 12 specifies;
* detach leaves no snapshot callback pointing to the destroyed session.

Use the repository's corrected test preset from step 01 and run the full Playground suite,
not only the new test binary.

---

# 16. Visual verification

Run the actual Playground executable and record the checks in the handoff:

1. Walk, hover a cube/slope/stair, and verify exploration hover did not regress.
2. Trigger F8 and observe a smooth tactical camera blend without unloading the world.
3. Orbit and zoom while terrain, daylight, and streaming remain live.
4. Manually select and place each player creature; verify illegal cells flash invalid and
   legal deployment cells match accepted clicks.
5. Confirm placement explicitly.
6. Select movement and inspect path/reachable masks at multiple heights.
7. Select a ranged/AoE ability and inspect legal aim and affected-cell masks.
8. Cancel, reselect, use keyboard shortcuts, and end a turn.
9. Resize to at least 800x600 and a wide window; picking must remain under the cursor.
10. Complete/leave the battle and verify exploration camera, hover, movement, fluid, and
    portals resume.
11. Repeat the battle once and check F7 for stable entity/contract counts.
12. Launch the authored handcrafted fixture: verify the same camera, overlays, picking, and
    command controls operate on the isolated arena, generated-world chunks stay hidden, and
    exit restores the exploration view without a camera/scene swap.

If prerequisite content does not yet include a meaningful AoE, use a test/debug definition
loaded through the normal registry. Do not hard-code an effect into presentation.

---

# 17. Documentation

Update:

* `Playground/docs/03-systems/ui-hud.md` with the implemented input flow, controls, overlay
  priority, and explicit rule/presentation boundary;
* the battle/mode system page created by step 12 with camera, picker, live-source, and
  handcrafted-arena lifecycle;
* `Playground/README.md` with the playable battle controls;
* CMake/test documentation if target names changed.

Include a small ownership/interaction diagram and state that `Actor.cell` and every battle
overlay coordinate identify the solid support voxel. Also state that a Bush trigger is
tested at `Actor.cell + (0, 1, 0)`; do not let future UI work reinterpret the coordinate
contract.

---

# 18. Non-goals

Do not implement:

* creature models, animation rigs, polished VFX, or a final HUD;
* a content-authoring/editor tool;
* a separate battle scene/camera or a copy of exploration terrain (the step-12 isolated
  authored-arena renderer is required and reused);
* free camera effects that influence rules;
* controller/gamepad/touch input;
* fog of war or hidden information;
* path spline animation;
* damage prediction that rolls RNG during hover;
* direct session mutation for convenient placement;
* fluid resumption while a battle is merely waiting for input;
* changes to taming, Feat progress, persistence, or battle results.

Keep extension seams, but make the requested mouse/keyboard loop complete.

---

# 19. Acceptance criteria

This step is complete only when:

* the private exploration hover builder has been extracted and both exploration and battle
  use it;
* cube, slope, stair, atlas, priority, picker, preview-purity, interaction, and lifecycle
  tests pass;
* the tactical camera exclusively controls the persistent camera during battle and returns
  smoothly to exploration;
* picking is restricted to exact frozen board support cells through a source-agnostic
  presentation cell adapter and works for live and handcrafted sources;
* overlays are generated from canonical previews and rebuild only on model revision;
* manual deployment and confirmation work through the common command gateway;
* movement, ability choice, cancel, click confirmation, and end-turn work through that same
  gateway;
* AI/debug autoplay/tests have no alternate execution route;
* event propagation prevents one input from reaching both battle and exploration;
* mode transitions remain queued post-update, fluid stays paused, and each source preserves
  its step-12 streaming/render-visibility invariants;
* teardown is idempotent and a second battle has no stale state;
* full builds/tests pass and the visual checklist has been performed;
* documentation describes the implemented controls and coordinate conventions.

---

# 20. Handoff to step 14

In the completion report, give step 14:

* exact `BattlePresentationRuntime` ownership and detach order;
* the stable interaction-controller methods HUD buttons must call;
* how snapshot/batch changes are observed and coalesced;
* the `BattleOverlayModel` and preview types;
* board-cell-to-presentation-surface mapping, presentation-cell source, and AABB contracts;
* the final keyboard controls and mask atlas mapping;
* whether a self-target ability submits immediately;
* test commands and any remaining visual limitations.

Step 14 must add views and HUD as consumers of these seams. It must not move selection
state into widgets or introduce a second command path.
