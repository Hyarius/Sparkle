## Backend / Runtime Architecture

| Priority | Item | Source / Files | Notes |
|---:|---|---|---|
| 100 | **Update default runtime factory tests when backend defaults exist** | `spk_application.cpp:12-17`, `ApplicationTest::DefaultCreateWindowThrowsWhenNoDefaultRuntimeExists` | `_createDefaultPlatformRuntime()` and `_createDefaultGPUPlatformRuntime()` currently throw by design. Once Win32/OpenGL defaults are implemented, the test should be updated or conditioned on platform/backend availability. |

---

## Test Infrastructure Cleanup

| Priority | Item | Source / Files | Notes |
|---:|---|---|---|
| 100 | **Replace `#define private public` in tests** | `window_runtime_test.cpp:6`, `application_test.cpp:7` | Tests currently access internals like `_windows`, `_windowRegistry`, and `_isRunning` by redefining `private` as `public`. Replace this with friend test accessors, similar to the existing `WindowAccess`, or a dedicated internal/test inspection header. |
| 50 | **Remove duplicated state from `TestFrame`** | `window_test_utils.hpp:146-284` | `TestFrame` keeps direct fields like `currentRect`, `resizeCount`, etc., plus a parallel `TestFrameStats` struct. Every mutation writes to both, creating divergence risk. Follow the `TestRenderContext` pattern: store state only in the shared stats struct. |
| 50 | **Clarify `ISurfaceState` polymorphic role** | `spk_surface_state.hpp:13`, `window_test_utils.hpp:141-144` | `ISurfaceState` has a virtual destructor and `TestSurfaceState` subclasses it empty. If it is intended as a backend-specific polymorphic extension point, document it. Otherwise, consider simplifying the design and making the concrete state non-polymorphic/final. |
