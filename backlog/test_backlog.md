## Image Comparison Utilities

| Priority | Item | Source / Files | Notes |
|---:|---|---|---|
| 180 | **Test tolerance boundary: pixel differing by exactly `rgbTolerance` passes, by `rgbTolerance+1` fails** | `image_comparison_test_utils.cpp:42-44`, `image_comparison_test.cpp` | `channelDiffers` uses `> tolerance`, so diff == tolerance should pass. No test currently exercises the boundary in either direction. |
| 170 | **Test transparent-pixel RGB skip behavior** | `image_comparison_test_utils.cpp:57-59` | When both `actual[3]` and `expected[3]` are `<= transparentAlphaThreshold`, RGB differences are silently ignored. No test covers this. A pixel with mismatched RGB but both alphas = 0 should count as matching. |
| 160 | **Test dimension mismatch case** | `image_comparison_test_utils.cpp:91-125` | When actual and expected have different sizes, out-of-overlap pixels are marked as mismatched and the diff image uses `max(w, h)`. No test exercises this at all. |
| 120 | **Test alpha-only difference is caught** | `image_comparison_test_utils.cpp:52-55` | Two pixels identical in RGB but differing in alpha by more than `alphaTolerance` should register as different. Not currently covered. |
| 100 | **Test that `compareImages` throws on missing file** | `image_comparison_test_utils.cpp:29-32` | `loadImage` throws `std::runtime_error` when the file does not exist. No test verifies this propagates correctly from `compareImages`. |
| 80 | **Test all-pixels-different case** | `image_comparison_test.cpp` | Current test has only 1 out of 4 pixels different. A test where every pixel differs should verify `differentPixelCount == width * height` and `matches == false`. |

---

## OpenGL Screenshot

| Priority | Item | Source / Files | Notes |
|---:|---|---|---|
| 150 | **Test that `saveScreenshot` throws on zero-size rect** | `spk_opengl_runtime.cpp:33-36` | The guard `if (width <= 0 || height <= 0)` throws, but no test exercises it. |
| 130 | **Test the no-argument `saveScreenshot` overload** | `spk_opengl_runtime.cpp:59-71` | `saveScreenshot(path)` reads the current viewport via `glGetIntegerv(GL_VIEWPORT, ...)`. This overload is entirely untested. |
| 90 | **Test `saveScreenshot` throws on unwritable output path** | `spk_opengl_runtime.cpp:53-55` | When `stbi_write_png` fails (bad path, no permission), it should throw. Not currently tested. |

---

## Widget Visual Test Helpers

| Priority | Item | Source / Files | Notes |
|---:|---|---|---|
| 140 | **Test `compareSnapshot` behavior when expected image is missing** | `spk_widget_visual_test_helpers.cpp:138-149` | When the expected PNG does not exist, `compareSnapshot` calls `ADD_FAILURE()` and returns `matches = false` with `differentPixelCount == w*h`. No test verifies this path. |
| 80 | **Test `compareSnapshot` behavior when snapshot does not match expected** | `spk_widget_visual_test_helpers.cpp:151-157` | `ADD_FAILURE()` is called and the result has `matches == false`. No test covers a deliberate mismatch case. |

---

## WinAPI::Class

| Priority | Item | Source / Files | Notes |
|---:|---|---|---|
| 160 | **Test constructor throws on empty name** | `spk_winapi_class.cpp:22-25` | Validated in constructor but never exercised by a test. |
| 160 | **Test constructor throws on null procedure** | `spk_winapi_class.cpp:26-29` | Same as above. |
| 150 | **Test normal construction: `name()` and `instance()` return correct values** | `spk_winapi_class.cpp:91-98` | Happy-path accessors not tested in isolation. |
| 140 | **Test duplicate class name is accepted gracefully** | `spk_winapi_class.cpp:40-47` | `ERROR_CLASS_ALREADY_EXISTS` sets `_isRegistered = false` without throwing. Not tested. |
| 100 | **Test move constructor leaves source in a safe state** | `spk_winapi_class.cpp:52-60` | Moved-from class must not call `UnregisterClassW` in its destructor. Not tested. |
| 100 | **Test move assignment unregisters previous class and adopts new one** | `spk_winapi_class.cpp:70-89` | Move assignment has both destruction and acquisition logic. Not tested. |

---

## WinAPI::Cursor

| Priority | Item | Source / Files | Notes |
|---:|---|---|---|
| 170 | **Test default-constructed cursor: `isValid() == false`** | `spk_winapi_cursor.cpp:13` | Default constructor leaves `_handle = nullptr`. |
| 170 | **Test construction with null handle throws** | `spk_winapi_cursor.cpp:18-22` | Guards against null, but no test exercises it. |
| 160 | **Test `activate()` on empty cursor throws** | `spk_winapi_cursor.cpp:86-90` | Throws `std::runtime_error` when `_handle == nullptr`. Not tested. |
| 150 | **Test all static factory methods return valid cursors** | `spk_winapi_cursor.cpp:60-82` | `arrow()`, `ibeam()`, `hand()`, `cross()`, `wait()` — each should return a cursor where `isValid() == true`. |
| 120 | **Test move constructor: moved-from cursor is no longer valid** | `spk_winapi_cursor.cpp:25-31` | After move, source `_handle` is null and `isValid() == false`. Not tested. |
| 100 | **Test move assignment: owned handle is destroyed, source is cleared** | `spk_winapi_cursor.cpp:41-58` | An owned cursor moved-over should destroy its old handle. Not tested. |

---

## WinAPI::Window

| Priority | Item | Source / Files | Notes |
|---:|---|---|---|
| 160 | **Test construction with null class throws** | `spk_winapi_window.cpp:66-68` | Guards against null but no test covers it. |
| 150 | **Test normal creation: `isValid() == true`, `handle() != nullptr`** | `spk_winapi_window.cpp:54-98` | Happy path not tested in isolation (only tested transitively via Frame). |
| 140 | **Test `title()` returns the title passed at construction** | `spk_winapi_window.cpp:260-273` | Round-trip not tested. |
| 140 | **Test `setTitle()` / `title()` round-trip** | `spk_winapi_window.cpp:168-173` | Mutation not tested. |
| 130 | **Test `rect()` and `clientRect()` return non-empty rects** | `spk_winapi_window.cpp:233-258` | After construction and show, both should return rects with non-zero size. |
| 130 | **Test `destroy()` makes `isValid() == false`** | `spk_winapi_window.cpp:155-166` | Post-destroy state not tested. |
| 120 | **Test move constructor: moved-from is invalid, moved-to is valid** | `spk_winapi_window.cpp:100-111` | After move, GWLP_USERDATA must point to the new owner object. Not tested. |
| 100 | **Test `show()` / `hide()` do not crash** | `spk_winapi_window.cpp:138-153` | Smoke tests for visibility toggling. |
| 100 | **Test `resize()` updates `rect()`** | `spk_winapi_window.cpp:176-208` | Post-resize geometry not tested in isolation. |

---

## WinAPI::Frame

| Priority | Item | Source / Files | Notes |
|---:|---|---|---|
| 160 | **Test frame event callbacks: WM_CLOSE emits `WindowCloseRequestedRecord`** | `spk_winapi_frame.cpp:51-53` | Core event dispatch not unit-tested. Requires posting a message and calling `pollEvents`. |
| 160 | **Test frame event callbacks: WM_DESTROY emits `WindowDestroyedRecord`** | `spk_winapi_frame.cpp:54-56` | Same as above. |
| 150 | **Test mouse event callbacks: move, button down/up, wheel, enter/leave** | `spk_winapi_frame.cpp:82-134` | All mouse message types mapped to records. None tested directly. |
| 150 | **Test keyboard event callbacks: keydown (with repeat), keyup, WM_CHAR** | `spk_winapi_frame.cpp:144-167` | Keyboard mapping and repeat flag logic not tested. |
| 140 | **Test `resize()` updates `rect()`** | `spk_winapi_frame.cpp:207-211` | Frame geometry after resize not tested in isolation. |
| 130 | **Test `setTitle()` / `title()` round-trip** | `spk_winapi_frame.cpp:213-217` | Not tested in isolation. |
| 120 | **Test `validateClosure()` destroys the underlying window** | `spk_winapi_frame.cpp:228-231` | After `validateClosure()`, `handle()` should return null. |
| 110 | **Test mouse-leave tracking is re-armed after re-entry** | `spk_winapi_frame.cpp:83-92` | `_isTrackingMouseLeave` is reset on WM_MOUSELEAVE and re-set on next WM_MOUSEMOVE. Edge case not tested. |
| 100 | **Test WM_SIZE / WM_MOVE emit correct record values** | `spk_winapi_frame.cpp:57-72` | Records carry geometry data. Values not verified by any test. |

---

## WinAPI::PlatformRuntime

| Priority | Item | Source / Files | Notes |
|---:|---|---|---|
| 150 | **Test `createFrame()` returns a non-null frame with correct title and geometry** | `spk_winapi_platform_runtime.cpp` | Tested only transitively via Application. Needs an isolated test. |
| 120 | **Test `pollEvents()` does not crash on an idle message queue** | `spk_winapi_platform_runtime.cpp` | Smoke test for the event pump with no pending messages. |
| 100 | **Test frame events reach the frame callback after `pollEvents()`** | `spk_winapi_platform_runtime.cpp` | End-to-end: post a message to the frame, call `pollEvents()`, confirm the event record was received. |

---

## OpenGL::RenderContext

| Priority | Item | Source / Files | Notes |
|---:|---|---|---|
| 170 | **Test construction from a valid `WinAPI::Frame` succeeds: `isValid() == true`** | `spk_opengl_render_context.cpp:47-70` | Happy-path construction not tested in isolation. |
| 160 | **Test `makeCurrent()` on an invalidated context throws** | `spk_opengl_render_context.cpp:106-115` | After `invalidate()`, `isValid()` is false and `makeCurrent()` must throw. |
| 150 | **Test `isValid()` returns `false` after `invalidate()`** | `spk_opengl_render_context.cpp:91-95` | State transition not tested. |
| 140 | **Test `notifyResize()` updates the OpenGL viewport** | `spk_opengl_render_context.cpp:142-151` | After calling `notifyResize(rect)`, `glGetIntegerv(GL_VIEWPORT, ...)` should reflect the new size. |
| 120 | **Test `present()` on a valid context does not crash** | `spk_opengl_render_context.cpp:119-125` | Smoke test for `SwapBuffers`. |
| 100 | **Test `setVSync(true/false)` does not crash** | `spk_opengl_render_context.cpp:128-139` | `wglSwapIntervalEXT` may not be available; the code guards for this. Smoke test both values. |

---

## OpenGL::Runtime

| Priority | Item | Source / Files | Notes |
|---:|---|---|---|
| 160 | **Test `createRenderContext()` with a non-`WinAPI::Frame` type throws** | `spk_opengl_runtime.cpp:23-27` | `requireFrame<spk::WinAPI::Frame>()` should throw when given a mismatched frame type. Not tested. |
