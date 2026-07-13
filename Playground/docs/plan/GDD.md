# Game Design Document (GDD)

Title: Erelia (working title)
Genre: Creature-collection RPG with stamina-turn combat and procedurally generated voxel world
Target: Single-player, offline

---

## 1) High-Level Concept
Erelia is a creature-collection RPG inspired by classic Pokemon, but with a different battle system and progression model. The world is procedurally generated each playthrough: towns, gyms, routes, and encounter tables shift with the seed. Creatures do not gain levels; instead, they earn **Feat Progress** through battle Abilitys, which unlocks abilities, stat bonuses, and evolutions on a **Feat Board**-style progression screen (FF12/PoE inspiration).

---

## 2) Core Pillars
1. **Familiar loop, new progression**: Explore ? encounter ? battle ? progress. Progress is Ability-based unlocks, not XP levels.
2. **Stamina-turn combat**: Faster creatures act more often, but all Abilitys happen in discrete turns.
3. **Procedural world**: Each run has a different layout, gym order, and progression path.
4. **Clear tactical placement**: Battles take place on a board derived from the world or predefined arenas.

---

## 3) Player Goals
Primary goals:
- Defeat 8 gyms in a randomized order.
- Defeat the Elite Four (endgame).

Secondary goals:
- Collect and evolve creatures.
- Optimize Feat Boards for different builds.
- Find rare encounters in specific biomes.

---

## 4) Game Loop
**Main loop**
1. Explore the procedurally generated world.
2. Trigger encounters (wild or trainer).
3. Enter battle on a board (world-derived or predefined).
4. Win/Lose battle ? creatures earn Feat Progress.
5. Unlock new Abilitys, bonuses, evolutions.
6. Continue exploring and progress through gyms.

---

## 5) Exploration & World
### World Structure
- World is voxel-based and chunk-generated.
- Terrain is generated with noise (Perlin or similar).
- Towns and gyms are placed procedurally and composed of voxels too. You can enter buildings that teleport you to an interior space.
- The world includes **structures** built from reusable cells of elements (ex: dungeon rooms, ponds with scenery).
- Biomes define encounter tables.
- Towns/cities/POIs are generated first, then connected via roads using a Voronoi-style diagram; pathing may be upgraded to account for height differences.

### Creature Abilities in the World
- The player can use creature abilities to interact with the world (ex: cut, rock smash).
- Structures can include ability gates that unlock items, secret areas, or rare encounter spots not found in normal chunks.

### Encounters
- **Wild encounters**: Triggered in bushes in the overworld, based on biome-specific encounter tables.
- **Interior encounters**: In caves or similar interiors, encounters can trigger anywhere.
- **Trainer encounters**: Triggered when the player walks into a trainer's line of sight.
- **Special battles**: Gyms, bosses, story fights use predefined boards and are scripted events (do not use world-seed variation).
- **Rarity**: Each biome uses a fixed encounter table (no time/weather). Some creatures appear only in specific bushes tied to specific points of interest.

---

## 6) Battle System
### Turn Logic (Stamina)
- Each creature has a **Stamina** value that determines how quickly its turn comes.
- The battle progresses through time until a creature is ready to act.
- When a creature reaches its turn time, the game pauses and accepts a command.
- At the start of its turn, the creature's **Ability Points (AP)** and **Movement Points (MP)** are restored to their maximum values.
- Stamina is likely a "seconds to fill the **Turn Bar**" value (lower is better). Name may change if it does not fit.
- Some Abilitys can reduce a target's AP/MP, but turns generally start at full pools.
- **Turn Bar UI**: displayed on the left side, showing each creature's Turn Bar progression.
- **Stun**: pauses Turn Bar fill; when stun ends, the bar resumes filling from its current value.

### Battle Turn Flow (PlantUML)
```plantuml
@startuml
skinparam packageStyle rectangle

actor Player
actor AI

class BattleLoop
class TurnService
class BattleState
class AbilityResolver

BattleLoop -> TurnService : AdvanceTime()
TurnService -> BattleState : SelectReadyUnit()
BattleLoop -> BattleState : PauseAtReadyUnit()
BattleLoop -> Player : RequestAbility (if player unit)
BattleLoop -> AI : RequestAbility (if enemy unit)
Player --> BattleLoop : ChosenAbility
AI --> BattleLoop : ChosenAbility
BattleLoop -> AbilityResolver : ResolveAbility()
AbilityResolver -> BattleState : ApplyEffects (damage, buffs, movement)
BattleLoop -> TurnService : ScheduleNextTurn()
@enduml
```


### Victory Condition
- Battle ends when all creatures on one side are defeated.

### Board
- Most battles use a board derived from the world around the player.
- Special battles can use handcrafted boards.
- Encounter data defines board size. Battles occur where the player is standing for immersion.

### Placement
- Player places creatures manually at the start.
- Enemies are placed using placement policies (random/fixed, by line).
- Placement uses side-based deployment zones; the game chooses a side for each team and creates placement nodes.

### Abilitys
- Each Ability costs **AP**, has a range, and may require line of sight (or ignore it).
- Targeting can be straight-line, free-form, or area-of-effect.
- Abilitys can be: direct damage, damage-over-time, heal-over-time, traps placed on cells, buffs, debuffs, cleanse, shields, armor/resistance boosts.
- Abilitys can be **physical** or **magical**, with corresponding mitigation stats.

### Movement
- Movement costs **MP** per cell (default 1 cell = 1 MP).
- Terrain can modify MP cost (ex: mud costs 2 MP per cell).
- Movement and Abilitys are separate; you can move and act in the same turn.
- Obstacles can block line of sight depending on type; elevation does not affect spell range.

---

## 7) Creatures & Progression
### No Levels
- All creatures start with the same baseline stats and attacks.
- Progression is earned through **Feat Progress**.

### Feat Progress
- Creatures gain Feat Progress by fulfilling conditions in battle.
- Feat Progress unlocks nodes on a Feat Board.
- Feat Progress is per-creature via a dedicated **Feat Board** UI/class, rendered on a creature-specific HUD.
- Feats are **not all active at once**. The center node starts completed, and all adjacent feats become active. Completing a feat activates its adjacent feats.
- Progress is permanent and passive; feats are not failed or abandoned.
- Each species has its own unique Feat Board. Some feat types can be shared across species (generic nodes), but layouts are species-specific.
- Time-to-unlock varies by feat; there is no fixed battle count target.

Examples:
- Hit an enemy with spell XXX between Y and Z cells -> unlock a long-range Ability.
- Take XXX total damage while shielded -> unlock shield bonus.
- Remove Poison from allies X times + Heal allies for XXX HP + Stay away from enemies for X fights -> unlock healer evolution.
- Get hit by XXX attacks -> gain bonus max health.

### Feat Board
- Visual progression tree (inspired by FF12 permits / PoE passive tree).
- Nodes unlock:
- New Abilitys
- Stat bonuses
- Evolutions
- Nodes require adjacency to a completed node.
- Respec exists but is not a main feature.
- Evolutions are feats that require: (1) completing a specified number of feats, then (2) satisfying a condition. Evolution choices are branching: A -> B or C; B -> D; C -> E. Choosing B blocks C and its branch.

### Taming

Wild creatures are not captured by spending a turn or throwing an item. Instead, each wild `CreatureSpecies` defines a **Taming Profile**: a list of **Taming Conditions** that describe feats the player's team must perform during the fight to impress that creature.

**Taming Conditions** are structured identically to `FeatRequirement` nodes: they specify an in-battle event type, a scope (per-turn, per-fight, per-ability, etc.), and a threshold. Examples:

- Deal more than 80 magical damage in a single hit.
- Keep at least two allies alive until the end of the fight.
- Move a creature to a cell adjacent to the wild creature.
- Use the same Ability three times in a single fight.
- Avoid taking any physical damage for two consecutive turns.

**Mechanics:**

- All Taming Conditions for a wild unit are tracked passively throughout the entire fight, with no player input required.
- When **all** conditions in a wild creature's Taming Profile are fulfilled, the creature is **impressed**: it immediately leaves the fight on its own, removing itself from the board.
- A creature that leaves through taming does **not** count as defeated; the battle continues normally until a standard victory or defeat condition is reached.
- If the wild unit is defeated before all its Taming Conditions are met, taming fails for that unit permanently in that encounter.
- At the end of a **won** battle, every impressed creature joins the player's active team (if a slot is available) or is sent to PC storage (if the team is full).
- If the player **loses** the battle, all impressed creatures are forfeit — they do not join the player's team or PC.
- Taming is only possible in wild encounters. Trainer and gym battles never apply Taming Conditions.

**Design intent:**

- Taming rewards deliberate, creative play patterns — not grinding or item use.
- Conditions are species-specific and hint at the creature's personality or fighting style.
- Players discover Taming Conditions organically through observation or by consulting in-game lore.

---

## 8) Team Management
- Player can carry up to 6 creatures in the active team.
- Additional creatures are stored in a PC (like Pokemon).
- Team composition is flexible and editable between encounters.

---

## 9) Enemy AI
- AI is rule-driven with a decision list.
- Each enemy has a list of conditions and Abilitys.
- The AI evaluates top-down and executes the first valid Ability.
- AI uses the same AP/MP rules as the player.
- AI behavior is per-creature scripted (not purely archetype-based).

Example:
- If enemy within 3 tiles ? cast Ability A.
- If ally HP < 50% ? cast Ability B.
- Otherwise ? move away.

---

## 10) Progression Structure
- Gyms are randomized in order and location.
- Each gym is tuned for a certain unlock tier, not level. Each gym has 8 different teams depending on how many gyms have already been defeated.
- Elite Four is the endgame challenge.
- Unlock tier is based on the number of gyms beaten. Encounters (wild/trainer/gym) scale by gyms defeated to keep challenge consistent.
- Encounter definitions can have 8 tiered lists (one per gym beaten), with a possible 9th list after Elite Four.

---

## 11) Saving & Persistence
Save data should include:
- World seed and generated POI layout.
- Player team and PC storage.
- Feat board unlocks per creature.
- Defeated gyms and trainers.
- World seed is fixed for the entire run and persists across saves.

### Loss Condition
- If the player loses a battle, they still gain progression from that fight.
- The player is returned to the previously visited heal point (like Pokemon).
- Won battles cannot be repeated; trainers and gyms are cleared permanently.

---

## 12) Creature Stats (Current List)
- Health
- Strength (boosts physical damage)
- Ability (boosts magical damage)
- Armor (reduces physical damage taken)
- Resistance (reduces magical damage taken)
- Ability Points (AP) (resources to cast Abilitys)
- Movement Points (MP) (resources to move on the board)
- Stamina (turn frequency)
- Range (adds cells to Ability range)
- Passive effects (poison resistance, lifesteal, healing bonuses, immunities, etc.)

---

## 13) World Guidance & Player Direction
- The player is guided by roads that connect locations.
- The main objective is to discover all creatures and defeat the Elite Four.
- Towns/cities/POIs are generated first, then connected via road placement using a Voronoi-style diagram; pathing may be upgraded to account for height differences.

---

## 14) What Makes Erelia Different
- No experience levels.
- Progression depends on **what you do in battle**, not just winning.
- Faster creatures act more often in a stamina-turn system.
- Randomized world order keeps each run unique.

---
