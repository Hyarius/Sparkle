# Rendering Code Review - Active Issues Only

Scope:

- `includes/structures/graphics/rendering/command/spk_draw_color_mesh_render_command.hpp`
- `includes/structures/graphics/rendering/command/spk_draw_font_render_command.hpp`
- `includes/structures/graphics/rendering/command/spk_draw_texture_mesh_render_command.hpp`
- Supporting render command wrappers, widget render-unit builders, resource classes,
  shaders, and the update/render snapshot pipeline.

This file only keeps issues that still appear to be active in the current code.

---

## Findings

### H1 - Render snapshots still borrow texture/font resources across threads

The remaining lifetime contract is still dangerous:

| Command | What it owns | What it borrows |
|---|---:|---|
| `DrawTextureMeshRenderCommand` | shared texture mesh | `const Texture&` |
| `DrawFontRenderCommand` | shared texture mesh | `Font::Atlas&` |
| `TextRenderCommand` | baked/rebuilt font draw command | `Font&`, `Font::Atlas&` |
| `ImageRenderCommand` / `SpriteRenderCommand` / `NineSliceRenderCommand` | generated mesh command | texture or sprite sheet by reference |

The application has a render thread and an update thread. `RenderModule::render()`
copies the current `shared_ptr<RenderSnapshot>` under a mutex, then executes it after
the mutex is released. At the same time, `Window::update()` can rebuild and publish a
new snapshot.

Widgets store resources in `std::shared_ptr`, but render commands are built from
dereferenced references:

- `TextLabel::_buildRenderUnit()` passes `*_font`.
- `ImageLabel::_buildRenderUnit()` passes `*_texture`.
- `AnimationLabel::_buildRenderUnit()` and `Panel::_buildRenderUnit()` pass
  `*_spriteSheet`.
- `TextEdit` and `TextArea` pass their font and nine-slice sprite sheet the same way.

If a widget replaces the last owning `shared_ptr` while an older snapshot is still
rendering, that older snapshot can contain commands referencing a destroyed font,
atlas, texture, or sprite sheet.

Recommended fixes:

- Pass and store `std::shared_ptr<const spk::Texture>` or a render-resource handle in
  texture-backed commands.
- Pass and store `std::shared_ptr<spk::Font>` or a shared atlas object in text-backed
  commands. An `Atlas&` is not enough unless the font/atlas lifetime is anchored.
- Update wrapper commands and widget builders so the snapshot owns every render
  resource needed by its commands.
- Add old-snapshot regression tests that replace the last owning texture/font/sprite
  sheet before rendering the previous snapshot.

### H2 - Font/texture CPU data can race between update and render threads

Snapshot construction is not purely local. `TextRenderCommand` calls `loadGlyphs()` and
reads glyph UVs while building its mesh. `Font::Atlas::glyph()` can load a glyph,
resize atlas CPU storage, call `setPixels()`, update `Texture::_pixels` and `_version`,
and trigger edition callbacks.

Because the update loop can build a new snapshot while the render loop executes an old
snapshot, a new text command can mutate the shared `Font::Atlas` while an old command
calls `_atlas.synchronize()` and binds the atlas texture.

`Texture::_pixels`, `_size`, `_format`, properties, and `_version` are not protected by
a mutex or atomics. This is a potential C++ data race, not only a visual ordering
issue.

Recommended fixes:

- Do not mutate `Font::Atlas` or `Texture` CPU data from snapshot construction while
  the render thread can read it.
- Move glyph loading/resource edits onto the render thread, or protect resource CPU
  state with a clear locking/snapshotting scheme.
- Prefer immutable render resources per published snapshot: commands should refer to
  a specific resource version whose upload source cannot be edited concurrently.
- Add a threaded regression test or TSAN run that repeatedly changes text while the
  render loop draws previous snapshots.

### M1 - Viewport UBO binding contract is implicit

The viewport UBO binds at point `0`, and the current shaders work because uniform block
bindings default to `0`. No code calls `glUniformBlockBinding`, and the `#version 330`
shaders cannot use a `binding =` qualifier.

`UniformBufferObject::validateFor()` exists, but the draw-command path does not call it.
It also validates only that some active block is currently bound at the requested
binding point, not that the program exposes the expected `ViewportData` block.

A shader block rename, a second UBO, or a program linked with unexpected block bindings
could fail as a black screen or stale matrix instead of an explicit validation error.

Recommended fixes:

- Explicitly bind the `ViewportData` block to the expected binding point when programs
  are linked or when `Program::gpu()` first builds the per-context program.
- Validate the expected block name and binding for each shared program once per
  context.
- Add a test that executes each draw command after a viewport command and asserts that
  `ViewportData` is bound to the expected binding point.

### L1 - Shader version split: color mesh uses 410, texture/font use 330

`draw_color_mesh.vert/.frag` use `#version 410 core`. Font and texture shaders use
`#version 330 core`.

This raises the minimum GL requirement for the color path only. If the engine target is
OpenGL 3.3, use matching names/interfaces and `#version 330` everywhere.

### L2 - Redundant `_atlas.synchronize()` in the font command

`DrawFontRenderCommand::execute()` calls `_atlas.synchronize()`, but
`SamplerObject::activate()` already resolves `texture()->gpu(p_context)`, which
performs the upload. The texture command relies on the sampler path alone.

Recommended fix:

- Drop `_atlas.synchronize()`, or keep it only if it documents a specific ordering
  requirement and add a comment explaining why.

### L3 - Texture binds are still uncached per draw

The binding cache covers program, VAO, and UBO bases. `SamplerObject::activate()` still
does `glActiveTexture` and `glBindTexture` on every texture/font draw.

If texture rebinding appears in profiles, extend the `BindingCache` pattern to texture
units.

### L4 - Small cleanup items

- `glBlendFunc` in `RenderContext` setup is immediately overridden by
  `glBlendFuncSeparate`.
- The font fragment shader composites fill/outline premultiplied, then un-premultiplies
  to match the straight-alpha blend state. Premultiplied output would be cheaper and
  more precise, but requires compatible blend-state management.
- `Atlas::glyph()` uploads the whole atlas per missing glyph inside `loadGlyphs`; batch
  missing glyph loading and upload once.
- `outlineThickness` is not clamped. A large outline relative to padding can push
  `outlineEdge` negative; this visually saturates but is unguarded.
- The SDF constants (`128`, padding slack, shader threshold mapping) should live in one
  shared location to avoid drift.

---

## Suggested Priority

1. Resolve remaining resource lifetime ownership for texture/font/sprite-backed
   commands.
2. Eliminate or protect update/render CPU-data races around `Font::Atlas` and
   `Texture`.
3. Pin viewport UBO bindings explicitly and validate them per program/context.
4. Unify shader versions around the intended OpenGL baseline.
5. Clean up low-priority GL and font-path inefficiencies if profiling or maintenance
   pressure justifies it.

---

## Highest-Value Tests To Add

1. Render a texture label, publish a snapshot, replace the widget texture with a new
   last-owner `shared_ptr`, then render the old snapshot and verify no crash or
   use-after-free.
2. Render a text label, publish a snapshot, replace the widget font with a new
   last-owner `shared_ptr`, then render the old snapshot and verify no crash or
   use-after-free.
3. Render a sprite/nine-slice widget, replace the last owning sprite sheet, then render
   the old snapshot.
4. Stress text changes while rendering on the real update/render loop, ideally under
   ThreadSanitizer.
5. Execute texture/font/color draw commands without a preceding `ViewportCommand` in a
   test harness and verify the failure mode is explicit rather than stale UBO state.
