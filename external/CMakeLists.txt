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
#
# Set BN_QT_DIR to the Binary Ninja Qt installation (e.g. .../qt/Qt/6.10.1/clang_64)
# to ensure the plugin links against the same Qt that Binary Ninja ships.
if(BN_QT_DIR)
  list(PREPEND CMAKE_PREFIX_PATH "${BN_QT_DIR}")
endif()
find_package(Qt6 COMPONENTS Core Gui Widgets QUIET)
if(Qt6_FOUND)
  set(HEADLESS OFF)
else()
  set(HEADLESS ON)
endif()
add_subdirectory(binaryninja-api EXCLUDE_FROM_ALL SYSTEM)
