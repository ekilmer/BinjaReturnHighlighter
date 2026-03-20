# Developing

Contributor guide for building, testing, and linting the BinjaReturnHighlighter plugin.

**Prerequisites:** CMake 4.2+, C++20 compiler, [vcpkg](https://github.com/microsoft/vcpkg), Binary Ninja API submodule (`external/binaryninja-api`).

## Dev Preset Setup

Create a `CMakeUserPresets.json` file (not versioned) with local development presets. The example below is for macOS, assuming a freshly cloned/updated [`vcpkg`](https://github.com/microsoft/vcpkg) repo in the parent directory. Adjust `<path-to-clang-tidy>`, `<path-to-clang>`, `<path-to-clang++>`, and `<path-to-qt>` to your installation paths.

<details>
<summary>Example <code>CMakeUserPresets.json</code> (macOS)</summary>

```json
{
  "version": 2,
  "configurePresets": [
    {
      "name": "dev",
      "inherits": ["ci-macos-universal", "dev-mode", "common"],
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build",
      "environment": {
        "VCPKG_ROOT": "${sourceParentDir}/vcpkg"
      },
      "cacheVariables": {
        "BN_QT_DIR": "<path-to-qt>",
        "CMAKE_CXX_CLANG_TIDY": "<path-to-clang-tidy>",
        "CMAKE_CXX_CPPCHECK": "cppcheck",
        "CMAKE_OSX_ARCHITECTURES": null,
        "CMAKE_BUILD_TYPE": "RelWithDebInfo"
      }
    },
    {
      "name": "asan",
      "inherits": ["sanitize", "flags-appleclang", "ci-std", "ci-macos-vcpkg", "dev-mode", "common"],
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build-asan",
      "environment": {
        "VCPKG_ROOT": "${sourceParentDir}/vcpkg"
      },
      "cacheVariables": {
        "BN_QT_DIR": "<path-to-qt>",
        "CMAKE_C_COMPILER": "<path-to-clang>",
        "CMAKE_CXX_COMPILER": "<path-to-clang++>",
        "CMAKE_CXX_FLAGS_SANITIZE": "-O2 -g -fsanitize=address -fno-omit-frame-pointer -fno-common -include cstdlib",
        "CMAKE_OSX_ARCHITECTURES": null
      }
    }
  ]
}
```

</details>

## Testing

Integration tests use [Google Test](https://github.com/google/googletest) and require a local Binary Ninja installation (the tests load a real binary via headless mode). Tests are only built in developer mode when `FindBinaryNinjaCore` succeeds.

```bash
cmake --build build
ctest --test-dir build
```

The test binary (`test/data/simple`) is a pre-compiled Mach-O from `test/data/simple.c`. To recompile it (e.g., after changing the source):

```bash
cc -o test/data/simple test/data/simple.c
```

## Pre-commit Hooks

This project uses [prek](https://github.com/j178/prek) for pre-commit hooks. Install it once to enable automatic checks before each commit:

```bash
prek install
```

Hooks include trailing whitespace removal, EOF newline fixing, YAML/TOML/JSON validation, merge conflict detection, large file checks, and automatic clang-format via the project's CMake lint script.

To run all hooks manually against the full repo:

```bash
prek run --all-files
```

## Suggested Workflow

Before building, auto-fix both clang-tidy and clang-format issues, then build and test:

```bash
cmake --preset dev
cmake --build build --target tidy-fix format-fix
cmake --build build

cmake --preset asan
cmake --build build-asan
ctest --test-dir build-asan -VV
```

The `tidy-fix` target runs clang-tidy with `--fix` on all project source files using the `compile_commands.json` from the build directory. Not all clang-tidy checks are auto-fixable — unfixable diagnostics will still be reported during the normal build. The ASan build runs tests with AddressSanitizer to catch memory errors.

Available lint targets:

| Target | Description |
|--------|-------------|
| `format-fix` | Auto-fix clang-format issues |
| `format-check` | Check formatting without fixing (CI) |
| `tidy-fix` | Auto-fix clang-tidy issues |
| `tidy-check` | Check clang-tidy issues without fixing |

## Code Quality with cppcheck and clang-tidy

Requires CMake 4.2.0+ for [`CMAKE_SKIP_LINTING`](https://cmake.org/cmake/help/latest/variable/CMAKE_SKIP_LINTING.html) so that we don't run code quality checks on binaryninja-api repository code.

clang-tidy and cppcheck run automatically during compilation when configured via `CMAKE_CXX_CLANG_TIDY` and `CMAKE_CXX_CPPCHECK` (set in the dev preset). The `tidy-fix` and `tidy-check` targets provide standalone clang-tidy analysis independent of the build.

## Sanitizers

Build the plugin with the ASan preset and install it:

```bash
cmake --preset asan
cmake --build build-asan
cmake --build build-asan --target install
```

Then launch Binary Ninja with the ASan runtime injected. The `asan.supp` file suppresses leaks from system libraries and Binary Ninja itself, so only leaks originating from plugins are reported.

**Linux:**

```bash
export ASAN_OPTIONS=detect_leaks=1 LSAN_OPTIONS=suppressions=asan.supp
LD_PRELOAD="/usr/lib/clang/20/lib/x86_64-redhat-linux-gnu/libclang_rt.asan.so" ~/binaryninja/binaryninja
```

**macOS (Homebrew LLVM):**

The ASan preset uses Homebrew Clang, so use the matching ASan runtime. Adjust the LLVM version path as needed:

```bash
export ASAN_OPTIONS=detect_leaks=1 LSAN_OPTIONS=suppressions=asan.supp
DYLD_INSERT_LIBRARIES=/opt/homebrew/Cellar/llvm/22.1.0/lib/clang/22/lib/darwin/libclang_rt.asan_osx_dynamic.dylib \
  /Applications/Binary\ Ninja.app/Contents/MacOS/binaryninja
```

**macOS (Xcode Clang):**

If you build with Xcode's clang instead, use the Xcode ASan runtime:

```bash
export ASAN_OPTIONS=detect_leaks=1 LSAN_OPTIONS=suppressions=asan.supp
DYLD_INSERT_LIBRARIES=/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/lib/clang/17/lib/darwin/libclang_rt.asan_osx_dynamic.dylib \
  /Applications/Binary\ Ninja.app/Contents/MacOS/binaryninja
```
