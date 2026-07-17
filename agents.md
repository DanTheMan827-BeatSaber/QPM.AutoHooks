# QPM.AutoHooks — Repository Overview

## Purpose

**QPM.AutoHooks** is a Beat Saber Quest mod helper library. Its job is to automatically generate a header file (`shared/hooks.hpp`) that wraps every `MAKE_HOOK*` macro defined in [beatsaber-hook](https://github.com/QuestPackageManager/beatsaber-hook) with three additional variants:

| Generated Macro Family | Install Trigger | Description |
|---|---|---|
| `MAKE_EARLY_HOOK*` | `INSTALL_EARLY_HOOKS()` | Hook registered for deferred installation at startup (early) |
| `MAKE_LATE_HOOK*` | `INSTALL_LATE_HOOKS()` | Hook registered for deferred installation at startup (late) |
| `MAKE_DLOPEN_HOOK*` | dlopen / constructor | Hook installed automatically via `__attribute((constructor))` when the library is loaded |

This removes boilerplate from mod authors: instead of manually calling `INSTALL_HOOK` at the right time, they use one of the generated macros and the library takes care of installation.

## Repository Structure

```
QPM.AutoHooks/
├── .github/
│   └── workflows/
│       ├── build-ndk.yml       # CI: builds against every beatsaber-hook version
│       └── publish.yml         # CI: publishes a tagged release to qpackages.com
├── cmake/
│   └── targets/
│       └── quest.cmake         # CMake target config for Quest (Android ARM64)
├── scripts/
│   └── make-hooking.ps1        # PowerShell script that generates shared/hooks.hpp
├── shared/
│   └── hooks.hpp               # GENERATED — do not edit directly
├── src/
│   └── main.cpp                # Stub compile test for shared/hooks.hpp
├── CMakeLists.txt              # CMake build definition
├── mod.template.json           # Template for the QPM mod manifest
├── qpm.json                    # QPM package manifest (dependencies, scripts)
└── qpm.shared.json             # QPM shared package definition (published API surface)
```

## Key Components

### `scripts/make-hooking.ps1`

The core generator. It:

1. Reads beatsaber-hook's `hooking.hpp` (at `extern/includes/beatsaber-hook/shared/utils/hooking.hpp` for versions < 8.x, or `extern/includes/beatsaber-hook/shared/hooking.hpp` for 8.x+).
2. Uses a regex to extract every `MAKE_HOOK*` macro definition together with its preceding comment block.
3. Calls `ReplaceMakeWithAuto` to produce three renamed families of macros (`EARLY`, `LATE`, `DLOPEN`) by:
   - Renaming the macro itself (`MAKE_HOOK_FIND_X` → `MAKE_EARLY_HOOK_FIND_X` etc.)
   - Inserting an auto-registration call (e.g., `MAKE_EARLY_HOOK_INSTALL_WITH_AUTOLOGGER(name_)`) before the last line of the macro body.
4. Prepends a static boilerplate section containing:
   - The `__AutoHooksInternal__::DeferredHooks` class (manages early/late hook queues and a shared `Paper::ConstLoggerContext` logger)
   - All install-helper macros (`INSTALL_HOOK_ON_DLOPEN_WITH_AUTOLOGGER`, `INSTALL_EARLY_HOOKS`, etc.)
5. Writes the result to `shared/hooks.hpp` (only if the content changed).

### `shared/hooks.hpp`

Auto-generated output. Included by mod authors instead of (or in addition to) beatsaber-hook's own `hooking.hpp`. Not committed to the main branch — each tagged release contains the version generated against a specific beatsaber-hook release.

### `src/main.cpp`

A minimal compile test to ensure the generated `shared/hooks.hpp` compiles correctly against the target beatsaber-hook version. Uses `MAKE_DLOPEN_HOOK(Test, (0), void)` where `(0)` is a null pointer constant — constexpr-safe for 7.x and compatible with 8.x's `resolve_addr` syntax.

### `.github/workflows/build-ndk.yml`

Runs on push, PR, and hourly. Fetches every beatsaber-hook version > 5.1.0 from [qpackages.com](https://qpackages.com) and builds a matrix job for each:

1. Resolves version-specific dependencies (libil2cpp, flamingo, capstone, paper vs paper2_scotland2, And64InlineHook).
2. Appends version-specific cmake flags to `cmake/targets/quest.cmake`.
3. Runs `qpm s make-hooking` to regenerate `shared/hooks.hpp`.
4. Runs `qpm s qmod` to compile the stub and package a `.qmod`.
5. On success, commits `shared/hooks.hpp`, tags the commit, and dispatches the publish workflow.

Each job runs with `continue-on-error: true` so one failing version doesn't block others.

## Version Compatibility Notes

| beatsaber-hook Version | Notes |
|---|---|
| < 6.3.0 | Uses `paper` (not `paper2_scotland2`) and `And64InlineHook` |
| 6.3.0 – 6.x | Uses `paper2_scotland2` and `And64InlineHook`, `UNITY_2021` |
| 7.0.0+ | Adds `libil2cpp`, `flamingo`, `capstone`; defines `UNITY_6`; also adds `beatsaber-hook/shared` to cmake include paths (needed by 7.1.0's capstone headers) |
| 7.2.0 – 7.4.x | `beatsaber-hook` has a `static_cast<void*>(uintptr_t)` bug in `InstallHook`; worked around in `hooks.hpp` via `#ifdef INSTALL_HOOK_DIRECT` |
| 8.0.0+ | `hooking.hpp` moved to `shared/hooking.hpp`; uses `i2c::` namespace and lowercase `hook_##name` structs |

## QPM Scripts (defined in `qpm.json`)

| Script | Command | Purpose |
|---|---|---|
| `make-hooking` | `pwsh ./scripts/make-hooking.ps1` | Regenerate `shared/hooks.hpp` |
| `qmod` | `qpm qmod zip` | Compile the stub and produce the `.qmod` package |
