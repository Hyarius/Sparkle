# Glossary

Canonical spelling and meaning for every game and engine term. When writing code, JSON, or
docs, use these exact names. Unity-reference names that differ are noted as *(Unity: X)*.

## Game terms

| Term | Meaning |
|------|---------|
| **Ability** | An action a creature can cast in battle: AP/MP cost, range, AoE, target profile, list of Effects, plus presentation: casterAnimation, targetAnimation (falls back to TakeDamage), travelVfx (projectile/beam, D36). *(Mockups call the UI for it "Action UI".)* |
| **AP — Ability Points** | Per-turn resource spent to cast abilities. Refills to max at turn start. *(Mockups: "PA".)* |
| **MP — Movement Points** | Per-turn resource spent to move (1 cell = 1 MP by default; terrain can cost more). Separate from AP. *(Mockups: "PM".)* |
| **Stamina** | Authored stat: seconds required to fill the Turn Bar. Lower = acts more often. *(Unity: `Recovery`.)* See decision D19. |
| **StaminaRate** | Runtime multiplier on turn-bar fill speed (default 1.0), moved by effects. *(Unity: `Stamina`/`StaminaRatio`.)* |
| **Turn Bar** | Per-unit gauge that fills over battle time; full = the unit takes a turn. Stun pauses fill. |
| **Effect** | Polymorphic building block of abilities/statuses: damage, heal, shield, status apply/remove, cleanse, revive, displacement, teleport, resource change/steal, turn-bar adjust, place/remove object. |
| **Status** | A named, stackable condition on a unit (poison, stun…): tags, one hook point, effects executed when the hook fires, a Duration. |
| **Status hook point** | When a status fires: `TurnStart, TurnEnd, BeforeConsumingResources, AfterConsumingResources, OnHPLoss, OnAPLoss, OnMPLoss, OnHPGain, OnAPGain, OnMPGain`. |
| **Passive** | A permanent status a creature always carries (granted by feat rewards). |
| **Shield** | Absorbs incoming damage of its kind (Physical/Magical) before HP; has an amount and turn duration (-1 = infinite). |
| **Duration** | `turnBased(n)` / `seconds(s)` / `infinite` — lifetime of statuses, shields, board objects. |
| **CreatureSpecies** | Authored species definition: base Attributes, default abilities, Forms, FeatBoard, TamingProfile. |
| **CreatureForm** | One visual/tier variant of a species (evolution stage): display name, tier, model. |
| **CreatureUnit** | One owned creature instance: species ref, current form, accumulated Attributes, ability pool, permanent passives, FeatBoardProgress. |
| **BattleUnit** | A creature's in-battle runtime wrapper: BattleAttributes (observable resources), statuses, board position, side. |
| **WildBattleUnit** | BattleUnit subclass for wild creatures carrying TamingProgress. |
| **Attributes** | The stat block: health, ap, mp, attack, armor, armorPenetration, magic, resistance, resistancePenetration, bonusRange, stamina, staminaRate, bonusHealing, lifeSteal, omnivampirism, timeEffectResistance. |
| **Feat** | A node's worth of progression earned by *doing things* in battle. Never "Quest" (D07). |
| **FeatBoard** | Per-species node graph (FF12/PoE style). Root node starts completed; completing a node activates its neighbours. |
| **FeatNode** | One board node: id, kind (`StatsBonus, Ability, Passive, Form`), neighbour ids, requirements (all must complete), rewards, repeat allowance. |
| **BattleCondition** | The shared evaluation core for in-battle conditions: a Scope window + RequiredRepeatCount; concrete types score typed events 0–100% per window (D33). |
| **FeatRequirement** | A BattleCondition parsed via the feat factory — the feat-board authoring catalog. Each entry on a node carries a UUID (D34). |
| **TamingCondition** | A BattleCondition parsed via the taming factory — the taming-profile catalog; may diverge from the feat catalog freely (D33). |
| **Scope (feat)** | Window a requirement must fit in: `Ability` (one ability resolution), `Turn`, `Fight`, `Game` (persists across fights). See D21. |
| **FeatReward** | What a completed node grants: `BonusStats`, `Ability`, `RemoveAbility`, `Passive`, `ChangeForm`. |
| **FeatProgress / FeatBoardProgress** | Per-creature persisted advancement per node/requirement. |
| **BattleEvent** | Typed record of something that happened in battle (DamageEvent, HealEvent, AbilityCastEvent, UnitDefeatedEvent…). The combat log; feats and taming both consume it. |
| **Taming / TamingProfile / TamingProgress** | Wild-species list of TamingConditions tracked live during a wild fight; all complete ⇒ the creature is **impressed**, leaves the board, joins the team on a won battle (inheriting the spawn's completed nodes, D35). Defeating it first ⇒ taming failed. No capture items (D24). |
| **EncounterTable / EncounterTier** | Per-biome table of weighted enemy team compositions, tiered by badge count (`NoBadge` … `EightBadges`, `PostGame`). |
| **EncounterUnit** | CreatureUnit + AIBehaviour, as authored inside encounter teams. |
| **Encounter trigger** | Bush (tagged cross-plane voxel), interior cell, trainer line-of-sight, or scripted gym/boss. |
| **Board** | The tactical battle grid derived (live) from world voxels, or handcrafted for special battles. |
| **BoardData** | Headless seam: terrain layer + navigation layer + runtime registry. Battle logic only sees this. |
| **Deployment zone** | Cells where a side may place units during Placement (two opposing strips by default, D13). |
| **LoS — Line of Sight** | Voxel raycast between cell centers; obstacles may block per their definition; elevation does not change range (GDD §6). |
| **Mode** | A control-state over the one persistent world: `ExplorationMode`, `BattleMode`, later `MainMenuMode`. Swaps input handling + HUD + camera behavior + overlay; never reloads the world (D03). |
| **Badge / unlock tier** | Count of defeated gyms; drives encounter tier scaling and progression gating (never levels). |
| **Heal point** | Respawn location after a lost battle (Pokémon-style). |
| **PC storage** | Overflow storage beyond the 6-slot active team. |
| **Biome** | Region type anchored to a major city; owns encounter rules and voxel palette. |
| **Macro world plan** | Seed-generated once per run: landmass, heights/rivers, 8 cities, biome regions, settlements, transport graph. Chunks query it during realization. |
| **Structure** | Reusable authored voxel prefab (dungeon room, pond, building) stamped into the world. |
| **Impressed** | State of a wild creature whose taming conditions are all fulfilled; it leaves the fight immediately. |

## Voxel & world terms

| Term | Meaning |
|------|---------|
| **VoxelDefinition** | Authored voxel: `VoxelData` (traversal + tags) + polymorphic `VoxelShape`. |
| **VoxelShape** | Builds render FaceSet, collision FaceSet, MaskSet, and per-flip CardinalHeightSets. Concrete: `Cube, Slab, Slope, Stair, CrossPlane`. |
| **FaceSet** | `innerFaces` (always drawn) + `outerShell` (6 axis-plane faces occludable by neighbours). |
| **MaskSet / mask faces** | Per-shape decal faces describing how an overlay drapes over the voxel's walkable surface (flat on cubes, slanted on slopes). **The battle overlay renders through them** (D31), textured from the mask sprite sheet; also used for world decals (paths). Stacked layers offset by 0.001·layer in Y. |
| **Mask sprite sheet** | `resources/textures/mask.png` — atlas of overlay looks; battle categories map to cells via `game-rules.json` `overlayMasks`. |
| **CardinalHeightSet** | Walk heights at a cell's PosX/NegX/PosZ/NegZ edges + center (`stationary`); drives traversal across slopes/stairs. |
| **Traversal: Solid / Passable** | `Solid` = ground you stand on; `Passable` = pass-through (bush). *(Unity: `Obstacle`/`Walkable` — inverted-sounding; renamed, D20.)* |
| **VoxelCell** | Placed instance: definition id + orientation (4 Y-rotations) + flip (Y up/down). Empty when id < 0. |
| **VoxelGrid** | Dense 3D array of VoxelCells. |
| **VoxelMesher** | Builds render/collision/mask meshes from a grid with neighbour-occlusion culling. |
| **Chunk** | 16×16×16 voxel unit of streaming/generation (`pg::Chunk::Size`, compile-time, D29). A `spk::Component` + `spk::SynchronizableTrait`: `setCell` requests synchronization; the baker logic rebuilds its render mesh / mask mesh / nav slice (D32). |
| **VoxelWorld** | Chunk container + streaming around the player + cell access across chunk borders; fans out border-edit synchronization to neighbour chunks. |
| **Prefab** | Reusable pure-voxel structure (`prefabs/*.json`): palette/fills/cells + named **anchors**; stamped into maps or generated chunks. No gameplay context. |
| **Portal** | Named cell in a map linking to `{targetMap, targetPortal}` — entering places you at the target portal (D37). How interiors connect; multi-exit by construction. |
| **Standable cell** | `Solid` voxel with 2 cells of head clearance above (D20). |
| **Vertical traversal gap** | Max walkable height difference between adjacent cell edges: **0.5** world units. |
| **Voxel raycast (DDA)** | Grid-walking ray used for LoS, mouse picking, trainer sight. |

## Engine terms (Sparkle / pg::)

| Term | Meaning |
|------|---------|
| **`spk::Entity`** | Hierarchy node owning heap-allocated `spk::Component`s (`addComponent<T>`, `component<T>`). |
| **`spk::Component`** | Data-only, attached to one entity; activable. |
| **`spk::ComponentLogic<T>`** | System processing all components of type T: `_onUpdateStarted/_parseComponentForUpdate/_executeUpdate`, `_onRenderStarted/_parseComponentForRender/_executeRender(builder)`, plus per-event-type hooks (mouse, key…). Registered on the engine with `engine.add<TLogic>()`. |
| **`spk::GameEngine`** | Owns the component registry + logic registry; `addEntity`; updates/renders/dispatches events to logics. |
| **`spk::GameEngineWidget`** | Widget hosting a GameEngine; forwards widget events into it; renders its RenderUnit. |
| **RenderCommand / RenderUnit / RenderUnitBuilder** | A frame = an ordered list of `spk::RenderCommand`s built by logics (`builder.emplace<T>(...)`) and executed against the GL context. |
| **`spk::CameraUpdateRenderCommand`** | Uploads a view-projection matrix to a UBO binding point, in-stream (multi-camera ready, D04). |
| **`spk::Transform3D` / `Entity3D` / `Camera3D` / `TextureMeshRenderer3D` / `TextureMeshRenderLogic` / `DrawTextureMesh3DRenderCommand`** | Sparkle's promoted textured 3D layer (Step 13). See [03-systems/rendering-cameras.md](03-systems/rendering-cameras.md). |
| **`spk::ContractProvider<Args…>`** | Signal/subscription primitive: `subscribe(cb) -> Contract` (RAII unsubscription), `trigger(args…)`, blocking, deferred trigger. Foundation of the event bus. |
| **`spk::ObservableValue<T>`** | ContractProvider-backed value that notifies on change. Already in `spk::`. |
| **`pg::ObservableResource`** | current/max int pair with change notification (to build; mirrors Unity). |
| **`pg::EventCenter`** | Typed pub-sub bus for cross-system events (to build over ContractProvider). |
| **Definition registry** | `pg::Registry<T>`: id → immutable definition, loaded from `resources/data/<domain>/`. |
| **`PG_RESOURCE_DIR`** | Compile definition giving the absolute path of `Playground/resources`. |
| **PlaygroundCore / SparklePlayground / PlaygroundTests** | Static lib with all `pg::` code / thin game exe / gtest exe (D17). |
| **EreliaTools** | The single tool-suite executable with editor tabs (D15). |
| **Promotion** | Lifting a proven `pg::` feature into `spk::`, as its own signed-off step (D18). |
| **Fake animation** | Rig of logical parts + recipes of phases/steps applying transform offsets; no skeletal rig. See [03-systems/animation.md](03-systems/animation.md). |
| **LogicalPart** | Animation target slot (`root, body, head, frontLimb, rearLimb, dominantLimb, offLimb, weapon, jaw, tail, wholeRig`); rigs map parts → entities; unmapped parts are skipped. |
| **Recipe / Phase / Step (animation)** | Recipe = named sequence of Phases; Phases run sequentially; Steps inside a phase run in parallel (`MoveLocal, RotateLocal, Scale, Shake, Flash, Wait, SpawnVfx, PlaySound`). |

## Naming rules

- C++ types `PascalCase` in namespace `pg::`; members/methods `camelCase`; parameters
  `p_camelCase`; private fields `_camelCase` (Sparkle house style).
- JSON keys `camelCase`; enum values lower-camel strings (`"physical"`, `"turnStart"`).
- Definition ids: `kebab-case` filename stems (`grass-block.json` → id `grass-block`).
- Files: `snake_case.hpp/.cpp` mirroring Sparkle (`voxel_definition.hpp`).
- Never: "Quest" (use Feat, D07), "Recovery" (use stamina, D19), "Obstacle/Walkable"
  traversal (use solid/passable, D20), "capture" (use taming, D24).
