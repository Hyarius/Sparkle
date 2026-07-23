# Sparkle modular repository and vcpkg pipeline

## Result

The Sparkle ecosystem is now organized as independent component repositories.
The original `Sparkle` repository has not been used as a source workspace for
the split: the new repositories live beside it under
`C:\Users\User\source\repos`.

There is no separate `SparkleComponentLibraryTemplate` repository. The reusable
repository structure belongs to `SparkleTooling` as:

```text
SparkleTooling/resources/repoStructure
```

The component creator copies that structure, replaces its explicit tokens, and
initializes a new independent Git repository. It never derives the CMake or
package identity from the destination folder name.

Runtime component dependencies are vcpkg packages from
`SparkleVcpkgRegistry`. They are no longer CMake source submodules and are no
longer discovered by looking under `libraries/` or beside `SparkleSDK`.

## Repository map

| Repository | Responsibility | Main exported target |
| --- | --- | --- |
| `SparkleCore` | Foundation types, containers, math, threading, utilities, and APIs that do not require a graphics or application runtime | `Sparkle::Core` |
| `SparkleOpenGL` | OpenGL-facing graphics objects and rendering infrastructure | `Sparkle::OpenGL` |
| `SparkleApplication` | Application lifecycle and the base widget abstraction | `Sparkle::Application` |
| `SparkleWidget` | Concrete widget implementations | `Sparkle::Widget` |
| `SparkleGameEngine` | Game-oriented runtime facilities built on Application and OpenGL | `Sparkle::GameEngine` |
| `SparkleVoxel` | Voxel-specific functionality | `Sparkle::Voxel` |
| `SparkleResource` | Resource compiler used while another component is built | `Sparkle::ResourceCompiler` and the `SparkleResourceCompiler` host tool |
| `SparkleSDK` | Optional umbrella integration repository for consumers that want every runtime component | `Sparkle::Sparkle` |
| `SparkleTooling` | Global CLI, embedded component structure, shared configuration, component creation, and registry maintenance | `sparkle` command |
| `SparkleVcpkgRegistry` | Custom vcpkg Git registry containing every `sparkle-*` port and version record | Not a CMake library |

The intended runtime dependency direction is:

```text
SparkleCore
└── SparkleOpenGL
    └── SparkleApplication
        ├── SparkleWidget
        └── SparkleGameEngine
            └── SparkleVoxel

SparkleSDK ── consumes every runtime component
```

Some components deliberately retain direct dependencies in addition to this
outline. For example, Widget directly declares Core, OpenGL, and Application;
Voxel directly declares Core, OpenGL, and GameEngine. The explicit manifests
make those API requirements reviewable instead of relying on transitive
linking by accident.

`SparkleResource` is a separate build-time edge. Newly created components
declare it as a vcpkg **host dependency**, because its executable must run on
the build machine even when the component is cross-compiled.

## Why the embedded structure belongs to SparkleTooling

A standalone template repository would add a repository, version, clone, and
compatibility relationship without providing an independently useful
deliverable. The component creator and the structure it understands must
evolve together, so they now have one version and one review boundary.

The embedded structure contains:

```text
repoStructure/
├── cmake/
│   ├── ComponentDependencies.cmake
│   ├── ComponentResources.hpp.in
│   ├── InstallLibrary.cmake
│   ├── PackageConfig.cmake.in
│   └── SparkleResources.cmake
├── includes/sparkle/<component>/
├── resources/
│   └── example.txt
├── srcs/
│   └── component.cpp
├── tests/TUs/
│   ├── CMakeLists.txt
│   ├── ComponentTestResources.hpp.in
│   └── component_resource_test.cpp
├── CMakeLists.txt
├── CMakePresets.json
├── README.md
├── vcpkg.json
└── vcpkg-configuration.json
```

Every generated repository therefore begins with public include and
implementation folders, a component-owned resources folder, a resource
translation unit, isolated TU/API tests, installation rules, a package config,
and vcpkg manifests.

## Global Sparkle tooling

The tooling is installed with:

```powershell
cd C:\Users\User\source\repos\SparkleTooling
.\tools\Install-SparkleTooling.ps1
```

It is currently installed in:

```text
%LOCALAPPDATA%\Sparkle\bin
```

That directory has been added to the user PATH. A terminal that was open before
installation must be reopened before `sparkle` is found by name.

The installer also creates:

```text
%LOCALAPPDATA%\Sparkle\bin\sparkle.config.json
```

The file is shared by all present and future Sparkle tools installed in that
directory. Reinstalling `SparkleTooling` preserves it. It stores locations and
defaults only; it must not contain credentials.

Inspect or edit it with:

```powershell
sparkle config show
sparkle config set -Name repositoryRoot -Value C:\Users\User\source\repos
sparkle config set -Name registryWorkingTree `
    -Value C:\Users\User\source\repos\SparkleVcpkgRegistry
sparkle config set -Name vcpkgRoot -Value C:\vcpkg
sparkle config set -Name githubOwner -Value YourGitHubOwner
```

The current local registry baseline is:

```text
12789520e92a1487646713e95f2e781dfc643c6c
```

## Creating a component

`FolderName` is only the directory to create. `ComponentName` is the API and
package identity, without the `Sparkle` prefix:

```powershell
sparkle component create `
    -FolderName RenderingRuntimeRepository `
    -ComponentName Renderer `
    -PublicDependency Core,OpenGL `
    -PrivateDependency Application
```

This creates a repository whose identities are:

```text
Folder:          RenderingRuntimeRepository
CMake project:   SparkleRenderer
CMake package:   SparkleRenderer
CMake target:    Sparkle::Renderer
vcpkg port:      sparkle-renderer
test option:     SPARKLE_RENDERER_BUILD_TESTS
```

Public dependencies become `PUBLIC` CMake links and are emitted into the
installed package's `find_dependency(...)` block. Private dependencies become
`PRIVATE` links. Both kinds are added to `vcpkg.json`.

The creator also:

1. adds `sparkle-resource` to `vcpkg.json` with `"host": true`;
2. writes the configured registry URL and immutable baseline to
   `vcpkg-configuration.json`;
3. copies the embedded resource and TU structure;
4. replaces every component token;
5. rejects unresolved tokens and invalid or duplicated dependency names; and
6. initializes an independent Git repository on `main`.

External vcpkg dependencies are intentionally manual. Add them directly to the
new repository's `vcpkg.json`, then add their `find_package` and link rules
where appropriate.

## Building a component

Set `VCPKG_ROOT` and use the generated presets:

```powershell
$env:VCPKG_ROOT = 'C:\vcpkg'
cmake --preset debug
cmake --build --preset debug
ctest --preset debug --output-on-failure
```

The build flow is:

```text
vcpkg.json
    │
    ├── restores sparkle-* runtime packages from SparkleVcpkgRegistry
    └── restores sparkle-resource for the host triplet
             │
             └── SparkleResourceCompiler embeds resources/*

CMake find_package(...)
    └── links exported Sparkle::* targets

tests/TUs
    └── validates the component API and embedded resource access
```

There is no `sparkle_add_component(...)`, adjacent-directory fallback, or
component source submodule in this path. A component repository may still be
used with `add_subdirectory` for a deliberate local experiment, but that is
not its dependency distribution mechanism.

## Adding or releasing a vcpkg port

A vcpkg Git registry records immutable source commits. Commit the component
source before creating its port.

For a local bootstrap source:

```powershell
sparkle registry create-port `
    -ComponentName Renderer `
    -Version 0.1.0
```

When the component is on GitHub, use an accessible repository and an immutable
commit or tag:

```powershell
sparkle registry create-port `
    -ComponentName Renderer `
    -Version 0.1.0 `
    -Repository https://github.com/YourOwner/SparkleRenderer.git `
    -Ref <full-commit-hash-or-v0.1.0-tag>
```

The command reads the component's manifest when available, creates
`ports/sparkle-renderer`, and adds the vcpkg CMake helper dependencies. Review
the generated port, then register it:

```powershell
sparkle registry add-version -ComponentName Renderer
sparkle registry validate -VerifyGitTrees
git -C C:\Users\User\source\repos\SparkleVcpkgRegistry push
sparkle registry use-baseline
```

`add-version` commits the reviewed port, runs `vcpkg x-add-version`, and commits
the updated version database. `use-baseline` stores the registry's current
commit in the shared tooling configuration for newly generated repositories.

For a new release of an existing port:

1. commit and preferably tag the component source;
2. update the port's `REF` and manifest version;
3. increment `port-version` instead when only packaging changed;
4. run `sparkle registry add-version`;
5. validate and push the registry;
6. select its new baseline; and
7. update `vcpkg-configuration.json` in existing consumers that should adopt
   the new baseline.

Changing a source working tree without changing the immutable ref and registry
version does not update a vcpkg package. This is intentional reproducibility.

## Moving the local bootstrap to GitHub

The registry is currently usable on this computer. Its component portfiles and
consumer configurations use local `file:///C:/Users/User/source/repos/...`
Git URLs.

Before another machine can consume the ecosystem:

1. create and push each component repository;
2. create and push `SparkleTooling`;
3. create and push `SparkleVcpkgRegistry`;
4. replace each registry port's local source URL with its GitHub URL and use a
   pushed immutable ref;
5. add those changed port versions and push the registry;
6. replace the custom registry repository in component and SDK
   `vcpkg-configuration.json` files with the GitHub registry URL;
7. set `registryRepository`, `registryWorkingTree`, `registryBaseline`, and
   `githubOwner` in the shared tooling configuration; and
8. reinstall the tooling on the other computer to make the `sparkle` command
   globally available there.

`registryRepository` is the URL written into generated component manifests.
`registryWorkingTree` is the local checkout modified by registry maintenance
commands. Keeping these as two explicit settings supports a remote Git URL and
a local maintenance checkout at the same time.

## Existing repositories migrated to vcpkg

The existing component repositories now:

- declare Sparkle dependencies in `vcpkg.json`;
- pin the custom registry in `vcpkg-configuration.json` when they consume
  another Sparkle component;
- use `find_package(Sparkle... CONFIG REQUIRED)`;
- link exported `Sparkle::*` targets; and
- no longer use the old source lookup helper.

`SparkleSDK` follows the same model and restores all six runtime components
through vcpkg. Its standalone package-consumer test also declares
`sparkle-voxel` through the custom registry.

Every current component has a `resources/` folder and `tests/TUs/` coverage
area. Future components receive both from the embedded structure.

## Validation performed

The following checks pass:

- the `SparkleTooling` CMake/CTest creator smoke test;
- PowerShell parsing for every tooling script and module;
- custom registry structure, manifest, baseline, and Git-tree validation for
  all seven ports;
- installation and preservation of the global shared configuration;
- standalone `SparkleOpenGL` configuration and build while resolving
  `SparkleCore` from vcpkg;
- generation of a component whose folder name differs from its API identity;
- restoration of `sparkle-resource` as a vcpkg host package;
- execution of the restored resource compiler during the generated build;
- resolution and linking of `Sparkle::Core` in that generated component; and
- the generated resource TU test.

The complete generated `SparkleDiagnostics` test configured, built, embedded
its resources, and passed its CTest suite without any component source
submodule.

## Known source compatibility issue

A full generated component using OpenGL was also attempted through vcpkg. The
registry correctly resolved and began building the dependency graph, but MSVC
rejects the existing `constexpr` call to `std::isfinite` in:

```text
SparkleOpenGL/includes/structures/graphics/geometry/spk_color.hpp
```

This is an OpenGL source/compiler compatibility issue, not a registry or
component-generation failure. OpenGL was already validated through the same
registry flow with the configured Clang toolchain. The MSVC issue should be
handled as a separate source change so its API and behavior can be reviewed
independently.

## API review policy

The split is useful only if dependency direction remains disciplined:

- Core must not depend on OpenGL, Application, or concrete widgets.
- OpenGL should own graphics data and context-aware GPU representations, not
  the application lifecycle.
- Application should expose the lifecycle and base widget contract.
- Widget should contain concrete reusable widgets.
- GameEngine should build on the application/rendering contracts.
- Voxel should remain a feature layer rather than leaking into lower layers.
- SDK should aggregate packages but contain no foundational implementation.

The proposed OpenGL `Context` abstraction belongs in `SparkleOpenGL`: one CPU
object can own or index per-context GPU realizations while Application selects
or activates the context. That API can now be designed and validated inside
the smaller OpenGL repository without expanding Core or coupling concrete
widgets to context management.
