find_package(GTest REQUIRED)

add_executable(ReturnHighlightRenderLayer_test)
target_sources(
    ReturnHighlightRenderLayer_test
    PRIVATE
        TestHelpers.cpp
        ReturnHighlightRenderLayer_test.cpp
)

target_link_libraries(
    ReturnHighlightRenderLayer_test
    PRIVATE
        BinjaReturnHighlighter::core
        ${BinaryNinjaCore_LIBRARIES}
        GTest::gtest
)

target_link_directories(
    ReturnHighlightRenderLayer_test
    PRIVATE
        ${BinaryNinjaCore_LIBRARY_DIRS}
)

target_compile_definitions(
    ReturnHighlightRenderLayer_test
    PRIVATE
        TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data/"
        BN_INSTALL_DIR="${BinaryNinjaCore_ROOT_DIR}"
)

add_test(NAME ReturnHighlightRenderLayer_test COMMAND ReturnHighlightRenderLayer_test)
