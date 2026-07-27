include("${CMAKE_CURRENT_LIST_DIR}/host_test_mode.cmake")

# Register modular host targets after the repository's main CMakeLists has
# defined shared support libraries and strict warning settings.
cmake_language(
    DEFER
    DIRECTORY "${CMAKE_SOURCE_DIR}"
    CALL include "${CMAKE_SOURCE_DIR}/cmake/extra_tests.cmake"
)
