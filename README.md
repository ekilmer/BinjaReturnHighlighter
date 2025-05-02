# Binary Ninja Return Statement Highlighter Plugin

This Binary Ninja plugin provides visual highlighting of return statements across all IL (Intermediate Language) views. It helps analysts quickly identify function exit points and control flow patterns by highlighting return statements in a distinctive color.

Requires Binary Ninja 5.0.7290-stable+.

This plugin was created as a working example for some CMake refactoring ideas I had in mind for Binary Ninja's public C++ API. While this repo includes vcpkg packaging and handling of dependencies, this only works with my [`cmake-refactor*` branches](https://github.com/ekilmer/binaryninja-api/branches/all?query=cmake-refactor&lastTab=overview). However, the CI for this plugin also tests upstream's [`dev` branch](https://github.com/Vector35/binaryninja-api/tree/dev) to ensure compatibility and prove that this plugin build doesn't rely on custom patches.

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

- CMake 3.14 or newer
- C++17 compatible compiler
- Binary Ninja API
  - Stable v5.0.7290+
  - 4.3.6779-dev (Jan 31, 2025) or newer
    - Includes commit [8862696](https://github.com/Vector35/binaryninja-api/commit/8862696926173104957729683832591438161557)

### Building

This plugin can be built using the Binary Ninja API submodule in the [`external/`](./external) directory.

```bash
git submodule update --init --recursive
```

You can adjust this API submodule to point to the [official Vector35 binaryninja-api repository](https://github.com/Vector35/binaryninja-api), and the following build instructions should still work.

```bash
cmake -B build -S .
cmake --build build
```

The plugin can be installed to the default Binary Ninja plugin directory on each OS by running the install target with CMake.

```bash
$ cmake --build build --target install
[...]
-- Installing: /Users/user/Library/Application Support/Binary Ninja/plugins/libReturnHighlighter.dylib
```

## How It Works

The plugin implements a custom `ReturnHighlightLayer` that:

1. Registers itself for all IL views during plugin initialization
2. Analyzes each IL operation for return statements
3. Applies blue highlighting with 50% opacity to lines containing return instructions
4. Uses Binary Ninja's [RenderLayer](https://dev-api.binary.ninja/cpp/class_binary_ninja_1_1_render_layer.html) API to integrate with the UI

## TODO Improvements

- Allow the user to choose a color for line highlighting
