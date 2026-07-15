# Modes, encounters, and lifecycle

Step 12 makes GameContext::control.mode the only exploration/battle authority.
GameContext::isExplorationActive() is the semantic gate used by exploration input, camera
control, and actor-path advancement; WorldContext no longer has an independently writable
activity flag. The scene publishes worldSeed and its immutable worldPlan with the live
world/navigation transaction and clears them after battle shutdown, before world teardown.

ExplorationInteractionResolver is a read-only, value-producing service. An arrival resolves
exactly one interaction in this order: portal, reserved scripted slot, Bush wild encounter,
none. Bush detection checks the actor support cell plus Y and VoxelData::hasTag("Bush").
It derives resolution and combat streams from a stable encounter identity, resolves the team
once, and reports whether the caller must advance PlayerData::encounterSerial (including a
failed Bush chance). F8 uses the same resolver for the debug-battle table.

ModeManager owns the optional BattleModeRuntime, a one-value transition queue, and
process-local generations. Scene update applies queued exploration interactions after engine
iteration, then processes mode transitions and runs the bounded BattlePump. Entry builds the
selected live or handcrafted BoardData and BattleSession before changing mode. Failed entry
publishes an owned EntryFailed notice and restores fluid activity. Successful entry clears
the exploration path, pauses fluid, hides the player and hover renderers, publishes construction
batches, then emits Entered. A terminal pump result emits ResultReady and queues exit for
the following frame boundary; exit emits WillExit/Exited and restores captured render/fluid
state after destroying the session.

EventCenter::battleLifecycle and battleBatchPublished carry only copied lifecycle values,
including the generation and battle id. Presentation code must treat that pair as the validity
token and reacquire the active session synchronously.

Live boards activate a separate battle streamer whose window comes from BoardBuilder's checked
pin plan; the normal player streamer remains active, so the frozen required chunks stay in their
union for the session. Its origin, range, and activation state are restored on every failed entry
or normal exit.

For the handcrafted branch, the runtime materializes BoardBuilder's immutable source, suspends
the captured generated-world chunks and player streamer, and registers one arena entity in the
same engine. Its mesh is derived from the validated geometry prefab at the board-local identity
transform; it is removed before the generated-world state is restored. The current content pack
has no handcrafted definitions, but the branch is independent of WorldPlan, live chunks, and
world seed.
