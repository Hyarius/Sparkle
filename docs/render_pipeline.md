# Scene render pipeline

Sparkle records each game frame as an inspectable `RenderPlan`. A plan owns an
ordered list of `RenderPass` objects and compiles them into the existing
move-owned `RenderUnit` snapshot. Plan construction performs no OpenGL work;
only the render thread executes the compiled commands.

## Passes and phases

A **pass** owns one target-binding and clear lifetime. Its description captures
the framebuffer (or the default window framebuffer), viewport/scissor, typed
color/depth/stencil clear policy, identity, and ordered phase list. Every pass
compiles an explicit target bind, so normal orchestration never restores an
unknown previous framebuffer.

A **phase** is ordered work while that target and depth buffer remain active.
The main scene is one pass with `PassSetup`, `SceneOpaque`,
`SceneTransparent`, and `SceneOverlay`; opaque and transparent work are not
separate passes and the depth buffer is not cleared between them.

`SceneRenderPipeline` exclusively owns pass order. A pass description owns its
phase order. Component logics declare a `RenderPhaseMask`, and
`ComponentLogicRegistry` invokes only contributors that declared the requested
phase.

## Ordering and priorities

The supported extension order is deliberately simple:

1. every feature's pre-main hook, in registration order;
2. exactly one main scene pass;
3. every feature's post-main hook, in registration order.

Render priority only orders contributors inside one phase. Higher values run
first and equal values retain registration order. `setRenderPriority(phase,
priority)` changes only that phase's cached contributor order. The inherited
logic priority continues to control update/event order and is the compatibility
default for phases without an explicit render priority.

Transparent contributors run only after every opaque contributor. Individual
contributors may sort their own work back-to-front (transparent voxel chunks
do), but Sparkle does not globally sort triangles or merge translucent queues
across renderer types.

## Frame and feature ownership

`GameEngineWidget` supplies its off-screen framebuffer, captured viewport, and
transparent-black/depth-one/stencil-zero clear through `RenderFrameRequest`.
After the engine plan, the widget explicitly binds the default framebuffer and
draws the color attachment with the existing vertical flip.

Pass-wide 3D camera and placeholder directional-light uploads are recorded in
the main `PassSetup` phase, independently of geometry. Pipeline features are
renderer services, not components. They may prepare immutable frame data, add
pre-main passes, add main setup commands, or add post-main passes.

A future cascaded-shadow feature can loop over cascade indices, add one
depth-only `Shadow` pass per index, record cascade setup, and invoke the same
`ShadowCaster` contributor phase for every pass. The main target is then
explicitly rebound by the main pass; no priority bands or framebuffer pop are
needed.

## Lifetimes and compatibility

Phase and frame contexts exist only while the plan is built. Commands copy
matrices and scalar state needed later and retain the same stable resource
owners used by existing commands. Framebuffer pointers remain borrowed and
their owner must outlive the compiled snapshot.

Repeated feature phases may attach immutable, typed data derived from
`RenderPhaseFeatureData` to `RenderPhaseContext`; `feature<T>()` avoids
free-form names and untyped pointers. `passInstance` is always available for a
stable cascade or repeated-pass index.

The legacy no-argument `GameEngine::buildRenderUnit()` uses a documented 1x1
default-framebuffer request with no clear. `ComponentLogicRegistry::render()`
is retained for callers that already own an active target (including visual
test capture); it still dispatches explicit semantic phases but intentionally
does not emit a target transition. New engine/widget code uses
`RenderFrameRequest` and `SceneRenderPipeline`.
