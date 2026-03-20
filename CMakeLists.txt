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

# fmt is used for string formatting in log messages
find_package(fmt REQUIRED)

# ---- Core object library (shared between plugin and tests) ----
add_library(BinjaReturnHighlighter_core OBJECT)
add_library(BinjaReturnHighlighter::core ALIAS BinjaReturnHighlighter_core)

target_sources(
    BinjaReturnHighlighter_core
    PRIVATE
        source/ExitPointDetection.cpp
        source/ReturnHighlightRenderLayer.cpp

    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS include
    FILES
        include/BinjaReturnHighlighter/ColorDefs.hpp
        include/BinjaReturnHighlighter/ExitPointDetection.hpp
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

# ---- Optional UI support (Qt6 color picker) ----

if(TARGET binaryninjaui)
  find_package(Qt6 COMPONENTS Core Gui Widgets QUIET)
  if(Qt6_FOUND)
    set_target_properties(BinjaReturnHighlighter_plugin PROPERTIES AUTOMOC ON)

    target_sources(
        BinjaReturnHighlighter_plugin
        PRIVATE
            source/ColorPickerAction.cpp
            source/ui_plugin.cpp

        PUBLIC
        FILE_SET HEADERS
        BASE_DIRS include
        FILES
            include/BinjaReturnHighlighter/ColorPickerAction.hpp
    )

    target_link_libraries(
        BinjaReturnHighlighter_plugin
        PUBLIC binaryninjaui Qt6::Core Qt6::Gui Qt6::Widgets
    )
  endif()
endif()

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
