set(
    FORMAT_PATTERNS
    source/*.cpp source/*.hpp
    include/*.hpp
    test/*.cpp test/*.hpp
    CACHE STRING
    "; separated patterns relative to the project source dir to format"
)

set(FORMAT_COMMAND clang-format CACHE STRING "Formatter to use")

add_custom_target(
    format-check
    COMMAND "${CMAKE_COMMAND}"
    -D "FORMAT_COMMAND=${FORMAT_COMMAND}"
    -D "PATTERNS=${FORMAT_PATTERNS}"
    -P "${PROJECT_SOURCE_DIR}/cmake/lint.cmake"
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    COMMENT "Linting the code"
    VERBATIM
)

add_custom_target(
    format-fix
    COMMAND "${CMAKE_COMMAND}"
    -D "FORMAT_COMMAND=${FORMAT_COMMAND}"
    -D "PATTERNS=${FORMAT_PATTERNS}"
    -D FIX=YES
    -P "${PROJECT_SOURCE_DIR}/cmake/lint.cmake"
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    COMMENT "Fixing the code"
    VERBATIM
)

# Extract clang-tidy path from CMAKE_CXX_CLANG_TIDY (first element is the binary)
# for consistency with build-time checks.
if(CMAKE_CXX_CLANG_TIDY)
  list(GET CMAKE_CXX_CLANG_TIDY 0 _tidy_default)
else()
  set(_tidy_default clang-tidy)
endif()
set(TIDY_COMMAND "${_tidy_default}" CACHE STRING "clang-tidy binary to use")

set(
    TIDY_PATTERNS
    source/*.cpp
    test/*.cpp
    CACHE STRING
    "; separated patterns relative to the project source dir for clang-tidy"
)

add_custom_target(
    tidy-check
    COMMAND "${CMAKE_COMMAND}"
    -D "TIDY_COMMAND=${TIDY_COMMAND}"
    -D "PATTERNS=${TIDY_PATTERNS}"
    -D "BUILD_DIR=${CMAKE_BINARY_DIR}"
    -P "${PROJECT_SOURCE_DIR}/cmake/tidy.cmake"
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    COMMENT "Checking clang-tidy diagnostics"
    VERBATIM
)

add_custom_target(
    tidy-fix
    COMMAND "${CMAKE_COMMAND}"
    -D "TIDY_COMMAND=${TIDY_COMMAND}"
    -D "PATTERNS=${TIDY_PATTERNS}"
    -D "BUILD_DIR=${CMAKE_BINARY_DIR}"
    -D FIX=YES
    -P "${PROJECT_SOURCE_DIR}/cmake/tidy.cmake"
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    COMMENT "Fixing clang-tidy diagnostics"
    VERBATIM
)
