option(
  BinjaReturnHighlighter_VENDORED_BINARYNINJAAPI
  "Use vendored Binary Ninja API project"
  ON
)

# ---- Developer mode ----

# Developer mode enables targets and code paths in the CMake scripts that are
# only relevant for the developer(s) of this project.
# Targets necessary to build the project must be provided unconditionally, so
# consumers can trivially build and package the project
if(PROJECT_IS_TOP_LEVEL)
  option(BinjaReturnHighlighter_DEVELOPER_MODE "Enable developer mode" OFF)
endif()
