# Priority-sorted render-pass bucket pack

Sparkle constructs every frame in one `RenderPassBucketPack`. Scene features,
component logics, and widgets all contribute to that shared per-frame pack; the
pack sorts physical passes and lowers directly to the generic `RenderPlan`.
There is no intermediate scene plan or semantic phase hierarchy.

```text
frame construction
  -> RenderPassBucketPack
    -> RenderPlan (inspectable, polymorphic pass ownership)
      -> RenderUnit (move-owned render-thread snapshot)
```

The generic `structures/graphics/rendering` layer knows only pass identities,
targets, clears, command contributions, plans, and units. It does not depend on
components, cameras, scenes, lighting semantics, or widgets. A CTest check
enforces that dependency direction.

## Identity and lifetime

`RenderPass::TypeId` is a stable 64-bit FNV-1a hash made from a canonical literal.
It identifies a pass family such as `spk.scene.main_opaque`. A complete
`RenderPass::Key` also contains:

- `scope`: the stable lifetime ID of one scene renderer, root window, or capture;
- `instance`: a repeated pass within that family and scope, such as a shadow
  cascade.

Thus two windows can both have `WidgetOverlay[0]`, and one scene can have
`DirectionalShadow[0..2]`, without collisions. Scope IDs come from a monotonic
generator and never expose owner addresses.

Persistent code may retain pass IDs, names, default priorities, and factories.
The command-bearing `RenderPass` instances are always new for each frame. After
`RenderPassBucketPack::build()`, the pack is sealed, pass ownership moves into
the plan, and retained contribution sinks reject mutation. The next frame starts
with an empty pack, so an executing `RenderUnit` cannot observe next-frame
changes.

Creation and lookup are deliberately separate. Pipelines and features use
`emplacePass`; contributors use `require`, `find`, or `passesOfType`. Duplicate
complete keys and missing required passes are configuration errors and throw.
Optional features use `find`/`passesOfType`, not exceptions as control flow.
Typed lookup uses `dynamic_cast` and reports a deterministic type mismatch.

## Ordering

Lower numeric pass priority executes first. Equal priorities retain declaration
order. The reserved ranges are:

```text
0..999       scene pre-main and shadow-like feature passes
1000..1999   main scene geometry and scene-space overlays
2000..2999   scene post-processing
3000..3999   screen-space widget/UI overlay
4000+        final presentation, capture, or debug passes
```

The defaults are `MainOpaque=1000`, `MainTransparent=1100`,
`MainOverlay=1200`, `PostProcessingBase=2000`, and `WidgetOverlay=3000`.
Pass-wide camera/light state uses the reserved contributor priority
`RenderContributionPriorities::PassSetup=-100000`.

Inside one pass, contributions sort by contributor priority ascending, then
logic/widget registration order. Commands within one contribution retain
insertion order. Logic priority still controls update/event ordering and is
independent of pass priority and pass-specific render priority.

## Default passes and widgets

Each scene scope installs `MainOpaque`, `MainTransparent`, and `MainOverlay`.
Only opaque clears the scene framebuffer; transparent and scene overlay rebind
the target without clearing it. Features may add typed shadow or post-processing
passes at priorities in their ranges.

Each root widget traversal installs one `WidgetOverlay` for its window/capture
scope. Ordinary widget viewport/scissor and cached draw units become ordered
contributions in that pass. `MainOverlay` is a scene-target extension point;
`WidgetOverlay` is screen-space UI and executes after all scene and post passes.

`GameEngineWidget` declares its scene passes against its owned off-screen
framebuffer. Its cached widget unit contains only the flipped framebuffer image,
which is contributed later in normal widget hierarchy order to `WidgetOverlay`:

```text
DirectionalShadow[*] -> MainOpaque -> MainTransparent -> MainOverlay
  -> scene post processing -> WidgetOverlay (GameEngineWidget image + UI)
```

The active-target flag on `RenderTargetReference` is reserved for explicit
capture/compatibility callers that already selected a target. Normal window and
scene paths identify their target directly.

## Adding a typed custom pass

```cpp
inline constexpr auto SelectionMask =
    spk::makeRenderPassTypeId("game.selection_mask");

class SelectionMaskPass : public spk::RenderPass
{
    std::uint32_t _layer;
public:
    SelectionMaskPass(spk::RenderPass::Key key, std::int32_t priority,
        std::size_t order, std::string name,
        spk::RenderPass::Description description, std::uint32_t layer)
        : RenderPass(key, priority, order, std::move(name),
              std::move(description)), _layer(layer) {}
    std::uint32_t layer() const noexcept { return _layer; }
};

// Feature declaration (post-processing range).
p_context.frame.passes.emplacePass<SelectionMaskPass>(
    {.type = SelectionMask, .scope = p_context.sceneScope, .instance = 0},
    spk::SceneRenderPriorities::PostProcessingBase + 20,
    "SelectionMask",
    {.debugName = "SelectionMask", .target = maskTarget, .clear = {}},
    3u);

// One component logic may contribute to opaque and the custom typed pass.
auto &opaque = p_context.frame.passes.require(
    {.type = spk::SceneRenderPasses::MainOpaque,
     .scope = p_context.sceneScope});
opaque.contribute(renderPriority(spk::SceneRenderPasses::MainOpaque),
    p_context.contributorRegistrationOrder).emplace<DrawObjectCommand>(object);

if (auto *mask = p_context.frame.passes.find<SelectionMaskPass>(
        {.type = SelectionMask, .scope = p_context.sceneScope}); mask != nullptr)
{
    mask->contribute(renderPriority(SelectionMask),
        p_context.contributorRegistrationOrder)
        .emplace<DrawSelectionCommand>(object, mask->layer());
}
```

Diagnostics expose type/scope/instance, debug and concrete type names, priority,
declaration order, target/viewport, typed clear, contributor count, and command
count. In the example, `SelectionMask` appears after the three main scene passes
but before `WidgetOverlay`, regardless of feature registration order.
