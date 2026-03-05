# Add external dependencies that are vendored in this repository

# We don't want to set any flags for the external dependencies
set(CMAKE_CXX_FLAGS "")

# We don't want to lint the external dependencies
# Added in CMake 4.2
set(CMAKE_SKIP_LINTING ON)

# Make the binaryninja-api code available
set(HEADLESS ON)
add_subdirectory(binaryninja-api EXCLUDE_FROM_ALL SYSTEM)
