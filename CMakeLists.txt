cmake_minimum_required(VERSION 4.2)

# Set up project-specific versions and options
project(
    BinjaReturnHighlighter
    VERSION 1.0.0
    LANGUAGES CXX
    DESCRIPTION "Binary Ninja Return Statement Highlighter Plugin"
)

include(cmake/variables.cmake)

# ---- Dependencies ----

add_subdirectory(external)

# fmt is included as an example vcpkg dependency for template adopters
find_package(fmt REQUIRED)

# ---- Core object library (shared between plugin and tests) ----
add_library(BinjaReturnHighlighter_core OBJECT)
add_library(BinjaReturnHighlighter::core ALIAS BinjaReturnHighlighter_core)

target_sources(
    BinjaReturnHighlighter_core
    PRIVATE
        source/ReturnHighlightRenderLayer.cpp

    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS include
    FILES
        include/BinjaReturnHighlighter/ReturnHighlightRenderLayer.hpp
)

set_target_properties(
    BinjaReturnHighlighter_core PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

target_compile_features(BinjaReturnHighlighter_core PUBLIC cxx_std_20)
target_link_libraries(BinjaReturnHighlighter_core PUBLIC binaryninjaapi fmt::fmt)

# ---- Declare plugin ----
add_library(BinjaReturnHighlighter_plugin SHARED)
add_library(BinjaReturnHighlighter::plugin ALIAS BinjaReturnHighlighter_plugin)

target_sources(
    BinjaReturnHighlighter_plugin
    PRIVATE
        source/plugin.cpp
)

set_target_properties(
    BinjaReturnHighlighter_plugin PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
    EXPORT_NAME BinjaReturnHighlighter
    OUTPUT_NAME ReturnHighlighter
)

target_link_libraries(BinjaReturnHighlighter_plugin PUBLIC BinjaReturnHighlighter::core)

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
