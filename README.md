# Binary Ninja Return Statement Highlighter Plugin

This Binary Ninja plugin provides visual highlighting of return and tailcall statements across all IL (Intermediate Language) views. It helps analysts quickly identify function exit points and control flow patterns by highlighting these statements in a distinctive color.

The API to perform efficient highlighting requires Binary Ninja 5.0.7290-stable+. **However**, this project realistically needs a newer version of the binaryninja-api repository to build effectively.

## Features

- Highlights all return and tailcall statements across different IL views
- Works in multiple IL representations:
  - Low Level IL (LLIL)
  - Medium Level IL (MLIL)
  - High Level IL (HLIL)
- Configurable highlight color: 9 preset colors (blue default) + custom hex via Binary Ninja settings
- Color picker UI accessible via Plugins > Return Highlighter > Choose Color...
- Integrates seamlessly with Binary Ninja's [RenderLayer](https://dev-api.binary.ninja/cpp/class_binary_ninja_1_1_render_layer.html) system

![Screenshot of plugin highlighting return statements](/screenshot.png?raw=true "Screenshot of Plugin")

## Usage

1. Open a binary in Binary Ninja
2. In any IL view, click the hamburger menu button (top-right corner of the view)
3. Hover over "Render Layers"
4. Enable the "Highlight Return Statements" layer
5. All return and tailcall statements will now be highlighted with a colored background (blue by default, configurable in settings)

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

See [DEVELOPING.md](DEVELOPING.md) for contributor setup, testing, linting, and sanitizer instructions.

## How It Works

The plugin implements a custom `ReturnHighlightLayer` that:

1. Registers itself for all IL views during plugin initialization
2. Analyzes each IL operation for return and tailcall statements
3. Highlights matching lines using a configurable color (default: blue, full opacity) resolved from the `returnHighlighter.highlightColor` setting
4. Uses Binary Ninja's [RenderLayer](https://dev-api.binary.ninja/cpp/class_binary_ninja_1_1_render_layer.html) API to integrate with the UI

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
