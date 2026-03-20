cmake_minimum_required(VERSION 3.14)

macro(default name)
  if(NOT DEFINED "${name}")
    set("${name}" "${ARGN}")
  endif()
endmacro()

default(
    PATTERNS
    source/*.cpp
    test/*.cpp
)
default(FIX NO)

if(NOT DEFINED BUILD_DIR)
  message(FATAL_ERROR "BUILD_DIR must be set to the build directory containing compile_commands.json")
endif()

if(NOT EXISTS "${BUILD_DIR}/compile_commands.json")
  message(FATAL_ERROR "compile_commands.json not found in ${BUILD_DIR}. Configure the project first.")
endif()

default(TIDY_COMMAND clang-tidy)

set(fix_flag "")
if(FIX)
  message(STATUS "Fixing clang-tidy issues")
  set(fix_flag "--fix")
else()
  message(STATUS "Checking clang-tidy issues")
endif()

# On macOS, clang-tidy from Homebrew LLVM needs the Apple SDK sysroot
# to find system headers when the compilation database uses Apple Clang.
set(extra_args "")
if(CMAKE_HOST_APPLE)
  execute_process(
      COMMAND xcrun --show-sdk-path
      OUTPUT_VARIABLE sdk_path
      OUTPUT_STRIP_TRAILING_WHITESPACE
      RESULT_VARIABLE xcrun_result
  )
  if(xcrun_result EQUAL "0" AND sdk_path)
    set(extra_args "--extra-arg=-isysroot${sdk_path}")
  endif()
endif()

file(GLOB_RECURSE files ${PATTERNS})
set(files_with_diagnostics "")
string(LENGTH "${CMAKE_SOURCE_DIR}/" path_prefix_length)

foreach(file IN LISTS files)
  execute_process(
      COMMAND "${TIDY_COMMAND}" -p "${BUILD_DIR}" ${fix_flag} ${extra_args} "${file}"
      WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
      RESULT_VARIABLE result
  )
  if(NOT result EQUAL "0")
    string(SUBSTRING "${file}" "${path_prefix_length}" -1 relative_file)
    list(APPEND files_with_diagnostics "${relative_file}")
  endif()
endforeach()

if(NOT FIX AND NOT files_with_diagnostics STREQUAL "")
  list(JOIN files_with_diagnostics "\n" bad_list)
  message("The following files have clang-tidy diagnostics:\n\n${bad_list}\n")
  message(FATAL_ERROR "Run again with FIX=YES to auto-fix where possible.")
endif()
