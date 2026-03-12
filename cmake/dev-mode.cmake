include(cmake/lint-targets.cmake)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/external/binaryninja-api/cmake")
find_package(BinaryNinjaCore QUIET)
if(BinaryNinjaCore_FOUND)
  enable_testing()
  add_subdirectory(test)
endif()
