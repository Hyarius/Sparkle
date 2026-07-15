# System — UI / HUD

Step 14 ships the prototype battle HUD as a persistent `BattleHudWidget` child of
`GameSceneWidget`. It is inactive during exploration, binds after battle presentation attaches,
and unbinds before the session and board are released.

```text
BattleSnapshot + immutable Registries + interaction state
                    -> BattleHudViewModelBuilder
                    -> BattleHudWidget
                    -> BattleInteractionController
                    -> BattleSession::submit
```

The view-model builder owns all strings and display values. Widgets never retain a
`BattleSession*`, never change HP/AP/MP/occupancy, and refresh only at attach, interaction-state,
or committed-batch boundaries. The scheduler forecast is a copied turn-bar projection: it does
not mutate time, the scheduler, the log, or battle RNG.

## Prototype layout and controls

The supported minimum is **800×600**. The player and enemy six-card columns stay at the screen
edges; a turn forecast stays adjacent to the player column; the resource/ability panel occupies
the bottom centre so the board remains visible.

- Player cards select a persistent `CreatureInstanceId` during deployment. They do not place a
  unit; the normal board click submits the `PlaceUnitCommand`.
- Ready and Enter both call `BattleInteractionController::confirmDeployment()`.
- Ability buttons and keys 1–8 both call `selectAbility(slot)`.
- End Turn and Space both call `endTurn()`.
- A guarded `AbilitySlotWidget` stores the enabled state and ability id. Its callback cannot
  forward a click while disabled even though `spk::PushButton` has no disabled API.

`TeamRosterWidget<TCard, Capacity>` is the reusable six-slot vertical layout. It owns ordering,
selection, card construction, copied card models, and geometry reflow; its `TeamRosterCard`
contract leaves a supplied card type responsible for its own visual and interaction behaviour.
The current `CreatureCardWidget` is one such implementation, so the battle roster can later use
a different creature-card frontend without duplicating roster code.

Each `CreatureFormDefinition` now requires an authored `icon: [x, y]` atlas coordinate alongside
its name, tier, and placeholder visual. The HUD copies that coordinate to `CreatureCardViewModel`,
so a roster card displays the current form's icon and name together and automatically changes when
the creature evolves.

Left-clicking an occupied card selects it and expands the selected card to show HP, AP, MP,
attack, armour, magic, and resistance. The roster releases its previous minimum size and asks its
parent to reflow whenever that expansion changes its preferred height. Right-clicking a card opens
a draggable detail window using only copied `CreatureDetailsViewModel` data: resources, initiative
cost, range, combat attributes, abilities, passive effects, and active effects. This remains a
presentation boundary: it never retains a live battle-unit pointer.

Cards show current-form tint, HP, placement/defeat/selection state, shield and ready metadata.
The bottom action bar projects the selected placement creature during deployment and the active
creature during an activation. It lays out rows of up to eight status-effect icons first, then AP,
health, and MP, followed by eight numbered ability shortcuts. Left-clicking a populated shortcut
selects that ability through `BattleInteractionController`; right-clicking it opens a copied spell
detail window. Right-clicking an effect icon likewise opens its copied status detail window. The
single bottom contextual button is `Ready` in deployment and `End Turn` for a player activation.

`ActionBarViewModel`, `AbilitySlotViewModel`, and `StatusViewModel` include the display data used
by those windows (resource values, descriptions, icon coordinates, range/cost, stacks, duration,
and effect ids), so frontend code has no dependency on a live `BattleUnit` or registry query.
Battle HUD text is white, and the top-centre battle-status text has an opaque backing panel for
contrast. `ResourceBarWidget` is read-only and clamps its visual fill; a zero maximum is safe.

JSON placeholder tint and scale are presentation-only. Scale changes neither a creature's tactical
footprint nor range, occupancy, or line of sight. Rich cards, final art, ability paging, result
screens, taming, and exploration HUD remain deferred.
