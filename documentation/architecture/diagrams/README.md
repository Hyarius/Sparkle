# Sparkle Architecture Diagrams

These diagrams are organized as a progressive learning path. Each level covers Sparkle with a different amount of detail, so a reader can start with the broad ideas and later revisit the same systems with more precision.

The level names are intentionally neutral:

| Level | Purpose | Best Used For |
| --- | --- | --- |
| `Level-1` | Simple mental models | First contact, course introductions, quick orientation |
| `Level-2` | Practical usage flows | Learning how to build small Sparkle applications |
| `Level-3` | System collaboration | Understanding how public concepts connect to internals |
| `Level-4` | Implementation detail | Maintenance, debugging, platform/rendering work |

Recommended reading path:

1. Read `Level-1` to learn the vocabulary.
2. Read `Level-2` when writing application code.
3. Read `Level-3` when modifying Sparkle features.
4. Read `Level-4` when debugging lifecycle, threading, platform, rendering, or utility internals.

## Level-1: First Concepts

Small diagrams with minimal vocabulary. These introduce the same subjects that later levels revisit in more detail.

| Topic | Diagram |
| --- | --- |
| Whole library map | `library-map.puml` |
| Program loop | `program-loop-simple.puml` |
| First window | `first-window-flow.puml` |
| Widget tree | `widget-tree-simple.puml` |
| Widget customization | `widget-basics.puml` |
| Input | `input-simple.puml` |
| Rendering | `rendering-simple.puml` |
| Assets | `assets-simple.puml` |
| Platform services | `platform-simple.puml` |
| Math values | `math-simple.puml` |
| Utilities | `utilities-simple.puml` |
| Tests | `testing-simple.puml` |

## Level-2: Application Usage

Diagrams for users who are ready to build with the library. These are still user-facing, but they show real Sparkle workflows.

| Topic | Diagram |
| --- | --- |
| Application lifecycle | `application-lifecycle.puml` |
| Window management | `window-management-flow.puml` |
| Widget layout | `widget-layout-flow.puml` |
| Input to widgets | `input-to-widget-flow.puml` |
| Rendering basics | `rendering-basics-flow.puml` |
| Viewport and clipping | `viewport-and-clipping-flow.puml` |
| Assets and drawing | `assets-and-drawing.puml` |
| Text and fonts | `text-and-font-flow.puml` |
| Meshes and shapes | `mesh-and-shape-flow.puml` |
| Time and update | `time-and-update-flow.puml` |
| Math values | `math-values-flow.puml` |
| Utility patterns | `utility-patterns-flow.puml` |

## Level-3: System Collaboration

Diagrams for understanding how Sparkle subsystems work together internally without going into every low-level edge case.

| Topic | Diagram |
| --- | --- |
| Public API surface | `public-api-surface-class.puml` |
| Application runtime | `application-runtime-class.puml` |
| Threading model | `threading-model-overview.puml` |
| Window registry and handles | `window-registry-handle-class.puml` |
| Window modules | `window-modules-class.puml` |
| Widget tree | `widget-tree-class.puml` |
| Widget event propagation | `widget-event-propagation-sequence.puml` |
| Input state | `input-state-class.puml` |
| Render snapshot rebuild | `render-snapshot-rebuild-sequence.puml` |
| Render context and viewport | `render-context-viewport-class.puml` |
| Render commands | `render-commands-class.puml` |
| Resource loading | `resource-loading-class.puml` |
| Mesh rendering | `mesh-rendering-class.puml` |
| OpenGL runtime | `opengl-runtime-class.puml` |
| OpenGL resources | `opengl-resources-class.puml` |
| WinAPI platform | `winapi-platform-class.puml` |
| Math primitives | `math-primitives-class.puml` |
| Utility primitives | `utility-primitives-class.puml` |
| Test support | `test-support-class.puml` |

## Level-4: Implementation Detail

Precise diagrams for maintainers. These describe lifecycle, ownership, synchronization, platform translation, OpenGL resource behavior, and utility internals.

| Topic | Diagram |
| --- | --- |
| Application run loops | `application-run-sequence.puml` |
| Application and windowing | `application-windowing-class.puml` |
| Window actions | `window-actions-class.puml` |
| Window host threading | `window-host-threading-class.puml` |
| Window registry lifecycle | `window-registry-lifecycle-sequence.puml` |
| Window event routing | `window-event-routing-sequence.puml` |
| Window rendering | `window-render-sequence.puml` |
| Window closure | `window-closure-sequence.puml` |
| Window snapshot manager | `window-snapshot-manager-sequence.puml` |
| Widget update | `widget-update-sequence.puml` |
| Widget geometry | `widget-geometry-sequence.puml` |
| Input records | `events-input-class.puml` |
| Input state update | `input-state-update-sequence.puml` |
| Platform rendering | `platform-rendering-class.puml` |
| WinAPI event translation | `platform-winapi-event-translation-sequence.puml` |
| OpenGL context lifecycle | `opengl-context-resource-lifecycle-sequence.puml` |
| Render command execution | `render-command-execution-sequence.puml` |
| OpenGL buffer resources | `opengl-buffer-resource-class.puml` |
| OpenGL program and uniforms | `opengl-program-uniform-sequence.puml` |
| Mesh buffer upload | `mesh-buffer-upload-sequence.puml` |
| Font atlas build | `font-atlas-build-sequence.puml` |
| Sprite sheet rendering | `sprite-sheet-sequence.puml` |
| Binary field class | `binary-object-class.puml` |
| Binary field layout | `binary-layout-sequence.puml` |
| Contracts | `contract-provider-sequence.puml` |
| Traits and utilities | `traits-and-utilities-class.puml` |
| Utility threading | `utility-threading-class.puml` |
| Observable and cache behavior | `observable-and-cache-sequence.puml` |
| Math value types | `math-value-types-class.puml` |
| Time primitives | `time-class.puml` |
| Test architecture | `test-architecture.puml` |

## Maintenance

Each `.puml` file should have a matching `.png` generated with PlantUML.

From the repository root:

```powershell
plantuml documentation\architecture\diagrams\Level-1\*.puml
plantuml documentation\architecture\diagrams\Level-2\*.puml
plantuml documentation\architecture\diagrams\Level-3\*.puml
plantuml documentation\architecture\diagrams\Level-4\*.puml
```

To validate syntax recursively:

```powershell
Get-ChildItem documentation\architecture\diagrams -Recurse -Filter *.puml | ForEach-Object { plantuml -checksyntax $_.FullName }
```
