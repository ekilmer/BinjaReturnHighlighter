# BinjaReturnHighlighter

Binary Ninja plugin (C++20) that highlights return/tailcall statements in IL views using a RenderLayer.

## Build

CMake 4.2+ with vcpkg for dependencies (fmt, gtest). Presets in `CMakePresets.json` (versioned/CI) and `CMakeUserPresets.json` (local dev, not versioned).

Two local development presets (both require `VCPKG_ROOT`):

    # Normal dev build — clang-tidy + cppcheck, RelWithDebInfo → build/
    cmake --preset dev
    cmake --build build

    # ASan build — address sanitizer for testing → build-asan/
    cmake --preset asan
    cmake --build build-asan

Key CMake cache variables:
- `BN_ALLOW_STUBS=ON` — build against API stubs (no BN install needed for compilation)
- `BN_QT_DIR` — path to BN's Qt install for UI support (color picker)
- `BinjaReturnHighlighter_DEVELOPER_MODE=ON` — enables lint targets and tests
- Hidden `sanitize` base preset provides `CMAKE_BUILD_TYPE=Sanitize`; platform presets define their own sanitizer flags

Install: `cmake --build build --target install`

## Lint

clang-format via `.clang-format` (copied from binaryninja-api):

    cmake --build build --target format-check   # check
    cmake --build build --target format-fix     # auto-fix

Standalone (no build dir needed):

    cmake -P cmake/lint.cmake
    cmake -D FIX=YES -P cmake/lint.cmake

## Test

Requires dev-mode AND a real Binary Ninja install (`BinaryNinjaCore` library). Use the ASan build:

    cmake --preset asan
    cmake --build build-asan
    cd build-asan && ctest

Test binary: `test/data/simple` (compiled from `test/data/simple.c`).

It is also important that the normal dev build also works because it runs important static analysis checks.

    cmake --preset dev
    cmake --build build

## Project structure

- `source/` — plugin implementation
  - `plugin.cpp` — `CorePluginInit` entry point (registers RenderLayer)
  - `ui_plugin.cpp` — `UIPluginInit` entry point (color picker, only with Qt6)
  - `ReturnHighlightRenderLayer.cpp` — core highlight logic
  - `ColorPickerAction.cpp` — Qt color picker UI
- `include/BinjaReturnHighlighter/` — public headers
- `test/` — GTest integration tests (link against real BinaryNinjaCore)
- `external/binaryninja-api/` — git submodule
- `cmake/` — lint and dev-mode modules

## Binary Ninja API reference

Headers in `external/binaryninja-api/`:
- `binaryninjaapi.h` — main C++ API (BinaryView, Function, Settings, RenderLayer, etc.)
- `binaryninjacore.h` — C core API and enums (BNHighlightColor, etc.)
- `highlevelilinstruction.h`, `mediumlevelilinstruction.h`, `lowlevelilinstruction.h` — IL types

## Code style

- C++20, `.clang-format` from binaryninja-api
- Tabs (width 4), 120 column limit, Allman-style braces
- Per-plugin named logger (`LogRegistry::CreateLogger`) instead of global `LogWarn`
