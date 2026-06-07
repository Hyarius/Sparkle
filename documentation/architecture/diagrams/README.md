# Sparkle Architecture Diagrams

This folder contains focused PlantUML diagrams for the currently implemented Sparkle library.

## Class Diagrams

- `application-windowing-class.puml`: compact overview of application, registry, handles, windows, host, platform, and GPU runtime relationships.
- `window-actions-class.puml`: window event queues, platform/render actions, closure notifications, and disposal readiness.
- `window-host-threading-class.puml`: `WindowHost` platform-thread and render-thread ownership, lazy render-context creation, and surface invalidation.
- `window-modules-class.puml`: window modules and how they bind input, update, render, and frame behavior to the root widget.
- `widget-tree-class.puml`: widget inheritance, activation, and render/update/event extension points.
- `resource-loading-class.puml`: texture, image, sprite sheet, font, atlas, glyph, and resource synchronization relationships.
- `mesh-rendering-class.puml`: generic mesh storage, 2D mesh variants, texture mesh behavior, and draw-command consumers.
- `platform-rendering-class.puml`: frame, surface state, render context, render commands, and GPU runtime abstractions.
- `render-commands-class.puml`: high-level drawing commands, concrete OpenGL commands, mesh/texture/font dependencies, and GPU resource wrappers.
- `events-input-class.puml`: split frame/mouse/keyboard event record variants, typed event views, and input state devices.
- `binary-object-class.puml`: binary field byte views, explicit sections, and assignment constraints.
- `traits-and-utilities-class.puml`: reusable traits and utility templates.
- `time-class.puml`: duration, timestamp, chronometer, timer, and time utilities.
- `test-architecture.puml`: GoogleTest organization, OpenGL test context, visual comparison helpers, and expected image resources.
- `public-api-surface-class.puml`: the umbrella include surface exposed by `sparkle.hpp`, grouped by subsystem.

## Sequence Diagrams

- `threading-model-overview.puml`: owner/platform thread, update thread, render thread, and explicit handoff points.
- `application-run-sequence.puml`: `Application::run()` and the three runtime loops.
- `window-event-routing-sequence.puml`: platform event reception into family queues, update-thread processing, modules, and widgets.
- `widget-event-propagation-sequence.puml`: child-first widget event propagation, inactive subtree skips, and consumed-event stops.
- `render-snapshot-rebuild-sequence.puml`: update-time snapshot rebuild from root viewport preparation through widget render unit collection.
- `window-render-sequence.puml`: render snapshots, render actions, context binding, command execution, and presentation.
- `window-closure-sequence.puml`: closure request, validation, destruction notification, and resource release.
- `platform-winapi-event-translation-sequence.puml`: WinAPI message handling into Sparkle frame, mouse, and keyboard records.
- `opengl-context-resource-lifecycle-sequence.puml`: lazy render-context creation, resize notification, invalidation, and release.
- `render-command-execution-sequence.puml`: snapshot execution through render units, render commands, and OpenGL wrappers.
- `binary-layout-sequence.puml`: binary field section creation, lookup, assignment, and byte access.
- `contract-provider-sequence.puml`: subscription, triggering, blocking, delayed flush, and contract resignation.
