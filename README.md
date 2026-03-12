# Binary Ninja Return Statement Highlighter Plugin

This Binary Ninja plugin provides visual highlighting of return statements across all IL (Intermediate Language) views. It helps analysts quickly identify function exit points and control flow patterns by highlighting return statements in a distinctive color.

The API to perform efficient highlighting requires Binary Ninja 5.0.7290-stable+. **However**, this project realistically needs a newer version of the binaryninja-api repository to build effectively.

## Features

- Highlights all return statements across different IL views
- Works in multiple IL representations:
  - Low Level IL (LLIL)
  - Medium Level IL (MLIL)
  - High Level IL (HLIL)
- Uses a semi-transparent blue highlight for easy visibility
- Integrates seamlessly with Binary Ninja's [RenderLayer](https://dev-api.binary.ninja/cpp/class_binary_ninja_1_1_render_layer.html) system

![Screenshot of plugin highlighting return statements](/screenshot.png?raw=true "Screenshot of Plugin")

## Usage

1. Open a binary in Binary Ninja
2. In any IL view, click the hamburger menu button (top-right corner of the view)
3. Hover over "Render Layers"
4. Enable the "Highlight Return Statements" layer
5. All return statements will now be highlighted with a semi-transparent blue background

## Installation

The GitHub Actions CI uploads artifacts for each OS, however, you can build from source by following the directions in the next sections.

### Prerequisites

- CMake 4.2.0 or newer
- C++20 compatible compiler
- Binary Ninja API
  - Stable v5.0.7290+
  - 4.3.6779-dev (Jan 31, 2025) or newer
    - Includes commit [8862696](https://github.com/Vector35/binaryninja-api/commit/8862696926173104957729683832591438161557)

### Building

This plugin can be built using the Binary Ninja API submodule in the [`external/`](./external) directory.

```bash
git submodule update --init external/binaryninja-api
git -C external/binaryninja-api submodule update --init --recursive vendor
```

Then, we build like any other CMake project:

```bash
cmake -B build -S .
cmake --build build
```

The plugin can be installed to the default Binary Ninja plugin directory on each OS by running the install target with CMake.

```bash
$ cmake --build build --target install
[...]
-- Installing: [...]/plugins/libReturnHighlighter.dylib
```

## How It Works

The plugin implements a custom `ReturnHighlightLayer` that:

1. Registers itself for all IL views during plugin initialization
2. Analyzes each IL operation for return statements
3. Applies blue highlighting with 50% opacity to lines containing return instructions
4. Uses Binary Ninja's [RenderLayer](https://dev-api.binary.ninja/cpp/class_binary_ninja_1_1_render_layer.html) API to integrate with the UI

## Improvements

- Allow the user to choose a color for line highlighting

## Using as a Template

To fork this project for your own plugin, update the following:

- `CMakeLists.txt`: project name, target names, developer mode variable (`BinjaReturnHighlighter_DEVELOPER_MODE`), and `OUTPUT_NAME`
- `vcpkg.json`: package `name`
- `include/BinjaReturnHighlighter/`: rename directory and header to match your plugin
- `source/`: rename/replace source files
- `CMakePresets.json`: developer mode variable reference
- `.github/workflows/build.yaml`: artifact name and NuGet feed URL
- `LICENSE`: copyright holder
- `README.md`: content

## Developing

Create a `CMakeUserPresets.json` file with the following (for macOS), assuming you have a freshly cloned/updated [`vcpkg`](https://github.com/microsoft/vcpkg) repo in the parent directory. Adjust `<path-to-clang-tidy>` to your installation (e.g., `/opt/homebrew/Cellar/llvm/22.1.0/bin/clang-tidy` on macOS with Homebrew):

```json
{
  "version": 2,
  "configurePresets": [
    {
      "name": "dev-macos",
      "inherits": ["ci-macos-universal", "dev-mode", "common"],
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build",
      "environment": {
        "VCPKG_ROOT": "${sourceParentDir}/vcpkg"
      },
      "cacheVariables": {
        "CMAKE_CXX_CLANG_TIDY": "<path-to-clang-tidy>;-warnings-as-errors=*",
        "CMAKE_CXX_CPPCHECK": "cppcheck",
        "CMAKE_OSX_ARCHITECTURES": null,
        "CMAKE_BUILD_TYPE": "RelWithDebInfo"
      }
    }
  ]
}
```

### Testing

Integration tests use [Google Test](https://github.com/google/googletest) and require a local Binary Ninja installation (the tests load a real binary via headless mode). Tests are only built in developer mode when `FindBinaryNinjaCore` succeeds.

```bash
cmake --build build
ctest --test-dir build
```

The test binary (`test/data/simple`) is a pre-compiled Mach-O from `test/data/simple.c`. To recompile it (e.g., after changing the source):

```bash
cc -o test/data/simple test/data/simple.c
```

### Format Code

```bash
cmake --build build --target format-fix
```

### Code Quality with cppcheck and clang-tidy

Requires CMake 4.2.0+ for [`CMAKE_SKIP_LINTING`](https://cmake.org/cmake/help/latest/variable/CMAKE_SKIP_LINTING.html) so that we don't code quality checks on binaryninja-api repository code.

### Sanitizers

Run from repository root:

```bash
export ASAN_OPTIONS=detect_leaks=1 LSAN_OPTIONS=suppressions=asan.supp
LD_PRELOAD="/usr/lib/clang/20/lib/x86_64-redhat-linux-gnu/libclang_rt.asan.so" ~/binaryninja/binaryninja
```

Or on macOS (still some issues I need to figure out...)

```bash
export ASAN_OPTIONS=detect_leaks=1
DYLD_INSERT_LIBRARIES=/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/lib/clang/17/lib/darwin/libclang_rt.asan_osx_dynamic.dylib /Applications/Binary\ Ninja.app/Contents/MacOS/binaryninja
# or if built with homebrew clang
DYLD_INSERT_LIBRARIES=/opt/homebrew/Cellar/llvm/22.1.0/lib/clang/22/lib/darwin/libclang_rt.asan_osx_dynamic.dylib /Applications/Binary\ Ninja.app/Contents/MacOS/binaryninja
```
