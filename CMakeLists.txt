cmake_minimum_required(VERSION 3.20)
project(polycpp_ini VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# Fetch polycpp (disable its tests when building as a dependency)
set(POLYCPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    polycpp
    GIT_REPOSITORY git@github.com:enricohuang/polycpp.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(polycpp)

# Library target
add_library(polycpp_ini
    src/ini.cpp
)
target_link_libraries(polycpp_ini PUBLIC polycpp)
target_include_directories(polycpp_ini PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Alias for consistent naming
add_library(polycpp::ini ALIAS polycpp_ini)

# Tests
option(POLYCPP_INI_BUILD_TESTS "Build tests" ON)
if(POLYCPP_INI_BUILD_TESTS)
    enable_testing()
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY git@github.com:google/googletest.git
        GIT_TAG        v1.15.2
    )
    FetchContent_MakeAvailable(googletest)

    add_executable(test_ini tests/test_ini.cpp)
    target_link_libraries(test_ini PRIVATE polycpp_ini GTest::gtest_main)
    target_compile_definitions(test_ini PRIVATE
        INI_TESTS_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
    )
    include(GoogleTest)
    gtest_discover_tests(test_ini)
endif()

# Examples
option(POLYCPP_INI_BUILD_EXAMPLES "Build examples" OFF)
if(POLYCPP_INI_BUILD_EXAMPLES)
    foreach(example_name IN ITEMS load_config reformat set_value)
        add_executable(polycpp_ini_example_${example_name} examples/${example_name}.cpp)
        target_link_libraries(polycpp_ini_example_${example_name} PRIVATE polycpp::ini)
        set_target_properties(polycpp_ini_example_${example_name} PROPERTIES
            OUTPUT_NAME ${example_name}
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
        )
    endforeach()
endif()
