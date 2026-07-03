# Step 13 — Promotion: the 3D layer → `spk::`

**Phase E · needs Milestone 1 (step 12) · API approved by the user on 2026-07-03**

## Goal

Lift the proven 3D layer into the Sparkle library, with library-grade tests. This is the
first promotion and the template for later promotions.

## Promoted API

| Playground source | Sparkle API |
|---|---|
| `pg::Transform3D` | `spk::Transform3D` |
| `pg::Entity3D` | `spk::Entity3D` |
| `pg::Camera3D` | `spk::Camera3D` |
| `pg::MeshVertex3D` | `spk::TextureVertex3D` |
| `pg::Mesh3D` | `spk::TextureMesh3D` |
| `pg::MeshRenderer3D` | `spk::TextureMeshRenderer3D` |
| `pg::MeshRenderLogic` | `spk::TextureMeshRenderLogic` |
| `pg::MeshRenderCommand` | `spk::DrawTextureMesh3DRenderCommand` |
| `pg::makeCube` | `spk::PrimitiveObject::CreateCube` |
| Playground mesh shaders | embedded Sparkle shaders in `resources/shaders/mesh_3d/` |

The light upload is promoted as:

```cpp
spk::DirectionalLightUpdateRenderCommand(
	std::size_t p_bindingPoint,
	spk::DirectionalLight p_light);
```

`spk::DirectionalLight` is an aligned, trivially-copyable 48-byte value matching one
`std140` UBO block. It stores values, not references, so the command uploads the complete
block directly. The caller supplies the binding point; the texture-mesh pipeline uses
binding 3, but the command is reusable with shaders that choose another binding.

Stays in `pg::`: chunk rendering, overlay rendering, mouse picking, camera controllers,
and voxel-mesh consumers. These are game-specific behavior built on the promoted API.

## Completed work

1. User reviewed and approved the public API names and directional-light contract.
2. Types, render commands, render logic, cube primitive, and shaders moved into Sparkle.
3. Every Playground consumer now uses the `spk::` API; the duplicate `pg::` files and
   Playground mesh shaders were removed.
4. Transform, camera, mesh, renderer, light layout/binding, render-logic emission, and
   OpenGL state-restoration tests live in Sparkle's test suite.
5. Sparkle and Playground build successfully; automated suites pass.

## Verification

- Sparkle: 1,980/1,980 tests pass.
- Playground: 139/139 tests pass.
- Remaining manual gate: run the Playground and compare the rendered game with the
  Step-12 baseline by eye.

## Definition of Done

`[spk-test]` green · `[test]` green · `[build]` green · API shape confirmed. The final
`[run]` visual comparison remains a user/manual validation because it requires judging the
native game output against the Step-12 baseline.
