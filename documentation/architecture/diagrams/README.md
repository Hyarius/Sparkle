# Sparkle Architecture Diagrams

This folder contains focused PlantUML diagrams for the currently implemented Sparkle library.

## Class Diagrams

- `application-windowing-class.puml`: application, registry, window, host, platform, and GPU runtime relationships.
- `window-modules-class.puml`: window modules and how they bind input, update, render, and frame behavior to the root widget.
- `widget-tree-class.puml`: widget inheritance, activation, and render/update/event extension points.
- `platform-rendering-class.puml`: frame, surface state, render context, render commands, and GPU runtime abstractions.
- `events-input-class.puml`: split frame/mouse/keyboard event record variants, typed event views, and input state devices.
- `binary-object-class.puml`: binary object, fields, layout nodes, and synchronization.
- `traits-and-utilities-class.puml`: reusable traits and utility templates.
- `time-class.puml`: duration, timestamp, chronometer, timer, and time utilities.

## Sequence Diagrams

- `application-run-sequence.puml`: `Application::run()` and the three runtime loops.
- `window-event-routing-sequence.puml`: platform event reception into family queues, update-thread processing, modules, and widgets.
- `window-render-sequence.puml`: render snapshots, render actions, context binding, command execution, and presentation.
- `window-closure-sequence.puml`: closure request, validation, destruction notification, and resource release.
- `binary-layout-sequence.puml`: binary schema mutation, lazy synchronization, layout, and buffer access.
- `contract-provider-sequence.puml`: subscription, triggering, blocking, delayed flush, and contract resignation.
