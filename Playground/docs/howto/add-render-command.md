# How-to — Add a RenderCommand + shader

Working example to copy: `pg::MeshRenderCommand`
(`Playground/srcs/rendering/mesh_render_command.*`) + `resources/shaders/mesh/`.

## 1. Shader files

`Playground/resources/shaders/<name>/<name>.vert` / `.frag` (GLSL `#version 430 core`).
Reserved UBO bindings: **1** camera view-projection (`CameraData`), **2** per-draw model
matrix (`ModelData`), **3** lighting. Pick 4+ for new blocks and document them here.

## 2. The command

```cpp
class MyRenderCommand : public spk::RenderCommand
{
public:
    MyRenderCommand(/* everything the draw needs — write-once, ctor-injected */);
    void execute(spk::RenderContext &p_renderContext) override;
private:
    [[nodiscard]] static spk::Program &_program();   // lazy-loaded static, keyed on shader files
    // UBOs, sampler, mesh refs…
};
```

Rules proven by the existing command:
- Load the `spk::Program` lazily in a static accessor (one GL program per command type),
  sources read from `PG_RESOURCE_DIR "/shaders/…"`.
- The command owns its per-draw GPU objects (`spk::UniformBufferObject`,
  `spk::SamplerObject`); upload in `execute`.
- **Restore GL state** you change (depth test, culling, blend): query-save → set → draw →
  restore. Commands must be order-independent w.r.t. state.
- Camera VP is *not* your business: an upstream `spk::CameraUpdateRenderCommand` (emitted by
  the render logic each pass) fills binding 1 (D04).

## 3. Emission

From a ComponentLogic's `_executeRender`:

```cpp
p_builder.emplace<spk::CameraUpdateRenderCommand>(1, camera.viewProjectionMatrix()); // once per pass
p_builder.emplace<pg::MyRenderCommand>(args...);        // or pre-built: p_builder.add(std::move(cmd))
```

Order within the builder = draw order. Project pass order: clear (widget-level) → opaque
world → translucent (overlay/flora, depth-write off) → HUD widgets (outside the engine).

## 4. Verify

`[build]` + `[run]`; GL errors land on stderr. Visual result: hand-validated
([build-and-run.md](build-and-run.md) smoke-run convention).
