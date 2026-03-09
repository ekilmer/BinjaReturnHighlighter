# Add external dependencies that are vendored in this repository

# Remove warning flags for external dependencies
string(REGEX REPLACE "(^| )-W[^ ]*" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string(REGEX REPLACE "(^| )/[Ww][^ ]*" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string(STRIP "${CMAKE_CXX_FLAGS}" CMAKE_CXX_FLAGS)

# Workaround compiler error in API
#   error C4996: 'getenv': This function or variable may be unsafe.
#   Consider using _dupenv_s instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS.
if(MSVC)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

# We don't want to lint the external dependencies
# Added in CMake 4.2
set(CMAKE_SKIP_LINTING ON)

# Make the binaryninja-api code available
set(HEADLESS ON)
add_subdirectory(binaryninja-api EXCLUDE_FROM_ALL SYSTEM)
