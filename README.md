# Sparkle

Sparkle is a C++23 OpenGL rendering library.

## Requirements

- CMake 3.16 or newer
- A C++23-capable compiler
- vcpkg with the dependencies declared in `vcpkg.json`
- Ninja when using the provided presets

## Build

Configure and build one of the standard presets:

```powershell
cmake --preset debug
cmake --build --preset debug
```

Run the test preset:

```powershell
cmake --preset test
cmake --build --preset test
ctest --output-on-failure --test-dir build/test
```

## Playground battle controls

The optional Playground battle prototype supports an 800×600 minimum window. During deployment,
select a player card (or 1–6), click a highlighted support cell, then press Enter or Ready. During
a player activation, use M for movement, 1–8 for abilities, Space for End Turn, and Escape or a
short right-click to cancel targeting. Right-drag orbits the tactical camera; the wheel zooms.

## Install and Package

The project installs the `Sparkle::Sparkle` target, public headers, generated resource header, and CMake package files:

```powershell
cmake --preset release
cmake --build --preset release
cmake --install build/release
```

Consumers can use either package name:

```cmake
find_package(Sparkle CONFIG REQUIRED)
# or:
find_package(sparkle CONFIG REQUIRED)

target_link_libraries(my_app PRIVATE Sparkle::Sparkle)
```

CPack is enabled after configuration, so a configured build tree can produce packages with:

```powershell
cmake --build build/release --target package
```

## Coverage

Coverage uses LLVM source-based coverage and requires Clang plus `llvm-cov` and `llvm-profdata` in `PATH`.

Use the generic preset when you already selected a Clang compiler externally:

```powershell
cmake --preset coverage
cmake --build --preset coverage
```

Use the Clang-specific preset to request `clang` and `clang++` from `PATH` explicitly:

```powershell
cmake --preset coverage-clang
cmake --build --preset coverage-clang
```
