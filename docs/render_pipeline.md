# Render pipeline

Sparkle builds one `RenderPipeline` per frame. A pipeline owns passes by a
string ID and keeps an ordered view of the same objects. `build()` seals the
passes, sorts them by `PriorizableTrait::priority()` (lower values first), and
moves them into an immutable `RenderPlan`.

```text
frame construction -> RenderPipeline -> RenderPlan -> RenderUnit
```

## Passes

Passes are created with an ID, a priority, and a target/clear description:

```cpp
auto &opaque = passes.emplace<spk::RenderPass>(
    "scene.main.opaque",
    1000,
    {.target = sceneTarget, .clear = sceneClear});

opaque.emplace<DrawObjectCommand>(object);
```

IDs are unique within one pipeline. Use `require("...")` when a pass is
required, `find("...")` for an optional pass, and `all<T>()` when a feature
needs every pass of one concrete type (for example, all shadow cascades).
Custom passes only need to derive from `RenderPass` and accept the standard
`(id, priority, description)` constructor arguments before their own state.

The pass itself owns only its command list and render description. It becomes
immutable when the pipeline is built. Commands are consumed once while the
render plan is compiled, so a plan cannot be changed by the next frame.

Lower priorities run first. The current convention is:

```text
below 1000   feature setup and shadow passes
1000-1999    scene geometry and scene overlays
2000-2999    post-processing
3000-3999    widgets and UI
4000+        presentation and debug passes
```

The default scene passes are `MainOpaque` (1000), `MainTransparent` (1100),
and `MainOverlay` (1200). The default widget pass is `WidgetOverlay` (3000).

## Diagnostics

`RenderPipeline::diagnostics()` and `RenderPlan::diagnostics()` expose the ID,
priority, target state, viewport, clear state, and command count. There is no
separate type ID, scope ID, contribution sink, or contributor ordering layer.
If a feature needs multiple instances, it gives each pass a distinct string
ID, such as `scene.shadow.cascade.0`.

## Render context

`RenderContext` is the OpenGL lifetime and state bridge for a window. Its
context ID and registry let OpenGL objects find the context that owns them;
the thread-local `current()` identifies the context bound to the calling
thread. The release queue defers destruction of GL objects until that context
is current. The binding cache avoids redundant program, vertex-array, and
uniform-buffer binds. These pieces are required by OpenGL resource lifetime
and state tracking; the profiler is intentionally owned by `Window` and is
not part of `RenderContext`.
