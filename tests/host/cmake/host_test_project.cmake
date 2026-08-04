include("${CMAKE_CURRENT_LIST_DIR}/host_test_mode.cmake")

cmake_language(
    DEFER
    DIRECTORY "${CMAKE_SOURCE_DIR}"
    CALL include "${CMAKE_SOURCE_DIR}/cmake/extra_tests.cmake"
)
