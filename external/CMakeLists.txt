# Add external dependencies that are vendored in this repository

# Remove warning flags for external dependencies
string(REGEX REPLACE "(^| )-W[^ ]*" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string(REGEX REPLACE "(^| )/[Ww][^ ]*" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

# Workaround compiler errors for insecure functions on Windows when compiling with MSVC
#   error C4996: 'getenv': This function or variable may be unsafe.
#   Consider using _dupenv_s instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS.
#   error C4996: 'strdup': The POSIX name for this item is deprecated.
string(REGEX REPLACE "(^| )/sdl(\\+|( |$))" "\\3" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string(STRIP "${CMAKE_CXX_FLAGS}" CMAKE_CXX_FLAGS)

# We don't want to lint the external dependencies
# Added in CMake 4.2
set(CMAKE_SKIP_LINTING ON)

# Make the binaryninja-api code available
# When Qt6 is found, build with UI support so binaryninjaui target is created.
# Otherwise, build headless (for tests and headless environments).
# Set Qt6_ROOT to point find_package at Binary Ninja's Qt fork.
find_package(Qt6 COMPONENTS Core Gui Widgets QUIET)
if(Qt6_FOUND)
  set(HEADLESS OFF)
else()
  set(HEADLESS ON)
endif()
add_subdirectory(binaryninja-api EXCLUDE_FROM_ALL SYSTEM)

# When building with stubs (no BN install), the API creates binaryninjaui as a
# UTILITY target that cannot be linked. If Qt was found, create a separate
# interface target providing headers and Qt linkage so UI code compiles.
# The real libbinaryninjaui is resolved at runtime via @rpath.
if(TARGET binaryninjaui AND Qt6_FOUND)
  get_target_property(_bn_ui_type binaryninjaui TYPE)
  if(_bn_ui_type STREQUAL "UTILITY")
    add_library(binaryninjaui_stub INTERFACE)
    target_link_libraries(binaryninjaui_stub INTERFACE binaryninjaapi)
    target_include_directories(binaryninjaui_stub SYSTEM INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/binaryninja-api/ui")

    # On Windows, __declspec(dllimport) requires an import library (.lib).
    # Generate stub implementations from UI headers and compile into a stub
    # DLL whose .lib satisfies the MSVC linker.
    if(WIN32)
      find_package(Python3 COMPONENTS Interpreter REQUIRED)
      set(_ui_headers_dir "${CMAKE_CURRENT_SOURCE_DIR}/binaryninja-api/ui")
      file(GLOB _ui_headers "${_ui_headers_dir}/*.h")

      add_custom_command(
          OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/ui_stubs.cpp"
          COMMAND ${Python3_EXECUTABLE}
              "${CMAKE_CURRENT_LIST_DIR}/../cmake/stubs/generate_ui_stubs.py"
              "${_ui_headers_dir}"
              "${CMAKE_CURRENT_BINARY_DIR}"
          DEPENDS
              "${CMAKE_CURRENT_LIST_DIR}/../cmake/stubs/generate_ui_stubs.py"
              ${_ui_headers}
          COMMENT "Generating UI stubs from binaryninja-api headers"
      )

      add_library(binaryninjaui_win_stub SHARED
          "${CMAKE_CURRENT_BINARY_DIR}/ui_stubs.cpp")
      target_link_libraries(binaryninjaui_win_stub PRIVATE
          binaryninjaapi Qt6::Core Qt6::Gui Qt6::Widgets)
      target_include_directories(binaryninjaui_win_stub PRIVATE "${_ui_headers_dir}")
      set_target_properties(binaryninjaui_win_stub PROPERTIES
          OUTPUT_NAME binaryninjaui
          ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
    endif()
  endif()
endif()
