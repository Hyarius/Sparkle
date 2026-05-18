## Threading / Render Ownership

| Priority | Item | Source / Files | Notes |
|---:|---|---|---|
| 200 | **Fix `notifyFrameResized()` thread ownership** | `spk_window.cpp:236`, `spk_window.cpp:406`, `spk_window_host.cpp:127-143` | `Window::render()` runs on the render loop and calls `_host.notifyFrameResized(...)` as a render action, but `notifyFrameResized()` currently validates platform-thread ownership. This can throw in the real application loop after resize once rendering is on its own thread. |
| 200 | **Prevent resize handling from binding the render thread to the platform thread** | `spk_window_host.cpp:127-143` | `notifyFrameResized()` calls `_ensureRenderContextLocked()`, which can call `_bindOrValidateRenderThreadLocked()`. If resize happens before the real render thread calls `makeCurrent()`, the render thread ID can be incorrectly bound to the platform thread. |
| 200 | **Add render-thread validation to `WindowHost::present()`** | `spk_window_host.cpp:258` | `present()` currently checks only that a context exists and is valid before calling `present()`. With OpenGL, presenting from the wrong thread is a backend bug. |
| 150 | **Normalize render-thread checks across render-context methods** | `spk_window_host.cpp` | `makeCurrent()`, `renderContext()`, `setVSync()`, `notifyFrameResized()`, and `present()` should have consistent ownership validation. Clearly separate platform-thread operations from render-thread operations. |
| 150 | **Add resize ordering tests** | Window/runtime tests | Add tests covering the scenario where resize notification happens before `makeCurrent()` has bound the real render thread. Also test resize after render-thread binding. |

---

## Backend / Runtime Architecture

| Priority | Item | Source / Files | Notes |
|---:|---|---|---|
| 150 | **Make GPU/runtime pairing explicit and fail early** | `spk_gpu_platform_runtime.hpp:16` | GPU runtimes currently inspect and downcast concrete frame types. This means mismatched `IPlatformRuntime` / `IGPUPlatformRuntime` pairs fail late at runtime. Add explicit diagnostics, for example: `OpenGLWin32Runtime requires Win32Frame`. |
| 150 | **Decide on native handle access strategy for backend frames** | Platform/GPU runtime interfaces | Either expose typed native handle accessors on backend-specific frames, or keep `dynamic_cast` but guard it clearly with meaningful errors. This should be clarified before adding Win32 + OpenGL. |
| 100 | **Move OpenGL/GLEW dependencies into backend-specific CMake targets** | `CMakeLists.txt:62`, `CMakeLists.txt:80` | The core target currently finds GLEW/OpenGL globally and links them `PUBLIC`, causing every consumer to inherit OpenGL/GLEW before the backend exists. Prefer a backend target, or at least `PRIVATE` linkage unless public headers expose those APIs. |
| 100 | **Update default runtime factory tests when backend defaults exist** | `spk_application.cpp:12-17`, `ApplicationTest::DefaultCreateWindowThrowsWhenNoDefaultRuntimeExists` | `_createDefaultPlatformRuntime()` and `_createDefaultGPUPlatformRuntime()` currently throw by design. Once Win32/OpenGL defaults are implemented, the test should be updated or conditioned on platform/backend availability. |

---

## Public API Cleanup

| Priority | Item | Source / Files | Notes |
|---:|---|---|---|
| 100 | **Decide whether `sparkle.hpp` is the public umbrella header** | `sparkle.hpp:1` | The file currently includes only traits/utilities. If it is intended as the public entry point, it should expose the application/window/runtime API. |
| 100 | **Expose main runtime/window headers through the umbrella header if appropriate** | `sparkle.hpp` | If `sparkle.hpp` is the public entry point, include APIs such as `Application`, `Window`, `IPlatformRuntime`, and `IGPUPlatformRuntime`. If not, document the intended public headers. |

---

## Test Infrastructure Cleanup

| Priority | Item | Source / Files | Notes |
|---:|---|---|---|
| 100 | **Replace `#define private public` in tests** | `window_runtime_test.cpp:6`, `application_test.cpp:7` | Tests currently access internals like `_windows`, `_windowRegistry`, and `_isRunning` by redefining `private` as `public`. Replace this with friend test accessors, similar to the existing `WindowAccess`, or a dedicated internal/test inspection header. |
| 50 | **Remove duplicated state from `TestFrame`** | `window_test_utils.hpp:146-284` | `TestFrame` keeps direct fields like `currentRect`, `resizeCount`, etc., plus a parallel `TestFrameStats` struct. Every mutation writes to both, creating divergence risk. Follow the `TestRenderContext` pattern: store state only in the shared stats struct. |
| 50 | **Clarify `ISurfaceState` polymorphic role** | `spk_surface_state.hpp:13`, `window_test_utils.hpp:141-144` | `ISurfaceState` has a virtual destructor and `TestSurfaceState` subclasses it empty. If it is intended as a backend-specific polymorphic extension point, document it. Otherwise, consider simplifying the design and making the concrete state non-polymorphic/final. |
