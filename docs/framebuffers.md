# Framebuffer render targets

`spk::FrameBufferObject` describes an off-screen target with independently
optional color and depth attachments. Sparkle currently supports one color
texture, a `Depth24Stencil8` depth/stencil renderbuffer, or a sampleable
`Depth24`/`Depth32F` depth texture. Multiple color attachments, multisampling,
and externally owned attachments are not supported.

A renderbuffer is attachment storage used only while rasterizing. A texture is
also an attachment, but can later be sampled by a shader. Shadow mapping needs
texture-backed depth because the scene pass must compare its fragments with the
depth written by the shadow pass.

The common descriptions are:

```cpp
auto gameTarget = spk::FrameBufferObject::colorTarget(size);
// RGBA texture + Depth24Stencil8 renderbuffer

auto shadowTarget = spk::FrameBufferObject::depthTextureTarget(
	{2048, 2048}, spk::Texture::Format::Depth24);
// no color attachment + sampleable depth texture
```

The compatibility constructor `FrameBufferObject(size, withDepth)` remains.
`false` maps to an RGBA texture only; `true` maps to an RGBA texture plus a
`Depth24Stencil8` renderbuffer. New code should pass a description or use a
named factory.

Zero-size targets are valid logical objects but have no complete OpenGL
framebuffer until resized to non-zero dimensions. An effective resize updates
the surface rectangle, viewport and scissor, republishes every texture
attachment's GPU-storage metadata, and advances the framebuffer version once.
Each render context lazily replaces its stale framebuffer, textures, and
renderbuffers; OpenGL names are never stored in shared logical state.

`tryColorAttachment()` and `tryDepthAttachment()` return null when no matching
texture exists. A depth renderbuffer still makes `hasDepthAttachment()` true,
but `tryDepthAttachment()` returns null because renderbuffers cannot be sampled.
The reference accessors throw when the requested texture attachment is absent.

Render-target textures contain allocation metadata rather than a fake CPU pixel
buffer. Calling `pixels()` or `saveAsPng()` on one throws. `Depth24`, `Depth32F`,
and `Depth24Stencil8` are render-target formats and are not PNG-exportable CPU
image formats.
