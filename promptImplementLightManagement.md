# Implement editable 3D lights and cascaded directional shadows in Sparkle

Implement a reusable Sparkle lighting system for 3D scenes, migrate Playground
away from its hard-coded directional-light placeholder, and render dynamic
cascaded directional shadows over the streamed voxel world and ordinary
textured 3D meshes.

This is an implementation task, not only an architectural proposal.

Modify the engine, migrate Playground, add tests and shaders, update
documentation, and ensure the complete Sparkle and Playground projects compile.

---

# 1. Repository baseline

Build against the current architecture in `Hyarius/Sparkle`.

Before editing, inspect the repository again and treat the checked-out code as
the final source of truth. The paths and APIs below reflect the repository state
when this task was written.

The relevant existing architecture is:

```text
RenderPassBucketPack
    -> RenderPlan
        -> RenderUnit
```

There is no semantic render-phase hierarchy.

The current scene pipeline:

* declares `MainOpaque`, `MainTransparent`, and `MainOverlay`;
* calls every feature's `prepareFrame`;
* calls every feature's `declarePasses`;
* appends default frame state;
* calls every feature's `contribute`;
* invokes component logics exactly once for rendering.

The existing typed directional-shadow pass is:

```cpp
namespace spk::LightingRenderPasses
{
    inline constexpr auto DirectionalShadow =
        spk::makeRenderPassTypeId("spk.lighting.directional_shadow");
}

class spk::ShadowRenderPass : public spk::RenderPass
{
public:
    [[nodiscard]] std::uint32_t cascadeIndex() const noexcept;
    [[nodiscard]] const spk::Matrix4x4 &lightViewProjection() const noexcept;
};
```

Reuse this pass family. Do not create another shadow orchestration path.

The current hard-coded light remains in:

```text
srcs/structures/game_engine/rendering/spk_scene_render_pipeline.cpp
```

inside `SceneRenderPipeline::_appendDefaultFrameState`.

The current implementation uploads:

```cpp
spk::DirectionalLight{
    .direction = spk::Vector3(1.0f, -2.0f, 0.5f).normalized(),
    .color = spk::Color(1.0f, 0.95f, 0.85f),
    .ambient = 0.35f
}
```

to the three main passes. This placeholder must be removed.

---

# 2. Required final behavior

Sparkle must provide editable lights attached to `spk::Entity3D` objects.

```text
directional light:
    direction comes from entity world rotation
    entity position does not affect illumination
    suitable for the sun

point light:
    position comes from entity world position
    direction is ignored
    suitable for exposed bulbs and localized glow

spot light:
    position comes from entity world position
    direction comes from entity world rotation
    suitable for hooded or directed lamps
```

The voxel world and ordinary textured 3D meshes must:

* receive directional, point, and spot diffuse illumination;
* receive cascaded shadows from one selected directional light;
* cast directional shadows when their renderer permits it;
* retain the existing opaque-before-transparent ordering.

Point-light and spot-light shadow maps are not required.

---

# 3. Implementation milestones

Implement the task in milestones and keep the repository compiling after each
milestone.

```text
Milestone 1:
    Component3D
    Light3D
    Transform3D world helpers
    Camera3D getters
    component and math tests

Milestone 2:
    packed GPU layouts
    binding contract
    light collection
    unshadowed directional/point/spot forward lighting
    collection and shader-layout tests

Milestone 3:
    cascade split and fitting math
    persistent shadow resources
    immutable frame resources
    typed shadow-pass declaration
    cascade math and render-order tests

Milestone 4:
    voxel and texture-mesh shadow commands
    cast/receive policy
    main-pass shadow sampling
    OpenGL state tests

Milestone 5:
    Playground sun and time-of-day controls
    debug diagnostics
    documentation
    complete builds, tests, and visual validation
```

Do not implement later milestones by leaving earlier APIs in a temporary or
contradictory state.

---

# 4. Pass identity and ordering

Every frame is constructed in one `spk::RenderPassBucketPack`.

A pass is identified by:

```cpp
spk::RenderPass::Key{
    .type = ...,
    .scope = ...,
    .instance = ...
}
```

`instance` identifies repeated passes such as cascades.

The reserved priorities are:

```text
ShadowBase          = 100
MainOpaque          = 1000
MainTransparent     = 1100
MainOverlay         = 1200
PostProcessingBase  = 2000
WidgetOverlay       = 3000
```

Passes execute by integer priority and then declaration order.

`RenderPlan::compile()` already lowers each pass to:

```text
UseFrameBufferRenderCommand
typed clear
pass contributions
```

Therefore:

* shadow passes must use priorities before `MainOpaque`;
* each shadow pass declares its own target and depth clear;
* the main framebuffer is rebound automatically;
* no manual framebuffer push, pop, or restore mechanism may be added.

`GameEngineWidget` must remain unaware of light types and cascades.

---

# 5. Responsibility boundaries

## Scene components own

`Light3D` owns authored scene data:

* light shape;
* color;
* intensity;
* point or spot range;
* spot cone angles;
* shadow intent;
* deterministic selection priority;
* activation through the existing component/entity hierarchy.

## Entity transforms own

`Transform3D` owns:

* point and spot world position;
* directional and spot world orientation;
* parent transform composition.

`Light3D` must not duplicate a position, rotation, transform, or enabled flag.

## Scene lighting feature owns

`SceneLightingRenderFeature` owns renderer-side lighting behavior:

* collecting processable lights;
* selecting bounded deterministic lists;
* selecting one directional shadow owner;
* producing immutable per-frame GPU snapshots;
* persistent shadow framebuffer resources;
* cascade split and fitting calculations;
* declaring one shadow pass per cascade;
* contributing main-pass lighting state;
* diagnostics.

## Geometry renderers own

Renderable components own:

* cast-shadow policy;
* receive-shadow policy;
* mesh, texture, and tint data;
* model transforms;
* opaque or translucent classification.

## Playground owns

Playground owns:

* creation and registration of the sun entity;
* initial sun settings;
* time-of-day behavior;
* artistic sun color and intensity curves;
* ambient-environment editing;
* controls and presentation.

Sparkle must not depend on `pg::GameContext`, voxel-world generation, actors,
biomes, or Playground-specific classes.

---

# 6. Generic graphics dependency rule

The generic `structures/graphics` and `structures/graphics/rendering` layers may
know:

* packed trivially copyable GPU data;
* binding constants;
* textures;
* buffers;
* samplers;
* shaders;
* draw commands;
* render targets.

They must not depend on:

* `spk::Component`;
* `spk::ComponentRegistry`;
* `spk::Entity`;
* `spk::Entity3D`;
* `spk::Transform3D`;
* `spk::Camera3D`;
* `spk::Light3D`;
* scene-level selection policy.

Anything that reads components, transforms, cameras, or the component registry
belongs under:

```text
structures/game_engine/rendering
```

next to `spk_shadow_render_pass.hpp`.

---

# 7. Remove obsolete placeholder architecture

Remove:

```text
includes/structures/graphics/spk_directional_light.hpp
includes/structures/graphics/rendering/command/spk_directional_light_update_render_command.hpp
srcs/structures/graphics/rendering/command/spk_directional_light_update_render_command.cpp
tests/TUs/srcs/structures/graphics/directional_light_test.cpp
includes/structures/graphics/rendering/pipeline/spk_render_pass.hpp
```

The last header is an obsolete phase-based orphan and conflicts with the current
physical `spk::RenderPass`.

In `SceneRenderPipeline::_appendDefaultFrameState`:

* remove the hard-coded light;
* retain camera upload as camera-only default state;
* upload camera state exactly once per main pass;
* do not upload any lighting state there.

The final call order remains:

```text
prepare lighting snapshot
declare shadow passes
append camera-only state
contribute lighting state
collect geometry once
```

Do not move pass-wide state into voxel or mesh geometry logics.

---

# 8. Add `Component3D`

Add:

```text
includes/structures/game_engine/spk_component_3d.hpp
srcs/structures/game_engine/spk_component_3d.cpp
```

with an API equivalent to:

```cpp
namespace spk
{
    class Component3D : public spk::Component
    {
    protected:
        Component3D() = default;
        void _onAttached(spk::Entity &p_entity) override;

    public:
        ~Component3D() override = default;

        [[nodiscard]] spk::Entity3D *entity();
        [[nodiscard]] const spk::Entity3D *entity() const;

        [[nodiscard]] spk::Transform3D &transform();
        [[nodiscard]] const spk::Transform3D &transform() const;
    };
}
```

Requirements:

* attachment to `Entity3D` succeeds;
* attachment to plain `Entity` throws before insertion;
* attachment to `Entity2D` throws before insertion;
* use the existing `Component2D` attachment contract as the model;
* access `Entity3D::transform()` rather than searching for or duplicating a
  transform.

Use `Component3D` for `Light3D`.

Migrating every existing 3D component to this base is optional unless required
for correctness.

---

# 9. Correct world-transform APIs

The current `Transform3D` model composes:

```text
parent model * translation * rotation * scale
```

Do not derive semantic light orientation by multiplying a direction by the
complete model matrix and normalizing it. Non-uniform parent scale can skew such
a vector.

Add APIs equivalent to:

```cpp
[[nodiscard]] spk::Vector3 worldPosition() const;

[[nodiscard]] spk::Vector3 transformPoint(
    const spk::Vector3 &p_localPoint) const;

[[nodiscard]] spk::Vector3 transformVector(
    const spk::Vector3 &p_localVector) const;

[[nodiscard]] spk::Quaternion worldRotation() const;

[[nodiscard]] spk::Vector3 worldForward() const;
```

Define their contracts precisely.

## `transformPoint`

Uses the complete composed model transform with homogeneous `w = 1`.

Translation, rotation, scale, and parent transforms affect the result.

## `transformVector`

Uses the linear portion of the complete composed model transform with
homogeneous `w = 0`.

Translation does not affect the result. Scale is allowed to affect the vector.
This function does not promise a normalized result.

## `worldRotation`

Composes ancestor and local quaternions explicitly:

```text
world rotation = parent world rotation * local rotation
```

It ignores translation and scale.

Validate or normalize quaternions according to existing Sparkle conventions.
A non-finite or zero-length quaternion must fail deterministically rather than
producing NaNs.

## `worldForward`

Uses:

```text
local forward = (0, 0, -1)
world forward = worldRotation applied to local forward
```

The returned vector is normalized.

Entity scale, including non-uniform scale, must not skew `worldForward()`.

A zero entity scale does not make world rotation undefined. It may make the
inverse model transform undefined, but it must not invalidate a valid semantic
orientation.

`worldPosition()` must be equivalent to:

```cpp
transformPoint({0.0f, 0.0f, 0.0f});
```

Do not treat `Transform3D::position()` as world position; it is local state.

---

# 10. Unified `Light3D` component

Use one concrete component type because the current component registry stores
exact dynamic types.

Add a model equivalent to:

```cpp
namespace spk
{
    struct DirectionalLightSettings
    {
    };

    struct PointLightSettings
    {
        float range = 10.0f;
    };

    struct SpotLightSettings
    {
        float range = 15.0f;
        float innerHalfAngleDegrees = 20.0f;
        float outerHalfAngleDegrees = 30.0f;
    };

    using LightShape = std::variant<
        DirectionalLightSettings,
        PointLightSettings,
        SpotLightSettings>;

    class Light3D final : public spk::Component3D
    {
    private:
        LightShape _shape;
        spk::Color _color{1.0f, 1.0f, 1.0f, 1.0f};
        float _intensity = 1.0f;
        bool _castsShadows = false;
        int _priority = 0;
    };
}
```

Equivalent type-safe storage is acceptable.

Do not:

* use three separate derived light components;
* extend the component registry solely to retrieve light subclasses;
* store all type-specific values in one unvalidated bag;
* make packed GPU structures inherit from `Component`.

---

# 11. `Light3D` public API

Provide APIs equivalent to:

```cpp
void setDirectional();

void setPoint(float p_range);

void setSpot(
    float p_range,
    float p_innerHalfAngleDegrees,
    float p_outerHalfAngleDegrees);

void setColor(const spk::Color &p_color);
void setIntensity(float p_intensity);
void setCastsShadows(bool p_castsShadows);
void setPriority(int p_priority);

[[nodiscard]] bool isDirectional() const noexcept;
[[nodiscard]] bool isPoint() const noexcept;
[[nodiscard]] bool isSpot() const noexcept;

[[nodiscard]] const LightShape &shape() const noexcept;
[[nodiscard]] const spk::Color &color() const noexcept;
[[nodiscard]] float intensity() const noexcept;
[[nodiscard]] bool castsShadows() const noexcept;
[[nodiscard]] int priority() const noexcept;
```

All public angle values are in degrees.

Changing light kind at runtime is permitted.

Every setter must either complete fully or leave the previous state unchanged.

Activation is governed by `Component::isProcessable()`. Do not add another
enabled flag.

---

# 12. `Light3D` validation

Validate before storing.

Required rules:

* intensity is finite and non-negative;
* range is finite and strictly positive;
* spot inner half-angle is finite and non-negative;
* spot outer half-angle is finite;
* outer half-angle is greater than or equal to inner half-angle;
* outer half-angle is strictly less than 90 degrees;
* RGB channels are finite and non-negative;
* alpha is ignored by lighting;
* RGB values above one are permitted;
* zero intensity is valid;
* directional lights ignore position;
* point lights ignore direction;
* point and spot `castsShadows` intent is retained but unsupported by the
  renderer in this task.

Do not silently clamp invalid values.

---

# 13. Direction convention

Use one convention everywhere:

```text
Light direction = normalized direction in which rays travel
                  from the emitter into the scene.

Entity local -Z = light ray-travel direction before world rotation.
```

Therefore:

```text
Lambert vector toward a directional light = -light.direction
```

For a spotlight:

```text
vectorToFragment =
    normalize(fragmentWorldPosition - spotWorldPosition)

cone cosine =
    dot(spot.direction, vectorToFragment)
```

Document this convention in:

* `Light3D`;
* packed GPU structures;
* cascade calculations;
* shaders;
* tests.

Do not alternate between ray direction and direction toward the light.

---

# 14. Environment lighting

Ambient illumination is environment state, not a light property.

Add:

```cpp
struct EnvironmentLighting
{
    spk::Color ambientColor{1.0f, 1.0f, 1.0f, 1.0f};
    float ambientIntensity = 0.20f;
};
```

`SceneLightingRenderFeature` may own and expose it through methods equivalent to:

```cpp
void setEnvironmentLighting(const spk::EnvironmentLighting &p_environment);
[[nodiscard]] const spk::EnvironmentLighting &environmentLighting() const;
```

Validation:

* RGB is finite and non-negative;
* intensity is finite and non-negative;
* alpha is ignored;
* RGB above one is permitted.

Ambient:

* remains when every direct light has zero intensity;
* is never shadowed;
* can be edited by Playground without editing `Light3D`.

Do not implement image-based lighting, probes, or sky lighting.

---

# 15. Camera API and GPU camera state

`Camera3D` currently stores its perspective data privately.

Expose read-only APIs equivalent to:

```cpp
[[nodiscard]] float fieldOfView() const noexcept;
[[nodiscard]] float nearPlane() const noexcept;
[[nodiscard]] float farPlane() const noexcept;
[[nodiscard]] float aspectRatio() const noexcept;
[[nodiscard]] const spk::Vector3 &up() const noexcept;
```

Retain existing camera behavior and `rayFromViewport` results.

Validate camera projection data before cascade construction:

* finite field of view;
* field of view strictly between 0 and 180 degrees;
* finite positive aspect ratio;
* finite positive near plane;
* finite far plane greater than near plane.

The current camera UBO only contains `viewProjection`. Cascade selection in the
fragment shader requires camera-view depth, so extend the camera GPU block to:

```cpp
struct alignas(16) CameraGpuData
{
    spk::Matrix4x4 viewProjection;
    spk::Matrix4x4 view;
};
```

Matching GLSL:

```glsl
layout(std140, binding = 1) uniform CameraData
{
    mat4 uViewProjection;
    mat4 uView;
};
```

Add size, alignment, and trivial-copy assertions.

Update `CameraUpdateRenderCommand` to copy both matrices.

The camera command remains the only camera-state contributor to the main passes.

---

# 16. Centralized GPU binding contract

Add one C++ binding contract under the generic graphics layer, for example:

```text
includes/structures/graphics/rendering/spk_scene_gpu_bindings.hpp
```

Use named constants equivalent to:

```text
Camera                 = 1
Model                  = 2
LightingHeader         = 3
DirectionalLights      = 4
PointLights            = 5
SpotLights             = 6
DirectionalShadow      = 7

MaterialTextureUnit     = 0
DirectionalShadowUnit0 = 8
DirectionalShadowUnit1 = 9
DirectionalShadowUnit2 = 10
DirectionalShadowUnit3 = 11
```

C++ commands and tests must use the constants.

GLSL cannot include the C++ header, so document the same contract in one
renderer document and keep shader declarations textually consistent.

Do not leave numeric binding literals duplicated through voxel and ordinary
mesh command implementations.

---

# 17. Packed GPU data

Create packed, trivially copyable data separate from components.

Use layouts equivalent to:

```cpp
struct alignas(16) DirectionalLightGpuData
{
    // xyz = ray-travel direction, w = intensity
    spk::Vector4 directionAndIntensity;

    // rgb = light color, alpha ignored
    spk::Color color;
};

struct alignas(16) PointLightGpuData
{
    // xyz = world position, w = range
    spk::Vector4 positionAndRange;

    // rgb = color, w = intensity
    spk::Vector4 colorAndIntensity;
};

struct alignas(16) SpotLightGpuData
{
    // xyz = world position, w = range
    spk::Vector4 positionAndRange;

    // xyz = ray-travel direction, w = cos(inner half-angle)
    spk::Vector4 directionAndInnerCosine;

    // rgb = color, w = intensity
    spk::Vector4 colorAndIntensity;

    // x = cos(outer half-angle), yzw = padding
    spk::Vector4 outerCosineAndPadding;
};
```

Use SSBO `std430` layout for light arrays.

Add a header/environment UBO equivalent to:

```cpp
struct alignas(16) LightingHeaderGpuData
{
    // rgb = ambient color, w = ambient intensity
    spk::Vector4 ambientColorAndIntensity;

    std::uint32_t directionalCount = 0;
    std::uint32_t pointCount = 0;
    std::uint32_t spotCount = 0;

    // -1 when no selected directional light owns shadows.
    std::int32_t shadowDirectionalIndex = -1;
};
```

This block uses `std140`.

Every structure must have:

* explicit alignment;
* exact `sizeof` assertion;
* exact alignment assertion;
* `std::is_trivially_copyable_v` assertion;
* matching documented GLSL layout.

Do not upload variants, pointers, references, `bool`, components, or entities.

---

# 18. Directional-shadow GPU data

Use a main-pass shadow block equivalent to:

```cpp
struct alignas(16) DirectionalShadowGpuData
{
    std::array<spk::Matrix4x4, 4> lightViewProjection;

    // Each active entry is the far camera-view distance of that cascade.
    spk::Vector4 cascadeFarDistances;

    // x = constant bias
    // y = normal bias
    // z = maximum shadow distance
    // w = cascade transition fraction
    spk::Vector4 biasDistanceAndTransition;

    // x = cascade count
    // y = PCF radius
    // z = enabled (0 or 1)
    // w = debug visualization mode
    std::array<std::uint32_t, 4> control;
};
```

Equivalent packing is acceptable.

Use a separate per-shadow-pass setup block containing one matrix:

```cpp
struct alignas(16) DirectionalShadowPassGpuData
{
    spk::Matrix4x4 lightViewProjection;
};
```

It may use binding point 7 because shadow and main programs are separate
programs.

Do not rely on uninitialized unused matrix or split entries. Initialize all
four entries deterministically.

---

# 19. Immutable frame-resource model

Sparkle builds render commands before later execution. A queued render command
must not observe next-frame mutation.

Implement immutable frame resources.

A suitable model is:

```cpp
class SceneLightingFrameResources
{
public:
    LightingHeaderGpuData header;
    std::vector<DirectionalLightGpuData> directional;
    std::vector<PointLightGpuData> point;
    std::vector<SpotLightGpuData> spot;
    DirectionalShadowGpuData shadow;

    spk::UniformBufferObject headerUBO;
    spk::BufferObject directionalSSBO;
    spk::BufferObject pointSSBO;
    spk::BufferObject spotSSBO;
    spk::UniformBufferObject shadowUBO;
};
```

Equivalent organization is acceptable.

Required behavior:

1. `prepareFrame` copies semantic light data.
2. It creates or publishes a frame-resource object.
3. All CPU buffer contents are finalized before commands retain the object.
4. The object is never edited after publication.
5. Main-pass commands capture `shared_ptr<const SceneLightingFrameResources>`.
6. No command stores a `Light3D *`.
7. No command reads a transform during `execute()`.
8. Empty light categories still produce safely bindable empty buffers or a
   renderer-supported minimum allocation.
9. Reuse CPU vector capacity where practical.
10. GPU buffers may only be reused across frames when it is proven that no
    executing render snapshot can observe modified contents.

Do not solve snapshot races by reading mutable gameplay objects from the render
thread.

---

# 20. Render-target ownership across queued frames

The current render-target references and framebuffer-binding commands use raw
framebuffer pointers. Shadow settings may replace framebuffer resources while
an older `RenderUnit` still references them.

Extend render-target ownership so queued commands can retain a target.

A suitable compatible design is:

```cpp
struct RenderTargetReference
{
    std::shared_ptr<const spk::FrameBufferObject> ownedFrameBuffer;
    const spk::FrameBufferObject *frameBuffer = nullptr;
    spk::Viewport viewport;
    bool activeTarget = false;
};
```

Equivalent naming is acceptable.

Required behavior:

* existing raw-pointer callers continue to work;
* when `ownedFrameBuffer` is present, the effective target is that object's
  address;
* `RenderPass::Description` retains the ownership handle;
* `UseFrameBufferRenderCommand` retains the same ownership handle after
  `RenderPlan::compile()`;
* replacing shadow settings publishes new framebuffer handles;
* queued frames retain old handles until execution and destruction complete;
* no shadow target is destroyed while referenced by a compiled render unit.

Do not assume retaining ownership only on `ShadowRenderPass` is sufficient,
because the pass is destroyed after lowering.

---

# 21. Frame-safe model UBO snapshots

The current voxel and ordinary-mesh caches update shared model UBO objects in
place.

Shadow commands must not introduce another reader of a model UBO that can be
modified by a later frame.

When a model or tint changes:

* publish a new shared model-UBO resource;
* let commands already created retain the old shared resource;
* let both main and shadow commands of the new frame capture the same new
  resource.

When model data is unchanged, reusing the immutable shared resource is allowed.

Do not edit a model UBO in place after a command has captured it.

Apply this rule consistently to the ordinary mesh and voxel model caches used by
this task.

---

# 22. `SceneLightingRenderFeature`

Add:

```text
includes/structures/game_engine/rendering/spk_scene_lighting_render_feature.hpp
srcs/structures/game_engine/rendering/spk_scene_lighting_render_feature.cpp
```

with:

```cpp
class SceneLightingRenderFeature final
    : public spk::ISceneRenderPipelineFeature
{
public:
    void prepareFrame(
        const spk::SceneRenderBuildContext &p_context) override;

    void declarePasses(
        const spk::SceneRenderBuildContext &p_context,
        spk::RenderPassBucketPack &p_passes) override;

    void contribute(
        const spk::SceneRenderBuildContext &p_context) override;
};
```

Register it explicitly:

```cpp
auto &lighting =
    engine.renderPipeline()
        .emplaceFeature<spk::SceneLightingRenderFeature>();
```

## `prepareFrame`

It must:

* query `p_context.components.components<spk::Light3D>()`;
* retain only processable lights;
* resolve semantic world position and direction;
* ignore zero-intensity lights for direct GPU lists;
* sort deterministic bounded lists;
* select one directional shadow owner;
* compute cascade data when camera and settings are valid;
* publish one immutable frame snapshot;
* update diagnostic counters.

## `declarePasses`

It must:

* declare zero passes without an eligible owner or valid camera;
* declare one `ShadowRenderPass` per active cascade;
* attach an owned persistent depth framebuffer to each pass;
* add per-cascade matrix state at `PassSetup`.

## `contribute`

It must append main-pass lighting resources at
`RenderContributionPriorities::PassSetup`.

Contribute to:

```text
MainOpaque
MainTransparent
MainOverlay
```

to preserve the current placeholder's availability contract.

The camera remains a separate camera-only setup contribution.

---

# 23. Bounded deterministic selection

Use explicit limits:

```text
directional = 4
point       = 128
spot        = 64
```

Equivalent documented constants are acceptable.

Use squared distance; do not compute square roots solely for sorting.

## Directional ordering

```text
priority descending
UUID bytes lexicographically ascending
```

## Point and spot ordering with a main camera

```text
priority descending
squared distance to camera ascending
UUID bytes lexicographically ascending
```

## Point and spot ordering without a main camera

```text
priority descending
UUID bytes lexicographically ascending
```

Do not depend on:

* pointer addresses;
* `unordered_map` iteration;
* component insertion order.

Expose:

* processable count per category;
* selected count per category;
* discarded count per category.

Construct fixed UUIDs in deterministic tests. Do not base ordering tests on
random UUID generation.

---

# 24. Primary shadow-owner selection

Select at most one directional shadow owner.

Eligible lights satisfy:

```text
processable
directional
castsShadows == true
intensity > 0
```

Order:

```text
priority descending
UUID bytes lexicographically ascending
```

Selection is performed across all eligible directional lights.

The selected shadow owner must also be present in the bounded directional GPU
list.

If normal directional truncation excluded it:

1. replace the final selected directional entry with the shadow owner;
2. restore deterministic ordering among the final selected entries;
3. calculate `shadowDirectionalIndex` after the final ordering.

This guarantees that the shader attenuates an uploaded directional light.

If no eligible owner exists:

* `shadowDirectionalIndex = -1`;
* no shadow passes are declared;
* shadow state is explicitly disabled;
* no stale depth map is sampled.

Point and spot shadow requests do not participate.

Do not use a global `mainLight` pointer.

---

# 25. Local-light attenuation

Implement Lambert diffuse contribution in both main 3D fragment shaders.

Point and spot attenuation:

```glsl
vec3 offset = lightPosition - fragmentWorldPosition;
float distanceSquared = max(dot(offset, offset), epsilon);
float distanceValue = sqrt(distanceSquared);
float normalizedDistance = distanceValue / range;

float rangeFade =
    pow(clamp(1.0 - pow(normalizedDistance, 4.0), 0.0, 1.0), 2.0);

float attenuation =
    normalizedDistance >= 1.0
        ? 0.0
        : rangeFade / distanceSquared;
```

Equivalent stable inverse-square-inspired attenuation is acceptable.

Required behavior:

* contribution is zero at and beyond range;
* no division by zero occurs;
* intensity scales linearly;
* normal and light vectors are normalized;
* Lambert terms are clamped to zero.

Spot cone factor:

```glsl
smoothstep(
    cosOuterHalfAngle,
    cosInnerHalfAngle,
    dot(spotDirection, vectorFromSpotToFragment))
```

Store both cosines in GPU data.

Do not implement specular local lighting.

---

# 26. Directional-shadow settings

Add:

```cpp
struct DirectionalShadowSettings
{
    spk::Vector2UInt resolution{2048, 2048};
    std::size_t cascadeCount = 3;
    float maximumDistance = 192.0f;
    float splitLambda = 0.65f;
    float depthPadding = 48.0f;
    float constantBias = 0.0008f;
    float normalBias = 0.0025f;
    std::size_t pcfRadius = 1;
    float transitionFraction = 0.10f;
};
```

Validate:

* both resolution dimensions are non-zero;
* dimensions fit supported OpenGL limits;
* cascade count is one through four;
* maximum distance is finite and positive;
* split lambda is finite and in `[0, 1]`;
* depth padding is finite and non-negative;
* both biases are finite and non-negative;
* PCF radius is bounded by a documented renderer maximum;
* transition fraction is finite and in `[0, 0.5]`.

Changing settings:

* recreates framebuffer resources only when resolution or cascade count
  requires it;
* publishes replacement owned resources;
* does not invalidate queued frames;
* does not resize shadow maps when the widget resizes.

---

# 27. Cascade split calculation

Compute splits between:

```text
camera near
min(camera far, maximum shadow distance)
```

For cascade `i`:

```cpp
ratio = float(i) / float(cascadeCount);

logarithmic =
    near * std::pow(far / near, ratio);

linear =
    near + (far - near) * ratio;

split =
    std::lerp(linear, logarithmic, splitLambda);
```

Required behavior:

* the first slice starts at camera near;
* the final slice ends at effective shadow distance;
* splits are finite and strictly increasing;
* lambda zero is linear;
* lambda one is logarithmic;
* invalid camera data disables shadows deterministically;
* a missing main camera disables shadows;
* view-depth values sent to the shader use the same positive-distance
  convention as cascade selection.

Use positive camera-view distance:

```glsl
float viewDepth =
    max(-(uView * vec4(fragmentWorldPosition, 1.0)).z, 0.0);
```

Do not use clip-space depth for split selection.

---

# 28. Frustum-slice corners

Provide either a pure helper or a `Camera3D` method equivalent to:

```cpp
std::array<spk::Vector3, 8> frustumSliceCorners(
    float p_nearDistance,
    float p_farDistance) const;
```

Validate:

```text
finite near and far
near > 0
far > near
near >= camera near
far <= camera far, within tolerance
```

Corners must use the camera's actual:

* position;
* target;
* up vector;
* field of view;
* aspect ratio.

Do not duplicate Playground camera constants inside the lighting feature.

Keep the math testable without an OpenGL context.

---

# 29. Stable cascade fitting

For each frustum slice:

1. compute its eight world-space corners;
2. compute a stable slice center;
3. build a light-aligned view using ray-travel direction;
4. place the shadow camera opposite the ray direction;
5. transform slice corners into light space;
6. compute X/Y bounds;
7. expand depth by configured padding;
8. snap the light-space X/Y center to texel increments;
9. construct an orthographic projection;
10. produce `lightViewProjection`.

The directional entity position is never used.

Use a robust up fallback when the light direction is nearly parallel to world
up.

The existing `Matrix4x4::lookAt` fallback may be reused if its convention
matches the calculation.

## Texel snapping

For fitted width and height:

```text
worldUnitsPerTexelX = fittedWidth / resolution.x
worldUnitsPerTexelY = fittedHeight / resolution.y
```

Snap the light-space X/Y center to those increments.

Test the snapped X/Y center or relevant projection coefficients.

Do not require the complete matrix to remain byte-identical for sub-texel
motion when depth fitting legitimately changes.

Required visual goal:

```text
small camera translations below one shadow texel must not continuously
translate the cascade projection in light-space X/Y
```

---

# 30. Persistent shadow resources

Own one depth-only framebuffer per active cascade.

Create it through:

```cpp
spk::FrameBufferObject(
    spk::FrameBufferObject::depthTextureTarget(
        resolution,
        spk::Texture::Format::Depth24))
```

`Depth32F` is also acceptable if justified.

The current framebuffer path already provides:

```text
sampleable depth texture
nearest filtering
clamp-to-border
border depth represented by 1.0 border color
no color attachment
no mipmaps
```

Keep shadow targets as separate 2D textures for this implementation.

Store them through owning shared handles so old render units survive resource
replacement.

Do not expose raw OpenGL IDs.

---

# 31. Shadow-pass declaration

Register the family:

```cpp
p_passes.registerPassType(
    spk::LightingRenderPasses::DirectionalShadow,
    "spk.lighting.directional_shadow");
```

For every active cascade:

```cpp
const std::string name =
    "DirectionalShadow[" + std::to_string(cascade) + "]";

p_passes.emplacePass<spk::ShadowRenderPass>(
    {
        .type = spk::LightingRenderPasses::DirectionalShadow,
        .scope = p_context.sceneScope,
        .instance = cascade
    },
    spk::SceneRenderPriorities::ShadowBase +
        static_cast<int>(cascade),
    name,
    {
        .debugName = name,
        .target = {
            .ownedFrameBuffer = cascadeTarget,
            .frameBuffer = cascadeTarget.get(),
            .viewport = cascadeTarget->viewport()
        },
        .clear = {
            .depth = 1.0f
        }
    },
    cascade,
    cascadeLightViewProjection);
```

Equivalent construction is acceptable.

At `PassSetup`, append a per-cascade matrix upload command before casters.

Required ordering:

```text
DirectionalShadow[0]
DirectionalShadow[1]
...
MainOpaque
MainTransparent
MainOverlay
```

With no eligible owner, declare no shadow pass.

Do not add a separate `GameEngine::buildShadowRenderUnit()`.

---

# 32. Main-pass lighting upload

Create generic commands/resources that bind:

* lighting header UBO;
* directional-light SSBO;
* point-light SSBO;
* spot-light SSBO;
* main-pass directional-shadow UBO;
* zero through four shadow samplers.

Commands capture immutable shared frame resources.

When shadows are disabled:

* upload `enabled = 0`;
* upload `shadowDirectionalIndex = -1`;
* set cascade count to zero;
* do not treat previously bound maps as active;
* shader execution must skip shadow sampling.

The state is contributed at `PassSetup` before geometry.

Do not upload camera state from the lighting feature.

---

# 33. Shadow texture binding

Bind cascade depth textures to units 8 through 11.

Use sampler names equivalent to:

```glsl
uniform sampler2D uDirectionalShadowMaps[4];
```

or four individually named samplers if the current `SamplerObject` abstraction
does not support sampler arrays cleanly.

Commands must retain owned texture or framebuffer resources.

Do not bind a raw OpenGL texture ID into a snapshot.

For inactive entries:

* bind a safe fallback or leave them unused;
* initialize corresponding matrices and splits;
* guarantee the shader never samples them.

---

# 34. Geometry cast/receive policy

Add to `TextureMeshRenderer3D`:

```cpp
void setCastsShadows(bool p_value);
void setReceivesShadows(bool p_value);

[[nodiscard]] bool castsShadows() const noexcept;
[[nodiscard]] bool receivesShadows() const noexcept;
```

Defaults:

```text
castsShadows    = true
receivesShadows = true
```

Effective ordinary-mesh casting rule:

```text
renderer.castsShadows() && renderer.translucent() == false
```

Thus translucent ordinary meshes never cast in this implementation, even when
their stored cast preference is true.

For voxel chunks:

```text
opaque mesh:
    casts
    receives

transparent mesh:
    does not cast
    receives
```

Water must not cast a solid opaque shadow.

`receivesShadows == false` affects only shadow attenuation. It does not disable
direct light or ambient light.

---

# 35. Geometry collection and one-frame consistency

Each component logic is collected exactly once per frame.

For each visible renderer, prepare one render snapshot containing:

* shared mesh handle;
* immutable model UBO handle;
* sampler or texture state;
* translucency;
* receive-shadow flag;
* cast-shadow eligibility.

The same prepared snapshot is then used to create:

* the main-pass draw command;
* one shadow draw command per declared cascade when eligible.

Do not reread renderer components or entity transforms per cascade.

Do not rebuild voxel meshes because the sun moved.

---

# 36. Shadow-caster contribution

In `_executeRender`, query typed passes:

```cpp
for (spk::ShadowRenderPass &shadow :
     p_context.frame.passes
        .passesOfType<spk::ShadowRenderPass>(
            spk::LightingRenderPasses::DirectionalShadow,
            p_context.sceneScope))
{
    // add eligible prepared caster commands
}
```

Use:

```cpp
renderPriority(
    spk::LightingRenderPasses::DirectionalShadow)
```

independently of main-pass priority.

Contribute:

* opaque voxel chunk meshes;
* opaque ordinary texture meshes with effective casting enabled.

Do not contribute:

* transparent voxel meshes;
* translucent ordinary meshes;
* inactive or non-processable renderers;
* missing or empty meshes.

`cascadeIndex()` and `lightViewProjection()` come from `ShadowRenderPass`.

Do not create a shadow-specific component-logic registry.

---

# 37. Shadow draw commands

Add commands equivalent to:

```text
DrawVoxelShadowRenderCommand
DrawTextureMeshShadowRenderCommand
```

under:

```text
structures/graphics/rendering/command
```

They capture:

* shared mesh;
* immutable model UBO.

The light matrix is pass-wide state, uploaded at `PassSetup`.

Shadow vertex shader behavior:

```glsl
gl_Position =
    uLightViewProjection *
    uModel *
    vec4(inLocalPosition, 1.0);
```

A fragment shader may be empty where required by the OpenGL shader pipeline.

Required state:

* culling enabled;
* back faces culled;
* CCW front face;
* depth test enabled;
* depth writes enabled;
* blending disabled;
* depth-only target used.

Prefer not to modify color write masks because the target has no color
attachment.

If polygon offset or color-mask state is changed, extend
`OpenGLDrawStateGuard` to snapshot and restore:

```text
GL_COLOR_WRITEMASK
GL_POLYGON_OFFSET_FILL
GL_POLYGON_OFFSET_FACTOR
GL_POLYGON_OFFSET_UNITS
```

Do not rely on the current guard for state it does not preserve.

---

# 38. Main vertex shaders

Update:

```text
resources/shaders/mesh_3d/draw_voxel_mesh_3d.vert
resources/shaders/mesh_3d/draw_texture_mesh_3d.vert
```

to output world position.

Equivalent behavior:

```glsl
vec4 worldPosition =
    uModel * vec4(inLocalPosition, 1.0);

gl_Position =
    uViewProjection * worldPosition;

vertexWorldPosition =
    worldPosition.xyz;

vertexNormal =
    normalize(
        transpose(inverse(mat3(uModel))) *
        inLocalNormal);
```

Both render paths use world-space lighting.

Do not derive local-light vectors from local positions.

---

# 39. Main fragment-shader lighting

Update both main 3D fragment shaders.

Conceptual model:

```text
lighting = environment ambient

for every selected directional light:
    contribution =
        Lambert directional diffuse

    when this directional index equals shadowDirectionalIndex:
        contribution *= directional shadow visibility

for every selected point light:
    contribution =
        Lambert diffuse * finite-range attenuation

for every selected spot light:
    contribution =
        Lambert diffuse * finite-range attenuation * cone factor
```

Only the selected shadow owner receives shadow attenuation.

Other directional lights remain unshadowed.

Preserve:

* texture sampling;
* tint multiplication;
* opaque alpha behavior;
* translucent alpha behavior.

Ambient is never multiplied by visibility.

`receivesShadows == false` forces visibility to one for that draw.

Do not implement specular BRDFs.

---

# 40. Cascade selection in GLSL

For a fragment receiving shadows:

1. compute positive camera-view depth using `uView`;
2. if depth exceeds maximum shadow distance, return visibility one;
3. find the first active cascade whose far distance contains the fragment;
4. transform world position with that cascade matrix;
5. divide by `w`;
6. convert clip coordinates to `[0, 1]`;
7. treat coordinates outside the map as lit;
8. perform depth comparison;
9. return visibility in `[0, 1]`.

Do not sample when:

```text
shadow enabled == false
shadowDirectionalIndex < 0
cascade count == 0
receivesShadows == false
view depth > maximum shadow distance
```

Do not interpret the depth attachment as a color image.

---

# 41. Bias and PCF

Receiver bias:

```glsl
float lambert =
    max(dot(normal, -lightDirection), 0.0);

float bias =
    max(
        constantBias,
        normalBias * (1.0 - lambert));
```

Equivalent stable bias is acceptable.

PCF:

* radius zero performs one comparison;
* radius one performs a 3 x 3 kernel;
* offsets use `textureSize` of the selected depth texture;
* no division by zero;
* outside-map samples count as lit;
* visibility multiplies only direct contribution from the selected sun.

Polygon offset may supplement receiver bias, but does not replace it.

---

# 42. Cascade transition

Blend near cascade boundaries using `transitionFraction`.

For cascade `i`, define a transition interval near its far split based on that
cascade's depth range.

Within the interval:

* sample current cascade;
* sample next cascade;
* blend visibility.

Do not sample beyond the next active cascade.

The final cascade does not blend to an inactive map.

When transition fraction is zero, use strict non-blended selection.

---

# 43. Playground sun entity

Add to `pg::GameSceneWidget`:

```cpp
spk::Entity3D _sunEntity;
spk::Light3D *_sun = nullptr;
spk::SceneLightingRenderFeature *_lighting = nullptr;
```

During scene construction:

1. register `SceneLightingRenderFeature`;
2. add `Light3D` to `_sunEntity`;
3. configure it as directional;
4. choose initial color and intensity;
5. set `castsShadows = true`;
6. assign a high deterministic priority;
7. rotate the entity so local `-Z` points downward into the world;
8. register `_sunEntity` with the same `GameEngine`.

Entity position may be used for editor or debug presentation only. It must not
affect lighting or shadow fitting.

Remove all dependence on the old hard-coded light.

---

# 44. Playground time-of-day component and logic

Add Playground-specific types under its existing component/logic organization.

A suitable design is:

```cpp
class SunController : public spk::Component
{
public:
    void setTimeOfDay(float p_hours);
    void setDayDuration(float p_seconds);
    void setPaused(bool p_paused);
    void setAutomaticCycle(bool p_enabled);

    [[nodiscard]] float timeOfDay() const noexcept;
    [[nodiscard]] float dayDuration() const noexcept;
    [[nodiscard]] bool paused() const noexcept;
    [[nodiscard]] bool automaticCycle() const noexcept;
};
```

Attach it to the sun entity.

Add:

```cpp
class SunControllerLogic
    : public spk::ComponentLogic<pg::SunController>
```

The logic may receive a reference to the lighting feature for ambient editing.

Required behavior:

* finite time values;
* deterministic wrap into `[0, 24)`;
* finite strictly positive day duration;
* paused/manual mode permits direct editing;
* automatic mode advances from update delta time;
* noon places the sun high;
* sunrise and sunset lower intensity and warm the color;
* below-horizon intensity is zero;
* zero intensity naturally removes the sun from shadow-owner eligibility;
* the component is not destroyed at night;
* no OpenGL code exists in the controller or logic.

Use an explicitly documented artistic curve. Astronomical simulation is not
required.

---

# 45. Playground debug controls and diagnostics

Extend the existing F7 overlay.

Display at least:

```text
time of day
sun world direction
sun intensity
shadow owner present
active cascade count
selected directional count
selected point count
selected spot count
discarded directional count
discarded point count
discarded spot count
```

Add controls for:

* toggling automatic time progression;
* pausing;
* increasing and decreasing time manually;
* toggling shadow debug visualization.

Choose available Sparkle key values that do not conflict with existing
Playground behavior, then document the final mapping.

Add one disabled-by-default shader debug mode:

```text
cascade color visualization
```

A fragment receiving shadows may tint its final color by selected cascade for
debugging.

The normal game must not depend on the overlay being active.

---

# 46. Voxel behavior

Use existing baked `VoxelMesh3D` geometry.

Required behavior:

* opaque chunk mesh casts and receives;
* transparent chunk mesh receives but does not cast;
* unloaded or inactive chunks contribute nothing;
* dirty chunk synchronization completes before render snapshots capture meshes;
* shadow commands retain the same shared opaque mesh version captured for the
  frame;
* loading and unloading cannot leave dangling pointers;
* no voxel DDA is used for raster shadows;
* no second shadow-specific voxel mesh is baked;
* moving the sun does not rebake chunks;
* voxel golden-mesh outputs remain byte-for-byte unchanged.

Do not change mesher geometry for this task.

---

# 47. Ordinary mesh behavior

For `TextureMeshRenderer3D`:

* opaque renderers cast and receive by default;
* translucent renderers receive but never cast in this implementation;
* `castsShadows == false` prevents casting;
* `receivesShadows == false` bypasses shadow attenuation;
* parent transforms affect main and shadow model matrices identically;
* missing mesh or texture remains safe;
* an opaque geometric shadow does not require sampling the color texture.

Do not create a second actor mesh representation.

---

# 48. Required component tests

Add GoogleTest coverage for:

* `Component3D` accepts `Entity3D`;
* rejects plain `Entity`;
* rejects `Entity2D`;
* `transform()` returns the entity's owned transform;
* `Light3D` defaults;
* directional, point, and spot switching;
* valid intensity, range, and cone edits;
* negative and non-finite intensity rejection;
* zero, negative, and non-finite range rejection;
* invalid cone ordering;
* outer angle at or above 90 degrees;
* non-finite angles;
* color validation;
* cast-shadow edits;
* priority edits;
* failed setters preserve old state;
* component activation affects processability;
* ancestor activation affects processability.

---

# 49. Required transform tests

Test:

* identity `worldForward()` is `(0, 0, -1)`;
* local rotation changes forward correctly;
* parent rotation changes child forward;
* parent translation changes child world position;
* translation does not change forward;
* local non-uniform scale does not change forward;
* parent non-uniform scale does not skew forward;
* `transformVector` is affected by scale but not translation;
* `worldPosition()` equals `transformPoint(origin)`;
* world position matches the model matrix point transform;
* zero scale leaves valid world rotation and forward available;
* singular inverse-model behavior remains deterministic;
* invalid quaternion input fails deterministically;
* directional entity translation does not change collected direction;
* spot collection uses both world position and world forward.

Use explicit approximate tolerances.

---

# 50. Required collection tests

Test `prepareFrame` without OpenGL where possible.

Cover:

* no-light frame;
* ambient-only frame;
* one light of each kind;
* inactive component exclusion;
* inactive ancestor exclusion;
* zero-intensity exclusion;
* correct packed world position;
* correct packed world direction;
* angle cosine packing;
* bounded directional list;
* bounded point list;
* bounded spot list;
* priority ordering;
* camera-distance tie-break;
* no-camera UUID fallback;
* stable UUID tie-break;
* deterministic result across insertion orders;
* selected and discarded counts;
* shadow owner selection;
* shadow owner forced into bounded directional list;
* correct final shadow index;
* no stale owner after deactivation;
* point/spot shadow intent ignored by directional selection;
* GPU size/alignment contracts;
* empty buffer handling;
* one collection per frame regardless of cascade count.

---

# 51. Required cascade math tests

Add pure-math tests for:

* one cascade;
* three cascades;
* lambda zero;
* lambda one;
* strict ordering;
* effective far-distance clamp;
* invalid camera projection;
* missing camera;
* frustum corners on requested planes;
* fitted corners inside the light clip volume;
* near-parallel light/up fallback;
* snapped X/Y center stable for sub-texel motion;
* movement beyond a texel advances snapping predictably;
* changed light direction changes fitted matrices;
* zero-intensity sun creates no active cascade data;
* no eligible owner creates no active cascade data;
* initialized unused matrix and split entries;
* transition interval calculations.

Keep these tests independent of OpenGL.

---

# 52. Required render-order tests

Extend existing `scene_render_pipeline_test.cpp` and `render_plan_test.cpp`
patterns.

With the real lighting feature, prove:

```text
DirectionalShadow[0..n]
    framebuffer bind
    depth clear
    PassSetup matrix
    casters

MainOpaque
    main framebuffer bind
    main clear
    camera setup
    lighting setup
    opaque geometry

MainTransparent
    main framebuffer bind
    no clear
    camera setup
    lighting setup
    transparent geometry

MainOverlay
    main framebuffer bind
    no clear
    camera setup
    lighting setup
```

Test:

* zero cascades declare no shadow passes;
* one cascade produces one complete sequence;
* three cascades produce three ordered sequences;
* main framebuffer bind follows the final shadow pass;
* transparent voxel rendering remains after opaque geometry;
* lighting exists with only ordinary meshes;
* lighting exists with only voxels;
* lighting exists with no geometry;
* geometry with no direct lights remains ambient-lit;
* camera upload occurs exactly once per main pass;
* lighting upload occurs exactly once per main pass;
* component light collection occurs once per frame.

---

# 53. Required OpenGL and command tests

Where the test harness supports a context, test:

* every depth-only cascade framebuffer is complete;
* each shadow pass binds the intended target;
* depth-only clear behavior;
* shadow draw enables depth writes;
* shadow draw disables blending;
* cull, depth, blend, color-mask, and polygon-offset state are restored for any
  state the command changes;
* camera UBO matches the new two-matrix layout;
* light UBO and SSBO bindings use named contract values;
* empty light buffers bind safely;
* shadow textures use units 8 through 11;
* main shaders link;
* shadow shaders link;
* disabled shadow state does not act on stale maps;
* resources are independent per `RenderContext`;
* replacement shadow resources do not invalidate a previously compiled unit.

Inspect OpenGL errors after meaningful operations.

Shader compilation alone is not proof of data-layout correctness.

---

# 54. Shader behavior tests and visual validation

Automated or controlled tests should cover where practical:

* upward face lit by downward sun;
* back-facing face has zero directional Lambert term;
* ambient remains in full shadow;
* point light reaches zero at range;
* point light remains finite at its position;
* spot light strongest inside inner cone;
* smooth spot fade between angles;
* zero spot contribution outside outer cone;
* selected sun visibility zero removes only that sun's direct term;
* another directional light remains unshadowed;
* `receivesShadows == false` bypasses attenuation;
* fragments beyond maximum distance are unshadowed;
* texture, tint, and alpha do not regress.

Document actual visual checks for:

* cube shadow direction;
* terrain self-shadowing;
* actor shadow on terrain;
* continuous shadow-direction change while the sun moves;
* acceptable cascade stability during camera movement;
* no severe acne;
* no unacceptable peter-panning;
* acceptable cascade transitions;
* water receives but does not cast;
* sunrise, noon, sunset, and night.

Never claim a visual check, build, or test was performed unless it was actually
performed.

List skipped validation and its reason.

---

# 55. Performance requirements

Required behavior:

* shadow framebuffer resources persist across frames;
* resources recreate only when relevant settings change;
* old resources survive queued frames;
* CPU light-selection vectors reuse capacity where practical;
* light counts are bounded before GPU upload;
* one immutable light snapshot is reused by all main passes;
* one prepared geometry snapshot feeds main and cascade commands;
* shared meshes are reused;
* immutable model UBO handles are reused while unchanged;
* no mesh rebuild occurs because the sun moves;
* no CPU depth readback occurs;
* shadow resolution is independent of window resolution.

Optional after correctness:

* cascade caster culling;
* point/spot frustum culling;
* shadow redraw caching.

Do not add caching that can produce stale shadows before the uncached
implementation is correct and tested.

---

# 56. Suggested Sparkle additions

Suggested files include:

```text
includes/structures/game_engine/spk_component_3d.hpp
srcs/structures/game_engine/spk_component_3d.cpp

includes/structures/game_engine/spk_light_3d.hpp
srcs/structures/game_engine/spk_light_3d.cpp

includes/structures/game_engine/rendering/spk_scene_lighting_render_feature.hpp
srcs/structures/game_engine/rendering/spk_scene_lighting_render_feature.cpp

includes/structures/game_engine/rendering/spk_directional_cascade_math.hpp
srcs/structures/game_engine/rendering/spk_directional_cascade_math.cpp

includes/structures/graphics/lighting/spk_environment_lighting.hpp
includes/structures/graphics/lighting/spk_light_gpu_data.hpp
includes/structures/graphics/lighting/spk_directional_shadow_settings.hpp
includes/structures/graphics/lighting/spk_directional_shadow_gpu_data.hpp

includes/structures/graphics/rendering/spk_scene_gpu_bindings.hpp

includes/structures/graphics/rendering/command/spk_scene_lighting_bind_render_command.hpp
srcs/structures/graphics/rendering/command/spk_scene_lighting_bind_render_command.cpp

includes/structures/graphics/rendering/command/spk_directional_shadow_pass_update_render_command.hpp
srcs/structures/graphics/rendering/command/spk_directional_shadow_pass_update_render_command.cpp

includes/structures/graphics/rendering/command/spk_draw_voxel_shadow_render_command.hpp
srcs/structures/graphics/rendering/command/spk_draw_voxel_shadow_render_command.cpp

includes/structures/graphics/rendering/command/spk_draw_texture_mesh_shadow_render_command.hpp
srcs/structures/graphics/rendering/command/spk_draw_texture_mesh_shadow_render_command.cpp

resources/shaders/shadow/draw_voxel_shadow.vert
resources/shaders/shadow/draw_voxel_shadow.frag
resources/shaders/shadow/draw_texture_mesh_shadow.vert
resources/shaders/shadow/draw_texture_mesh_shadow.frag
```

Equivalent cohesive organization is acceptable.

Avoid one-file wrappers with no clear responsibility.

---

# 57. Expected Sparkle modifications

Inspect and update at least:

```text
includes/structures/game_engine/spk_transform_3d.hpp
srcs/structures/game_engine/spk_transform_3d.cpp

includes/structures/game_engine/spk_camera_3d.hpp
srcs/structures/game_engine/spk_camera_3d.cpp

includes/structures/game_engine/rendering/spk_scene_render_pipeline.hpp
srcs/structures/game_engine/rendering/spk_scene_render_pipeline.cpp

includes/structures/game_engine/rendering/spk_shadow_render_pass.hpp

includes/structures/game_engine/spk_texture_mesh_renderer_3d.hpp
includes/structures/game_engine/spk_texture_mesh_render_logic.hpp

includes/structures/voxel/spk_voxel_chunk_render_logic.hpp
srcs/structures/voxel/spk_voxel_chunk_render_logic.cpp

includes/structures/voxel/spk_voxel_chunk_transparent_render_logic.hpp
srcs/structures/voxel/spk_voxel_chunk_transparent_render_logic.cpp

includes/structures/graphics/rendering/pass/spk_render_target_reference.hpp
includes/structures/graphics/rendering/command/spk_use_framebuffer_render_command.hpp
srcs/structures/graphics/rendering/command/spk_use_framebuffer_render_command.cpp

includes/structures/graphics/rendering/command/spk_camera_update_render_command.hpp
srcs/structures/graphics/rendering/command/spk_camera_update_render_command.cpp

includes/structures/graphics/rendering/command/spk_opengl_draw_state_guard.hpp

resources/shaders/mesh_3d/draw_voxel_mesh_3d.vert
resources/shaders/mesh_3d/draw_voxel_mesh_3d.frag
resources/shaders/mesh_3d/draw_texture_mesh_3d.vert
resources/shaders/mesh_3d/draw_texture_mesh_3d.frag

docs/render_pipeline.md
```

Verify `GameEngineWidget`; do not add lighting-specific logic to it.

Sparkle source and resource collection currently uses the project's CMake
source organization. Verify that every new source, test, and shader is included
in its intended target and generated-resource output.

---

# 58. Expected Playground modifications

Inspect and update at least:

```text
Playground/srcs/game_scene_widget.hpp
Playground/srcs/game_scene_widget.cpp

Playground/srcs/components/
Playground/srcs/logics/

Playground/docs/03-systems/rendering-cameras.md
Playground/docs/diagrams/02-class-rendering-3d.puml

Playground/tests/
```

The existing rendering documentation is stale:

* it still describes a fixed directional light;
* it attributes frame-state ownership to geometry logics rather than the scene
  pipeline.

Rewrite it against the final architecture.

---

# 59. Documentation requirements

Document:

* why `Light3D` is one concrete component;
* why position and orientation come from `Entity3D`;
* difference between `transformVector` and `worldRotation`;
* local `-Z` ray-travel convention;
* scene data versus GPU data;
* environment ambient ownership;
* binding points and GLSL layouts;
* bounded deterministic selection;
* no-camera selection fallback;
* forced shadow-owner inclusion;
* directional-shadow-owner index;
* point/spot attenuation;
* spot angle units and cone definition;
* why point/spot shadows are deferred;
* split calculation;
* frustum fitting;
* texel snapping;
* cascade transition behavior;
* cast/receive policy;
* translucent limitations;
* shadow pass identity and priority;
* immutable frame-resource ownership;
* owned framebuffer lifetime;
* Playground time-of-day behavior;
* debug controls.

Do not describe unimplemented point-light shadows, emissive materials, or
astronomical simulation as implemented.

---

# 60. Non-goals

Do not implement:

* point-light cube shadow maps;
* spot-light shadow maps;
* omnidirectional atlases;
* deferred rendering;
* Forward+ or clustered lighting;
* physically accurate photometric units;
* specular BRDFs;
* normal mapping;
* image-based lighting;
* skyboxes;
* atmospheric scattering;
* global illumination;
* voxel light propagation;
* baked lightmaps;
* SSAO;
* transparent colored shadows;
* alpha-tested vegetation shadows unless an existing material mode already
  distinguishes cutout from blend;
* one light per lava voxel;
* latitude, longitude, season, or astronomical date simulation;
* a scene editor;
* light serialization;
* network synchronization;
* a new semantic render-phase abstraction.

---

# 61. Acceptance criteria

The task is complete when:

* `Component3D` enforces `Entity3D` attachment;
* `Light3D` is editable and type-safe;
* all authoring values are validated;
* world position uses the composed model transform;
* world light direction uses explicit composed rotation and ignores scale;
* local `-Z` is consistently ray-travel direction;
* environment ambient is separate;
* camera GPU data contains both view-projection and view matrices;
* all GPU bindings use named constants;
* packed layouts match GLSL;
* light collection occurs once per frame;
* lists are bounded and deterministic;
* no-camera tie-breaking is deterministic;
* the shadow owner is guaranteed to be in the directional GPU list;
* one immutable frame resource is shared by main passes;
* no command retains a light component or reads transforms during execution;
* queued units retain framebuffer resources;
* queued units do not observe later model-UBO mutation;
* point and spot diffuse lighting works in both main render paths;
* zero through four directional cascades work;
* cascade math is headlessly tested;
* cascade X/Y fitting is texel-snapped;
* shadow maps use depth-only `FrameBufferObject` resources;
* opaque voxel chunks cast and receive;
* transparent voxel chunks receive but do not cast;
* opaque ordinary meshes cast and receive by default;
* translucent ordinary meshes receive but do not cast;
* bias and PCF are configurable;
* cascade transition behavior is implemented;
* shadow passes precede main passes through the existing bucket pack;
* no manual framebuffer restoration exists;
* camera and lighting state are each uploaded exactly once per main pass;
* the hard-coded light and obsolete placeholder types are removed;
* Playground owns and registers a sun entity;
* Playground can edit and animate time of day;
* debug diagnostics expose selection and cascade information;
* voxel golden-mesh output does not change;
* all existing and new tests pass;
* complete Sparkle and Playground builds pass.

At the end, report:

1. created files;
2. modified files;
3. removed files;
4. final `Light3D` model;
5. final transform-direction model;
6. final camera and GPU binding layouts;
7. immutable frame-resource strategy;
8. render-target lifetime strategy;
9. deterministic light-selection algorithm;
10. cascade configuration and math;
11. remaining intentional limitations;
12. Playground controls;
13. exact build commands and results;
14. exact automated test results;
15. actual visual validations;
16. skipped validation and reasons.

Do not claim success for commands or checks that were not executed.
