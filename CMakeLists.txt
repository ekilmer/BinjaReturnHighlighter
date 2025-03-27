cmake_minimum_required(VERSION 3.14)

# Set up project-specific versions and options
project(
    BinjaReturnHighlighter
    VERSION 1.0.0
    LANGUAGES CXX
    DESCRIPTION "Binary Ninja Return Statement Highlighter Plugin"
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Declare plugin ----
add_library(
    BinjaReturnHighlighter_plugin MODULE
    source/plugin.cpp
    source/ReturnHighlightRenderLayer.cpp
)
add_library(BinjaReturnHighlighter::plugin ALIAS BinjaReturnHighlighter_plugin)

set_target_properties(
    BinjaReturnHighlighter_plugin PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
    EXPORT_NAME BinjaReturnHighlighter
    OUTPUT_NAME ReturnHighlighter
)

if(APPLE)
    # On macOS, MODULE libraries end with ".so" by default, but Binja doesn't load these
    set(BinjaReturnHighlighter_SHARED_MODULE_SUFFIX ".dylib" CACHE STRING "Plugin shared module suffix")
    mark_as_advanced(BinjaReturnHighlighter_SHARED_MODULE_SUFFIX)
    set_target_properties(
        BinjaReturnHighlighter_plugin PROPERTIES
        SUFFIX "${BinjaReturnHighlighter_SHARED_MODULE_SUFFIX}"
    )
endif()

target_include_directories(
    BinjaReturnHighlighter_plugin
    PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
)

target_compile_features(BinjaReturnHighlighter_plugin PUBLIC cxx_std_17)

# ---- Dependencies ----

if(BinjaReturnHighlighter_VENDORED_BINARYNINJAAPI)
    set(HEADLESS True)
    add_subdirectory(external/binaryninja-api EXCLUDE_FROM_ALL)
else()
    find_package(BinaryNinjaAPI REQUIRED)
endif()
target_link_libraries(BinjaReturnHighlighter_plugin PUBLIC binaryninjaapi)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  bn_install_plugin(BinjaReturnHighlighter_plugin)
endif()

# ---- Developer mode ----

if(NOT BinjaReturnHighlighter_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of BinjaReturnHighlighter"
  )
endif()

include(cmake/dev-mode.cmake)
